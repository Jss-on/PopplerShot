#pragma once

#include <string>
#include <vector>
#include <memory>
#include <poppler-document.h>
#include <poppler-page.h>
#include <poppler-page-renderer.h>

namespace popplershot {

class PDFConverter {
public:
    struct ConversionResult {
        bool success;
        std::string error_message;
        int pages_converted;
    };

    struct ConversionOptions {
        double dpi = 300.0;
        std::string output_format = "png";
        bool preserve_aspect_ratio = true;
        int max_width = 0;  // 0 means no limit
        int max_height = 0; // 0 means no limit
    };

    PDFConverter();
    ~PDFConverter();

    // Function overloads for convenience (default options)
    ConversionResult convert_pdf(const std::string& pdf_path, 
                               const std::string& output_dir);
    
    ConversionResult convert_pdf(const std::string& pdf_path, 
                               const std::string& output_dir,
                               const ConversionOptions& options);

    // Function overloads for page conversion
    ConversionResult convert_page(const std::string& pdf_path,
                                int page_number,
                                const std::string& output_path);
                                
    ConversionResult convert_page(const std::string& pdf_path,
                                int page_number,
                                const std::string& output_path,
                                const ConversionOptions& options);

    static std::string generate_output_filename(const std::string& pdf_path, 
                                              int page_number,
                                              const std::string& extension = "png");

private:
    std::unique_ptr<poppler::document> load_document(const std::string& pdf_path);
    bool save_page_as_image(poppler::page* page, 
                          const std::string& output_path,
                          const ConversionOptions& options);
};

} // namespace popplershot
