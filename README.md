# PopplerShot

**Batch PDF document to image converter**

PopplerShot is a high-performance, multi-threaded command-line tool for converting PDF documents to images. Built on top of the powerful Poppler library, it provides fast and reliable batch conversion with extensive customization options.

[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](https://github.com/Jss-on/PopplerShot/releases)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard)
[![CMake](https://img.shields.io/badge/CMake-3.20%2B-green.svg)](https://cmake.org/)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

## Features

### 🚀 **High Performance**
- **Multi-threaded processing** with automatic thread detection or manual configuration
- **Batch processing** of entire directories
- **Optimized memory usage** with efficient resource management
- **Progress tracking** with real-time updates

### 🎨 **Flexible Output Options**
- **Multiple output formats**: PNG (default), JPG
- **Configurable DPI resolution** (default: 300 DPI)
- **Custom image dimensions** with max width/height constraints
- **Aspect ratio control** (preserve or ignore)
- **Automatic output filename generation**

### 📁 **Smart File Management**
- **Recursive PDF discovery** in input directories
- **Automatic output directory creation**
- **Collision-safe output naming**
- **Comprehensive error handling and reporting**

### 🔧 **Developer-Friendly**
- **Modular C++ architecture** with clean separation of concerns
- **Comprehensive logging** with configurable verbosity levels
- **Cross-platform build system** (Linux, Windows via MinGW)
- **vcpkg integration** for dependency management

## Installation

### Prerequisites

#### System Dependencies
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install build-essential cmake ninja-build pkg-config

# For Windows cross-compilation (optional)
sudo apt install mingw-w64 mingw-w64-tools
```

#### Building from Source
```bash
# Clone the repository
git clone <repository-url>
cd PopplerShot

# Initialize vcpkg submodule
git submodule update --init --recursive

# Build (Linux)
./build.sh --release

# Build with custom options
./build.sh --debug --jobs 8 --clean

# Cross-compile for Windows (from Linux)
./build.sh --target windows --arch x64 --release
```

## 🚀 Automated Builds & Releases

PopplerShot features a fully automated CI/CD pipeline using GitHub Actions that builds for multiple platforms and creates releases automatically.

### 🔄 **Continuous Integration**

Every push to the main branch triggers automated builds for:
- **Linux (x64)** - Ubuntu latest with native dependencies
- **Windows (x64)** - Latest Windows with vcpkg dependencies

### 📦 **Automatic Releases**

The CI/CD pipeline automatically creates releases with downloadable binaries:

#### **Development Releases**
- **Trigger**: Every push to `main`/`master` branch  
- **Version Format**: `dev-{git-hash}-{date}` (e.g., `dev-a1b2c3d4-20240312`)
- **Type**: Pre-release
- **Downloads**: Platform-specific zip archives with executables

#### **Stable Releases** 
- **Trigger**: Push a git tag (e.g., `v1.0.0`)
- **Version Format**: Uses the tag name (e.g., `v1.0.0`)  
- **Type**: Full release
- **Downloads**: Platform-specific zip archives with executables

### 🏷️ **Creating a Release**

To create a new stable release:

```bash
# Tag the current commit
git tag v1.2.0
git push origin v1.2.0

# The CI will automatically:
# 1. Build for Linux and Windows
# 2. Create release archives  
# 3. Generate changelog from commits
# 4. Upload artifacts to GitHub Releases
```

### 📥 **Download Pre-built Binaries**

Get the latest builds from the [Releases page](../../releases):

1. **Latest Development Build** - Most recent features (may be unstable)
2. **Latest Stable Release** - Recommended for production use

Each release includes:
- `popplershot-linux-{version}.zip` - Linux x64 executable
- `popplershot-windows-{version}.zip` - Windows x64 executable  
- `VERSION.txt` - Build information and metadata

### 🔧 **CI/CD Pipeline Features**

- **Multi-platform Matrix Builds** - Parallel Windows and Linux builds
- **Dependency Caching** - vcpkg packages cached for faster builds
- **Automatic Versioning** - Semantic version injection into binaries
- **Smart Changelog Generation** - Auto-generated from git commit history
- **Artifact Management** - Build outputs stored as GitHub releases
- **Quality Gates** - Build must pass before release creation

### 🛠️ **Pipeline Configuration**

The pipeline is defined in `.github/workflows/build-and-release.yml` and includes:

- **Build Matrix**: Ubuntu + Windows runners  
- **Dependency Management**: vcpkg integration with caching
- **Version Strategy**: Tag-based releases + commit-based dev builds
- **Artifact Handling**: Zip archives with version metadata
- **Release Automation**: GitHub Releases with changelog generation

## Usage

### Command Line Interface

```bash
./popplershot [OPTIONS] INPUT_DIR OUTPUT_DIR
```

### Arguments
- `INPUT_DIR` - Directory containing PDF files to convert
- `OUTPUT_DIR` - Directory where PNG files will be saved

### Options

| Option | Description | Default |
|--------|-------------|---------|
| `-h, --help` | Show help message | - |
| `-v, --verbose` | Enable verbose logging | false |
| `-q, --quiet` | Suppress progress output | false |
| `-j, --jobs N` | Number of parallel threads | auto-detect |
| `-d, --dpi N` | Output DPI resolution | 300 |
| `-f, --format FORMAT` | Output format: png, jpg | png |
| `--max-width N` | Maximum output width in pixels | unlimited |
| `--max-height N` | Maximum output height in pixels | unlimited |
| `--no-aspect-ratio` | Don't preserve aspect ratio when scaling | false |

### Examples

```bash
# Basic conversion
./popplershot /path/to/pdfs /path/to/output

# High-performance conversion with custom settings
./popplershot -j 16 -d 150 /input /output

# Web-optimized output with size constraints
./popplershot --max-width 1920 --max-height 1080 -d 150 /pdfs /images

# Quiet batch processing
./popplershot --quiet --format jpg /documents /converted
```

## Architecture

### Core Components

#### 1. **PDFConverter Class** (`include/pdf_converter.h`)
Core PDF processing engine with the following capabilities:

**Key Features:**
- Single PDF and individual page conversion
- Configurable output options via `ConversionOptions`
- Automatic output filename generation
- Error handling with detailed `ConversionResult`

**API:**
```cpp
class PDFConverter {
public:
    struct ConversionOptions {
        double dpi = 300.0;
        std::string output_format = "png";
        bool preserve_aspect_ratio = true;
        int max_width = 0;  // 0 means no limit
        int max_height = 0; // 0 means no limit
    };

    struct ConversionResult {
        bool success;
        std::string error_message;
        int pages_converted;
    };

    ConversionResult convert_pdf(const std::string& pdf_path, 
                               const std::string& output_dir,
                               const ConversionOptions& options = {});

    ConversionResult convert_page(const std::string& pdf_path,
                                int page_number,
                                const std::string& output_path,
                                const ConversionOptions& options = {});
};
```

#### 2. **BatchProcessor Class** (`include/batch_processor.h`)
Multi-threaded batch processing engine with the following capabilities:

**Key Features:**
- Parallel processing across multiple threads
- Real-time progress callbacks
- Cancellation support
- Comprehensive batch result reporting

**API:**
```cpp
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

    BatchResult process_directory(const std::string& input_dir,
                                const std::string& output_dir,
                                const PDFConverter::ConversionOptions& options = {},
                                ProgressCallback progress_callback = nullptr);
};
```

#### 3. **FileUtils Class** (`include/file_utils.h`)
Comprehensive file system utilities:

**Key Features:**
- PDF file discovery and filtering
- Directory creation and validation
- Path manipulation and normalization
- Cross-platform file operations

**API:**
```cpp
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
```

### Application Flow

1. **Command Line Parsing** - Arguments processed with comprehensive validation
2. **Logging Setup** - Configurable logging with spdlog integration  
3. **Directory Validation** - Input/output directory verification
4. **Batch Processing** - Multi-threaded PDF discovery and conversion
5. **Progress Reporting** - Real-time progress updates with timing information
6. **Result Summary** - Detailed completion statistics and error reporting

## Dependencies

### Core Libraries
- **Poppler C++** - PDF rendering engine
- **fmt** - Fast and safe C++ formatting library
- **spdlog** - Fast C++ logging library

### Build System
- **CMake 3.22+** - Build system generator
- **vcpkg** - C++ package manager
- **Ninja** - Fast build tool

## Build Configuration

### CMake Presets

The project includes comprehensive CMake presets for different platforms and configurations:

#### Configure Presets
- `linux-x64` - Linux 64-bit native build
- `linux-x86` - Linux 32-bit native build  
- `windows-x64` - Windows 64-bit cross-compilation (MinGW)
- `windows-x86` - Windows 32-bit cross-compilation (MinGW)

#### Build Presets
Each configure preset supports multiple build types:
- `{preset}-debug` - Debug build with symbols
- `{preset}-release` - Optimized release build
- `{preset}-relwithdebinfo` - Release with debug info
- `{preset}-minsizerel` - Size-optimized release

### Cross-Platform Support

#### Linux (Native)
```bash
./build.sh --arch x64 --release  # 64-bit release
./build.sh --arch x86 --debug    # 32-bit debug
```

#### Windows (Cross-compilation)
```bash
./build.sh --target windows --arch x64 --release
./build.sh --target windows --arch x86 --debug
```

## Performance

### Optimization Features
- **Thread pool architecture** for optimal CPU utilization
- **Memory-efficient PDF loading** with automatic cleanup
- **Vectorized image processing** via Poppler's optimized renderer
- **Minimal file system overhead** with batch operations

### Benchmarks
- **Typical performance**: 2-5 pages/second per thread on modern hardware
- **Scalability**: Linear performance scaling with thread count
- **Memory usage**: ~50-100MB base + ~10-20MB per concurrent PDF

## Error Handling

### Robust Error Management
- **Per-file error isolation** - Single PDF failures don't stop batch processing
- **Detailed error reporting** - Specific error messages for each failure
- **Graceful degradation** - Continues processing remaining files after errors
- **Comprehensive logging** - Full audit trail of all operations

### Common Error Scenarios
- Corrupted or encrypted PDF files
- Insufficient disk space for output
- Permission issues with input/output directories
- Unsupported PDF features or formats

## Contributing

### Development Setup
1. Fork the repository
2. Set up development environment with required dependencies
3. Run tests to ensure everything works
4. Make changes following the coding standards
5. Submit a pull request

### Code Style
- Modern C++17 standards
- RAII principles for resource management
- Clear separation of concerns
- Comprehensive error handling
- Extensive logging for debugging

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Developer

**Jession Diwangan** - *Main Developer*  
📧 Email: jessiondiwangan@gmail.com

## Support

For issues, questions, or contributions, please contact:
- **Email**: jessiondiwangan@gmail.com
- **Issues**: Create an issue in this repository

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request or reach out with suggestions and improvements.

---

**PopplerShot** - Making PDF to image conversion fast, reliable, and developer-friendly.
