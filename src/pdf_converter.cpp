#include "pdf_converter.h"
#include "progress_bar.h"
#include <iostream>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <poppler-image.h>
#include <future>
#include <thread>
#include <mutex>
#include <semaphore>

namespace popplershot {

PDFConverter::PDFConverter() = default;
PDFConverter::~PDFConverter() = default;

std::unique_ptr<poppler::document> PDFConverter::load_document(const std::string& pdf_path) {
    auto doc = std::unique_ptr<poppler::document>(poppler::document::load_from_file(pdf_path));
    if (!doc || doc->is_locked()) {
        spdlog::error("Failed to load PDF: {}", pdf_path);
        return nullptr;
    }
    return doc;
}

// Convenience overload with default options
PDFConverter::ConversionResult PDFConverter::convert_pdf(const std::string& pdf_path, 
                                                       const std::string& output_dir) {
    ConversionOptions default_options;
    return convert_pdf(pdf_path, output_dir, default_options);
}

PDFConverter::ConversionResult PDFConverter::convert_pdf(const std::string& pdf_path, 
                                                       const std::string& output_dir,
                                                       const ConversionOptions& options) {
    ConversionResult result{false, "", 0};
    
    auto doc = load_document(pdf_path);
    if (!doc) {
        result.error_message = "Failed to load PDF document";
        return result;
    }

    int page_count = doc->pages();
    spdlog::info("Converting PDF: {} ({} pages)", pdf_path, page_count);

    // Pre-create output directory to avoid repeated filesystem calls
    std::filesystem::create_directories(output_dir);

    // Create progress bar for page conversion
    ProgressBar progress_bar(page_count, 40, "█", "░");
    progress_bar.set_description("Converting pages");

    // Use controlled parallel processing for pages to prevent memory exhaustion
    // Limit concurrent page conversions to prevent OOM kills on large PDFs
    const int max_concurrent_pages = std::min(8, std::max(2, static_cast<int>(std::thread::hardware_concurrency())));
    std::counting_semaphore<> page_semaphore(max_concurrent_pages);
    std::vector<std::future<bool>> futures;
    std::mutex doc_mutex; // Protect document access
    
    spdlog::info("Using {} concurrent page conversions (max memory safety)", max_concurrent_pages);
    
    for (int i = 0; i < page_count; ++i) {
        auto future = std::async(std::launch::async, [&, i]() -> bool {
            // Acquire semaphore before processing page (blocks if at limit)
            page_semaphore.acquire();
            
            // Ensure semaphore is released even if exception occurs
            struct SemaphoreGuard {
                std::counting_semaphore<>& sem;
                SemaphoreGuard(std::counting_semaphore<>& s) : sem(s) {}
                ~SemaphoreGuard() { sem.release(); }
            } guard(page_semaphore);
            
            std::unique_ptr<poppler::page> page;
            {
                std::lock_guard<std::mutex> lock(doc_mutex);
                page = std::unique_ptr<poppler::page>(doc->create_page(i));
            }
            
            if (!page) {
                spdlog::warn("Failed to create page {}", i + 1);
                progress_bar.update();
                return false;
            }

            std::string output_filename = generate_output_filename(pdf_path, i + 1, options.output_format);
            std::string output_path = std::filesystem::path(output_dir) / output_filename;

            bool success = save_page_as_image(page.get(), output_path, options);
            if (success) {
                spdlog::debug("Converted page {} to {}", i + 1, output_path);
            } else {
                spdlog::warn("Failed to convert page {} of {}", i + 1, pdf_path);
            }
            
            // Update progress bar after page completion
            progress_bar.update();
            return success;
        });
        
        futures.push_back(std::move(future));
    }

    // Collect results
    for (auto& future : futures) {
        try {
            if (future.get()) {
                result.pages_converted++;
            }
        } catch (const std::exception& e) {
            spdlog::error("Exception during page conversion: {}", e.what());
        }
    }
    
    // Finish progress bar
    progress_bar.finish();

    result.success = result.pages_converted > 0;
    if (!result.success) {
        result.error_message = "No pages were successfully converted";
    }

    return result;
}

// Convenience overload with default options  
PDFConverter::ConversionResult PDFConverter::convert_page(const std::string& pdf_path,
                                                      int page_number,
                                                      const std::string& output_path) {
    ConversionOptions default_options;
    return convert_page(pdf_path, page_number, output_path, default_options);
}

PDFConverter::ConversionResult PDFConverter::convert_page(const std::string& pdf_path,
                                                      int page_number,
                                                      const std::string& output_path,
                                                      const ConversionOptions& options) {
    ConversionResult result{false, "", 0};
    
    auto doc = load_document(pdf_path);
    if (!doc) {
        result.error_message = "Failed to load PDF document";
        return result;
    }

    if (page_number < 1 || page_number > doc->pages()) {
        result.error_message = "Invalid page number";
        return result;
    }

    auto page = std::unique_ptr<poppler::page>(doc->create_page(page_number - 1));
    if (!page) {
        result.error_message = "Failed to create page";
        return result;
    }

    if (save_page_as_image(page.get(), output_path, options)) {
        result.success = true;
        result.pages_converted = 1;
    } else {
        result.error_message = "Failed to save page as image";
    }

    return result;
}

bool PDFConverter::save_page_as_image(poppler::page* page, 
                                    const std::string& output_path,
                                    const ConversionOptions& options) {
    if (!page) return false;

    poppler::page_renderer renderer;
    renderer.set_render_hint(poppler::page_renderer::antialiasing, true);
    renderer.set_render_hint(poppler::page_renderer::text_antialiasing, true);

    // Get page dimensions
    poppler::rectf page_rect = page->page_rect();
    double page_width = page_rect.width();
    double page_height = page_rect.height();

    // Calculate scaling factors
    double scale_x = options.dpi / 72.0;
    double scale_y = options.dpi / 72.0;

    // Apply max width/height constraints if specified
    if (options.max_width > 0 || options.max_height > 0) {
        double target_width = page_width * scale_x;
        double target_height = page_height * scale_y;

        if (options.max_width > 0 && target_width > options.max_width) {
            scale_x = options.max_width / page_width;
        }
        if (options.max_height > 0 && target_height > options.max_height) {
            scale_y = options.max_height / page_height;
        }

        if (options.preserve_aspect_ratio) {
            double min_scale = std::min(scale_x, scale_y);
            scale_x = scale_y = min_scale;
        }
    }

    // Render the page
    poppler::image img = renderer.render_page(page, 
                                            scale_x * 72.0, scale_y * 72.0);
    
    if (!img.is_valid()) {
        spdlog::error("Failed to render page");
        return false;
    }

    // Ensure output directory exists
    std::filesystem::path output_file_path(output_path);
    std::filesystem::create_directories(output_file_path.parent_path());

    // Save the image
    bool saved = false;
    if (options.output_format == "png") {
        saved = img.save(output_path, "png");
    } else if (options.output_format == "jpg" || options.output_format == "jpeg") {
        saved = img.save(output_path, "jpeg");
    } else {
        saved = img.save(output_path, options.output_format);
    }

    if (!saved) {
        spdlog::error("Failed to save image: {}", output_path);
    }

    return saved;
}

std::string PDFConverter::generate_output_filename(const std::string& pdf_path, 
                                                 int page_number,
                                                 const std::string& extension) {
    std::filesystem::path path(pdf_path);
    std::string base_name = path.stem().string();
    
    // Format: filename_page_001.png
    return base_name + "_page_" + 
           std::string(3 - std::to_string(page_number).length(), '0') + 
           std::to_string(page_number) + "." + extension;
}

} // namespace popplershot
