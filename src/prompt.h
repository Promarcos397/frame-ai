#pragma once
#include <string>

struct ArtBrief {
    std::string colour_palette;
    std::string lighting_mood;
    std::string composition_style;
    std::string art_references;
    std::string texture_material;
    std::string tone_atmosphere;
    std::string what_to_avoid;
    bool valid = false;
};

std::string build_system_prompt();
std::string build_user_input(const std::string& text,
                              const std::string& image_caption = "");
ArtBrief    parse_brief(const std::string& raw_output);
std::string format_brief_terminal(const ArtBrief& brief);
std::string format_brief_json(const ArtBrief& brief);
