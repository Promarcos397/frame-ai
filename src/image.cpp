#include "image.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <cstdio>
#include <string>

bool is_image_file(const std::string& path) {
    std::string ext = std::filesystem::path(path).extension().string();
    for (char& c : ext) c = (char)tolower((unsigned char)c);
    return ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp";
}

std::string caption_image(const std::string& image_path,
                           const std::string& vision_model_path) {
    if (image_path.empty() || vision_model_path.empty()) return "";

    if (!is_image_file(image_path)) {
        std::cerr << "[artdir] Warning: not a recognised image file: " << image_path << "\n";
        return "";
    }

    // Use llama.cpp's llava CLI tool as a subprocess for image captioning.
    // This is the Phase-1 approach: the vision model runs as a child process
    // and its stdout caption is captured.
    // Phase 2 would integrate native llava/clip support directly into the binary.

    std::string command =
        "llama-llava-cli"
        " -m \"" + vision_model_path + "\""
        " --image \"" + image_path + "\""
        " -p \"Describe this image in detail for an art director. "
              "Include dominant colours, mood, lighting, composition, "
              "and any notable visual elements. Be specific and concise.\""
        " --no-display-prompt"
        " -ngl 0"
        " 2>/dev/null";

#ifdef _WIN32
    // Redirect stderr on Windows differently
    command =
        "llama-llava-cli"
        " -m \"" + vision_model_path + "\""
        " --image \"" + image_path + "\""
        " -p \"Describe this image in detail for an art director. "
              "Include dominant colours, mood, lighting, composition, "
              "and any notable visual elements. Be specific and concise.\""
        " --no-display-prompt"
        " -ngl 0"
        " 2>nul";
#endif

    FILE* pipe =
#ifdef _WIN32
        _popen(command.c_str(), "r");
#else
        popen(command.c_str(), "r");
#endif

    if (!pipe) {
        std::cerr << "[artdir] Warning: could not run vision model — image input ignored\n";
        return "";
    }

    std::string result;
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }

#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif

    return result;
}
