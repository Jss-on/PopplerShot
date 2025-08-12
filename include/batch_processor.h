#pragma once

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include "pdf_converter.h"

namespace popplershot {

class BatchProcessor {
public:
    struct BatchResult {
        int total_pdfs;
        int successful_conversions;
        int failed_conversions;
        int total_pages_converted;
        std::vector<std::string> errors;
    };

    struct ProgressInfo {
        int current_file;
        int total_files;
        std::string current_filename;
        int pages_processed;
    };

    using ProgressCallback = std::function<void(const ProgressInfo&)>;

    BatchProcessor(int num_threads = std::thread::hardware_concurrency());
    ~BatchProcessor();

    // Function overloads for convenience (default options and callback)
    BatchResult process_directory(const std::string& input_dir,
                                const std::string& output_dir);
                                
    BatchResult process_directory(const std::string& input_dir,
                                const std::string& output_dir,
                                const PDFConverter::ConversionOptions& options);
                                
    BatchResult process_directory(const std::string& input_dir,
                                const std::string& output_dir,
                                const PDFConverter::ConversionOptions& options,
                                ProgressCallback progress_callback);

    void set_thread_count(int num_threads);
    void cancel_processing();

private:
    void worker_thread(const std::vector<std::string>& pdf_files,
                      const std::string& output_dir,
                      const PDFConverter::ConversionOptions& options,
                      ProgressCallback progress_callback,
                      BatchResult& result,
                      std::mutex& result_mutex,
                      std::atomic<int>& file_index);

    int num_threads_;
    std::atomic<bool> cancel_requested_;
    PDFConverter converter_;
};

} // namespace popplershot
