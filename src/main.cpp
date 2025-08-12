#include <iostream>
#include <string>
#include <chrono>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <fmt/format.h>

#include "batch_processor.h"
#include "pdf_converter.h"
#include "file_utils.h"

void print_usage(const char* program_name) {
    std::cout << "PopplerShot - Efficient batch PDF to PNG converter\n\n";
    std::cout << "Usage: " << program_name << " [OPTIONS] INPUT_DIR OUTPUT_DIR\n\n";
    std::cout << "Arguments:\n";
    std::cout << "  INPUT_DIR    Directory containing PDF files to convert\n";
    std::cout << "  OUTPUT_DIR   Directory where PNG files will be saved\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help           Show this help message\n";
    std::cout << "  -v, --verbose        Enable verbose logging\n";
    std::cout << "  -q, --quiet          Suppress progress output\n";
    std::cout << "  -j, --jobs N         Number of parallel threads (default: auto)\n";
    std::cout << "  -d, --dpi N          Output DPI resolution (default: 150)\n";
    std::cout << "  -f, --format FORMAT  Output format: png, jpg (default: png)\n";
    std::cout << "  --max-width N        Maximum output width in pixels\n";
    std::cout << "  --max-height N       Maximum output height in pixels\n";
    std::cout << "  --no-aspect-ratio    Don't preserve aspect ratio when scaling\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << program_name << " /data /output\n";
    std::cout << "  " << program_name << " -j 8 -d 200 /pdfs /images\n";
    std::cout << "  " << program_name << " --max-width 1920 /input /output\n";
}

void setup_logging(bool verbose, bool quiet) {
    auto console = spdlog::stdout_color_mt("console");
    spdlog::set_default_logger(console);
    
    if (quiet) {
        spdlog::set_level(spdlog::level::warn);
    } else if (verbose) {
        spdlog::set_level(spdlog::level::debug);
    } else {
        spdlog::set_level(spdlog::level::info);
    }
    
    spdlog::set_pattern("[%H:%M:%S] [%^%l%$] %v");
}

void show_progress(const popplershot::BatchProcessor::ProgressInfo& progress) {
    static auto start_time = std::chrono::steady_clock::now();
    auto current_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time);
    
    double progress_percent = (double)progress.current_file / progress.total_files * 100.0;
    
    fmt::print("\r[{:3.0f}%] Processing file {}/{}: {} (Pages: {}) [{:02d}:{:02d}]",
               progress_percent,
               progress.current_file,
               progress.total_files,
               progress.current_filename,
               progress.pages_processed,
               elapsed.count() / 60,
               elapsed.count() % 60);
    std::cout.flush();
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    std::string input_dir, output_dir;
    int num_threads = 0;
    double dpi = 300.0;
    std::string format = "png";
    int max_width = 0;
    int max_height = 0;
    bool preserve_aspect_ratio = true;
    bool verbose = false;
    bool quiet = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "-q" || arg == "--quiet") {
            quiet = true;
        } else if (arg == "-j" || arg == "--jobs") {
            if (i + 1 < argc) {
                num_threads = std::stoi(argv[++i]);
            }
        } else if (arg == "-d" || arg == "--dpi") {
            if (i + 1 < argc) {
                dpi = std::stod(argv[++i]);
            }
        } else if (arg == "-f" || arg == "--format") {
            if (i + 1 < argc) {
                format = argv[++i];
            }
        } else if (arg == "--max-width") {
            if (i + 1 < argc) {
                max_width = std::stoi(argv[++i]);
            }
        } else if (arg == "--max-height") {
            if (i + 1 < argc) {
                max_height = std::stoi(argv[++i]);
            }
        } else if (arg == "--no-aspect-ratio") {
            preserve_aspect_ratio = false;
        } else if (arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << std::endl;
            print_usage(argv[0]);
            return 1;
        } else {
            // Positional arguments
            if (input_dir.empty()) {
                input_dir = arg;
            } else if (output_dir.empty()) {
                output_dir = arg;
            }
        }
    }
    
    // Validate arguments
    if (input_dir.empty() || output_dir.empty()) {
        std::cerr << "Error: Both input and output directories must be specified\n\n";
        print_usage(argv[0]);
        return 1;
    }
    
    // Setup logging
    setup_logging(verbose, quiet);
    
    // Validate input directory
    if (!popplershot::FileUtils::is_directory(input_dir)) {
        spdlog::error("Input directory does not exist: {}", input_dir);
        return 1;
    }
    
    // Create conversion options
    popplershot::PDFConverter::ConversionOptions options;
    options.dpi = dpi;
    options.output_format = format;
    options.max_width = max_width;
    options.max_height = max_height;
    options.preserve_aspect_ratio = preserve_aspect_ratio;
    
    // Initialize batch processor
    popplershot::BatchProcessor processor(num_threads);
    
    spdlog::info("PopplerShot starting conversion");
    spdlog::info("Input directory: {}", input_dir);
    spdlog::info("Output directory: {}", output_dir);
    spdlog::info("DPI: {}", dpi);
    spdlog::info("Format: {}", format);
    if (num_threads > 0) {
        spdlog::info("Threads: {}", num_threads);
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Process directory
    auto progress_callback = quiet ? nullptr : show_progress;
    auto result = processor.process_directory(input_dir, output_dir, options, progress_callback);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    if (!quiet) {
        std::cout << std::endl; // New line after progress
    }
    
    // Print results
    spdlog::info("Conversion completed in {:.2f} seconds", duration.count() / 1000.0);
    spdlog::info("PDFs processed: {}/{}", result.successful_conversions, result.total_pdfs);
    spdlog::info("Total pages converted: {}", result.total_pages_converted);
    
    if (result.failed_conversions > 0) {
        spdlog::warn("Failed conversions: {}", result.failed_conversions);
        if (verbose) {
            for (const auto& error : result.errors) {
                spdlog::error("  {}", error);
            }
        }
    }
    
    if (result.successful_conversions == 0) {
        spdlog::error("No PDFs were successfully converted");
        return 1;
    }
    
    spdlog::info("PopplerShot completed successfully");
    return 0;
}
