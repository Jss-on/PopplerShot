#pragma once

#include <string>
#include <vector>

namespace popplershot {

class FileUtils {
public:
    static std::vector<std::string> find_pdf_files(const std::string& directory);
    static bool create_directories(const std::string& path);
    static bool file_exists(const std::string& path);
    static bool is_directory(const std::string& path);
    static std::string get_filename_without_extension(const std::string& filepath);
    static std::string get_parent_directory(const std::string& filepath);
    static std::string join_path(const std::string& dir, const std::string& filename);
    static bool ensure_output_directory(const std::string& output_dir);
};

} // namespace popplershot
