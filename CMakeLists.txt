cmake_minimum_required(VERSION 3.14)
project(frame-ai)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Require Windows 10 for cpp-httplib (uses modern Win32 API)
if(WIN32)
  add_compile_definitions(_WIN32_WINNT=0x0A00 WINVER=0x0A00)
endif()


# llama.cpp
add_subdirectory(vendor/llama.cpp)

# Frame AI sources
add_executable(frame-ai
    src/main.cpp
    src/model.cpp
    src/prompt.cpp
    src/server.cpp
    src/image.cpp
    src/utils.cpp
)

target_include_directories(frame-ai PRIVATE
    src/
    src/vendor/
    vendor/llama.cpp/include/
    vendor/llama.cpp/ggml/include/
)

target_link_libraries(frame-ai PRIVATE llama)

# Platform-specific threading
find_package(Threads REQUIRED)
target_link_libraries(frame-ai PRIVATE Threads::Threads)

# Windows: link winsock for cpp-httplib
if(WIN32)
    target_link_libraries(frame-ai PRIVATE ws2_32)
endif()
