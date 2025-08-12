#include "batch_processor.h"
#include "file_utils.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <future>

namespace popplershot {

BatchProcessor::BatchProcessor(int num_threads) 
    : num_threads_(num_threads), cancel_requested_(false) {
    if (num_threads_ <= 0) {
        num_threads_ = std::thread::hardware_concurrency();
    }
    spdlog::info("BatchProcessor initialized with {} threads", num_threads_);
}

BatchProcessor::~BatchProcessor() {
    cancel_requested_ = true;
}

// Convenience overloads with default options/callback
BatchProcessor::BatchResult BatchProcessor::process_directory(
    const std::string& input_dir,
    const std::string& output_dir) {
    PDFConverter::ConversionOptions default_options;
    return process_directory(input_dir, output_dir, default_options, nullptr);
}

BatchProcessor::BatchResult BatchProcessor::process_directory(
    const std::string& input_dir,
    const std::string& output_dir,
    const PDFConverter::ConversionOptions& options) {
    return process_directory(input_dir, output_dir, options, nullptr);
}

BatchProcessor::BatchResult BatchProcessor::process_directory(
    const std::string& input_dir,
    const std::string& output_dir,
    const PDFConverter::ConversionOptions& options,
    ProgressCallback progress_callback) {
    
    BatchResult result{0, 0, 0, 0, {}};
    cancel_requested_ = false;

    // Find all PDF files in the input directory
    std::vector<std::string> pdf_files = FileUtils::find_pdf_files(input_dir);
    result.total_pdfs = static_cast<int>(pdf_files.size());

    if (pdf_files.empty()) {
        spdlog::warn("No PDF files found in directory: {}", input_dir);
        result.errors.push_back("No PDF files found in input directory");
        return result;
    }

    // Ensure output directory exists
    if (!FileUtils::ensure_output_directory(output_dir)) {
        spdlog::error("Failed to create output directory: {}", output_dir);
        result.errors.push_back("Failed to create output directory");
        return result;
    }

    spdlog::info("Processing {} PDF files using {} threads", pdf_files.size(), num_threads_);

    // Prepare threading variables
    std::mutex result_mutex;
    std::atomic<int> file_index(0);
    std::vector<std::thread> workers;

    // Launch worker threads
    for (int i = 0; i < num_threads_ && !cancel_requested_; ++i) {
        workers.emplace_back([this, &pdf_files, &output_dir, &options, 
                             progress_callback, &result, &result_mutex, &file_index]() {
            worker_thread(pdf_files, output_dir, options, progress_callback, 
                         result, result_mutex, file_index);
        });
    }

    // Wait for all threads to complete
    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    spdlog::info("Batch processing completed. Success: {}/{}, Pages: {}", 
                result.successful_conversions, result.total_pdfs, result.total_pages_converted);

    return result;
}

void BatchProcessor::worker_thread(
    const std::vector<std::string>& pdf_files,
    const std::string& output_dir,
    const PDFConverter::ConversionOptions& options,
    ProgressCallback progress_callback,
    BatchResult& result,
    std::mutex& result_mutex,
    std::atomic<int>& file_index) {
    
    while (!cancel_requested_) {
        int current_index = file_index.fetch_add(1);
        if (current_index >= static_cast<int>(pdf_files.size())) {
            break;
        }

        const std::string& pdf_file = pdf_files[current_index];
        
        // Update progress
        if (progress_callback) {
            ProgressInfo progress;
            progress.current_file = current_index + 1;
            progress.total_files = static_cast<int>(pdf_files.size());
            progress.current_filename = FileUtils::get_filename_without_extension(pdf_file);
            
            {
                std::lock_guard<std::mutex> lock(result_mutex);
                progress.pages_processed = result.total_pages_converted;
            }
            
            progress_callback(progress);
        }

        // Convert the PDF
        auto conversion_result = converter_.convert_pdf(pdf_file, output_dir, options);
        
        // Update results
        {
            std::lock_guard<std::mutex> lock(result_mutex);
            if (conversion_result.success) {
                result.successful_conversions++;
                result.total_pages_converted += conversion_result.pages_converted;
            } else {
                result.failed_conversions++;
                result.errors.push_back(pdf_file + ": " + conversion_result.error_message);
            }
        }
    }
}

void BatchProcessor::set_thread_count(int num_threads) {
    num_threads_ = num_threads > 0 ? num_threads : std::thread::hardware_concurrency();
}

void BatchProcessor::cancel_processing() {
    cancel_requested_ = true;
    spdlog::info("Batch processing cancellation requested");
}

} // namespace popplershot
