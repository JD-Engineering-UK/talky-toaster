# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include(generate_deets.cmake)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
include($ENV{ADF_PATH}/CMakeLists.txt)
idf_build_set_property(MINIMAL_BUILD ON)
project(talky_toaster)

generate_deets(esp32s3)
