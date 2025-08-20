

cmake_minimum_required(VERSION 3.15)
project(SystemMonitor)

# Enable C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")


set(CURL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows-static/include")
set(CURL_LIBRARY "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows-static/lib/libcurl.lib")
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})
link_directories("${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/lib")

# Source files
file(GLOB SRC_FILES src/*.cpp)

# Include directories
include_directories(include)


# Executable
add_executable(SystemMonitor ${SRC_FILES} main.cpp)

# Set output directory for Release builds
set_target_properties(SystemMonitor PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin"
)

# Link with static libcurl and required system libraries
set(ZLIB_LIBRARY "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows-static/lib/zlib.lib")
target_link_libraries(SystemMonitor PRIVATE ${CURL_LIBRARY} ${ZLIB_LIBRARY} advapi32 ws2_32 crypt32 Secur32 IPHLPAPI)
target_compile_definitions(SystemMonitor PRIVATE CURL_STATICLIB)

