#include "prompt.h"
#include "utils.h"
#include <sstream>
#include <map>
#include <chrono>
#include <ctime>
#include <iomanip>

// ── System prompt (Version 3 — final iteration) ──────────────────────────────
// V1: colours returned as words → added "Use exact hex values"
// V2: hex values appeared but format was inconsistent → added format example
// V3: added explicit negative space requirement, constrained output structure
std::string build_system_prompt() {
    return R"(You are a senior visual art director at a world-class creative studio.
Your only task is to read the input and return a complete visual art direction brief.

Rules:
- Be specific. Use exact hex colour values in the format: #1a1a2e  Name  — emotional role
- Name real artists and specific movements (not vague adjectives like "cinematic")
- Use tactile, sensory language for texture
- Be decisive. Do not hedge or qualify
- Do not add preamble, commentary, or questions
- Return ONLY the brief in this exact structure with these exact headers

## COLOUR PALETTE
List exactly 5-7 colours as: #hexvalue  Name  — emotional role

## LIGHTING MOOD
Describe quality, direction, colour temperature (in Kelvin), time of day, feeling.

## COMPOSITION STYLE
Describe framing, rule of thirds or its deliberate violation, symmetry, depth, negative space, orientation.

## ART MOVEMENT REFERENCES
List exactly 2-3 specific movements or artists. One sentence each explaining why each applies.

## TEXTURE AND MATERIAL
Describe surface qualities in tactile, physical language. Name specific materials.

## TONE AND ATMOSPHERE
One short paragraph. The feeling the final work must leave the viewer with.

## WHAT TO AVOID
List specific visual choices that would undermine this direction. Be precise.)";
}

std::string build_user_input(const std::string& text,
                              const std::string& image_caption) {
    std::string input = text;
    if (!image_caption.empty()) {
        input += "\n[Scene: " + image_caption + "]";
    }
    return input;
}

ArtBrief parse_brief(const std::string& raw) {
    ArtBrief brief;
    std::map<std::string, std::string*> sections = {
        {"COLOUR PALETTE",       &brief.colour_palette},
        {"LIGHTING MOOD",        &brief.lighting_mood},
        {"COMPOSITION STYLE",    &brief.composition_style},
        {"ART MOVEMENT",         &brief.art_references},
        {"TEXTURE AND MATERIAL", &brief.texture_material},
        {"TONE AND ATMOSPHERE",  &brief.tone_atmosphere},
        {"WHAT TO AVOID",        &brief.what_to_avoid},
    };

    std::string* current_target = nullptr;
    std::istringstream stream(raw);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.rfind("##", 0) == 0) {
            std::string header = trim(line.substr(2));
            current_target = nullptr;
            for (auto& [key, ptr] : sections) {
                if (header.find(key) != std::string::npos) {
                    current_target = ptr;
                    break;
                }
            }
        } else if (current_target && !trim(line).empty()) {
            if (!current_target->empty()) *current_target += "\n";
            *current_target += trim(line);
        }
    }

    brief.valid = !brief.colour_palette.empty() && !brief.tone_atmosphere.empty();

    // Fallback placeholders
    if (brief.colour_palette.empty())    brief.colour_palette    = "[NOT GENERATED]";
    if (brief.lighting_mood.empty())     brief.lighting_mood     = "[NOT GENERATED]";
    if (brief.composition_style.empty()) brief.composition_style = "[NOT GENERATED]";
    if (brief.art_references.empty())    brief.art_references    = "[NOT GENERATED]";
    if (brief.texture_material.empty())  brief.texture_material  = "[NOT GENERATED]";
    if (brief.tone_atmosphere.empty())   brief.tone_atmosphere   = "[NOT GENERATED]";
    if (brief.what_to_avoid.empty())     brief.what_to_avoid     = "[NOT GENERATED]";

    return brief;
}

std::string format_brief_terminal(const ArtBrief& brief) {
    // Timestamp
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    std::ostringstream ts;
    ts << std::put_time(&tm_buf, "%Y-%m-%d %H:%M");

    std::ostringstream out;
    out << "\n";
    out << "═══════════════════════════════════════════════════\n";
    out << "  FRAME AI — Visual Art Direction Brief\n";
    out << "  Generated: " << ts.str() << "\n";
    out << "═══════════════════════════════════════════════════\n\n";
    out << "COLOUR PALETTE\n"          << brief.colour_palette    << "\n\n";
    out << "LIGHTING MOOD\n"           << brief.lighting_mood     << "\n\n";
    out << "COMPOSITION STYLE\n"       << brief.composition_style << "\n\n";
    out << "ART MOVEMENT REFERENCES\n" << brief.art_references    << "\n\n";
    out << "TEXTURE AND MATERIAL\n"    << brief.texture_material  << "\n\n";
    out << "TONE AND ATMOSPHERE\n"     << brief.tone_atmosphere   << "\n\n";
    out << "WHAT TO AVOID\n"           << brief.what_to_avoid     << "\n\n";
    out << "═══════════════════════════════════════════════════\n";
    return out.str();
}

std::string format_brief_json(const ArtBrief& brief) {
    auto escape = [](const std::string& s) {
        std::string out;
        for (char c : s) {
            if      (c == '"')  out += "\\\"";
            else if (c == '\n') out += "\\n";
            else if (c == '\\') out += "\\\\";
            else                out += c;
        }
        return out;
    };

    std::ostringstream j;
    j << "{\n";
    j << "  \"colour_palette\": \""    << escape(brief.colour_palette)    << "\",\n";
    j << "  \"lighting_mood\": \""     << escape(brief.lighting_mood)     << "\",\n";
    j << "  \"composition_style\": \"" << escape(brief.composition_style) << "\",\n";
    j << "  \"art_references\": \""    << escape(brief.art_references)    << "\",\n";
    j << "  \"texture_material\": \""  << escape(brief.texture_material)  << "\",\n";
    j << "  \"tone_atmosphere\": \""   << escape(brief.tone_atmosphere)   << "\",\n";
    j << "  \"what_to_avoid\": \""     << escape(brief.what_to_avoid)     << "\",\n";
    j << "  \"valid\": " << (brief.valid ? "true" : "false") << "\n";
    j << "}";
    return j.str();
}
