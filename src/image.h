#pragma once
#include <string>

// Returns a textual caption of the image using an external vision model.
// Returns empty string if image_path is empty or captioning fails.
std::string caption_image(const std::string& image_path,
                           const std::string& vision_model_path);

// Returns true if the file extension looks like a supported image
bool is_image_file(const std::string& path);
