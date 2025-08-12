cmake_minimum_required(VERSION 3.20)

project(PopplerShot 
    VERSION 1.0.0
    DESCRIPTION "Efficient batch PDF to PNG converter"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# Find poppler-cpp
pkg_check_modules(POPPLER_CPP REQUIRED poppler-cpp)

# Create executable
add_executable(popplershot
    src/main.cpp
    src/pdf_converter.cpp
    src/batch_processor.cpp
    src/file_utils.cpp
    src/progress_bar.cpp
)

# Set target properties
target_include_directories(popplershot PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${POPPLER_CPP_INCLUDE_DIRS}
)

target_link_libraries(popplershot PRIVATE
    ${POPPLER_CPP_LIBRARIES}
    fmt::fmt
    spdlog::spdlog
)

target_compile_options(popplershot PRIVATE
    ${POPPLER_CPP_CFLAGS_OTHER}
    -Wall -Wextra -O3
)

# Link directories
target_link_directories(popplershot PRIVATE
    ${POPPLER_CPP_LIBRARY_DIRS}
)

# Install target
install(TARGETS popplershot
    RUNTIME DESTINATION bin
)
