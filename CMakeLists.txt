cmake_minimum_required(VERSION 3.16)
project(pybind_collections_wrapper LANGUAGES CXX)

find_package(Python 3.10 REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 REQUIRED)

# Header-only library target
add_library(pybind_collections_wrapper INTERFACE)

# Expose include directory to dependents
target_include_directories(pybind_collections_wrapper INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Require at least C++20 (adjust as needed)
target_compile_features(template-helpers INTERFACE cxx_std_20)