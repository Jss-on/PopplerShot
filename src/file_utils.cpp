#include "file_utils.h"
#include <filesystem>
#include <algorithm>
#include <spdlog/spdlog.h>

namespace popplershot {

std::vector<std::string> FileUtils::find_pdf_files(const std::string& directory) {
    std::vector<std::string> pdf_files;
    
    if (!is_directory(directory)) {
        spdlog::error("Directory does not exist: {}", directory);
        return pdf_files;
    }

    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                std::string filepath = entry.path().string();
                std::string extension = entry.path().extension().string();
                
                // Convert extension to lowercase for comparison
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                
                if (extension == ".pdf") {
                    pdf_files.push_back(filepath);
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& ex) {
        spdlog::error("Error reading directory {}: {}", directory, ex.what());
    }

    spdlog::info("Found {} PDF files in directory: {}", pdf_files.size(), directory);
    return pdf_files;
}

bool FileUtils::create_directories(const std::string& path) {
    try {
        return std::filesystem::create_directories(path);
    } catch (const std::filesystem::filesystem_error& ex) {
        spdlog::error("Failed to create directories {}: {}", path, ex.what());
        return false;
    }
}

bool FileUtils::file_exists(const std::string& path) {
    return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
}

bool FileUtils::is_directory(const std::string& path) {
    return std::filesystem::exists(path) && std::filesystem::is_directory(path);
}

std::string FileUtils::get_filename_without_extension(const std::string& filepath) {
    std::filesystem::path path(filepath);
    return path.stem().string();
}

std::string FileUtils::get_parent_directory(const std::string& filepath) {
    std::filesystem::path path(filepath);
    return path.parent_path().string();
}

std::string FileUtils::join_path(const std::string& dir, const std::string& filename) {
    std::filesystem::path path = std::filesystem::path(dir) / filename;
    return path.string();
}

bool FileUtils::ensure_output_directory(const std::string& output_dir) {
    if (is_directory(output_dir)) {
        return true;
    }
    
    try {
        std::filesystem::create_directories(output_dir);
        spdlog::info("Created output directory: {}", output_dir);
        return true;
    } catch (const std::filesystem::filesystem_error& ex) {
        spdlog::error("Failed to create output directory {}: {}", output_dir, ex.what());
        return false;
    }
}

} // namespace popplershot
