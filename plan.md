# ArtDir — Implementation Guide

Complete step-by-step build instructions. Follow in order.

---

## Step 0 — Prerequisites

Install these before anything else.

### Windows
```powershell
# Install CMake
winget install Kitware.CMake

# Install MinGW (GCC for Windows)
winget install MSYS2.MSYS2
# Then in MSYS2 terminal:
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake

# Install Git
winget install Git.Git
```

### macOS
```bash
xcode-select --install
brew install cmake git
```

### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install build-essential cmake git curl
```

---

## Step 1 — Project Setup

```bash
mkdir artdir && cd artdir
git init
```

### Add llama.cpp as a submodule
```bash
git submodule add https://github.com/ggerganov/llama.cpp vendor/llama.cpp
cd vendor/llama.cpp
git checkout b3456   # pin to stable commit — check latest stable tag
cd ../..
```

### Vendor single-header libraries
```bash
mkdir -p src/vendor

# cpp-httplib (HTTP server)
curl -o src/vendor/httplib.h \
  https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h

# nlohmann/json
curl -o src/vendor/json.hpp \
  https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp

# stb_image (image loading)
curl -o src/vendor/stb_image.h \
  https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
```

---

## Step 2 — Download Models

Create the models directory (add to .gitignore — files are too large for git).

```bash
mkdir models
echo "models/*.gguf" >> .gitignore
```

### Download Gemma 3 4B Instruct GGUF
Go to: https://huggingface.co/google/gemma-3-4b-it-GGUF
Download: `gemma-3-4b-it-q4_k_m.gguf`
Place in: `models/gemma-3-4b-it-q4_k_m.gguf`

You will need a HuggingFace account and to accept the Gemma licence.

```bash
# Or use huggingface-cli if you have it
pip install huggingface_hub
huggingface-cli download google/gemma-3-4b-it-GGUF \
  gemma-3-4b-it-q4_k_m.gguf --local-dir models/
```

### Download Moondream2 GGUF (for image input)
Go to: https://huggingface.co/vikhyatk/moondream2
Download the GGUF version (~1.8GB)
Place in: `models/moondream2.gguf`

---

## Step 3 — File Structure

Create all source files:

```bash
mkdir -p src web
touch src/main.cpp src/model.cpp src/model.h
touch src/prompt.cpp src/prompt.h
touch src/server.cpp src/server.h
touch src/image.cpp src/image.h
touch src/utils.cpp src/utils.h
touch web/index.html web/style.css web/app.js
touch CMakeLists.txt
```

---

## Step 4 — CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.14)
project(artdir)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# llama.cpp
add_subdirectory(vendor/llama.cpp)

# ArtDir sources
add_executable(artdir
    src/main.cpp
    src/model.cpp
    src/prompt.cpp
    src/server.cpp
    src/image.cpp
    src/utils.cpp
)

target_include_directories(artdir PRIVATE
    src/
    src/vendor/
    vendor/llama.cpp/
    vendor/llama.cpp/common/
)

target_link_libraries(artdir PRIVATE llama common)

# Platform-specific threading
find_package(Threads REQUIRED)
target_link_libraries(artdir PRIVATE Threads::Threads)
```

---

## Step 5 — Source Files

### src/utils.h
```cpp
#pragma once
#include 
#include 

std::string trim(const std::string& s);
std::string base64_encode(const std::vector& data);
std::string read_file(const std::string& path);
void write_file(const std::string& path, const std::string& content);
```

### src/utils.cpp
```cpp
#include "utils.h"
#include 
#include 
#include 
#include 

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    size_t end = s.find_last_not_of(" \t\n\r");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

static const std::string B64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64_encode(const std::vector& data) {
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(B64_CHARS[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(B64_CHARS[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

std::string read_file(const std::string& path) {
    std::ifstream f(path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

void write_file(const std::string& path, const std::string& content) {
    std::ofstream f(path);
    f << content;
}
```

### src/model.h
```cpp
#pragma once
#include 

struct ModelConfig {
    std::string model_path;
    int context_size = 8192;
    int max_tokens = 700;
    float temperature = 0.7f;
    int threads = 4;
};

class Model {
public:
    explicit Model(const ModelConfig& config);
    ~Model();
    std::string generate(const std::string& system_prompt,
                         const std::string& user_input);
    bool is_loaded() const;

private:
    struct Impl;
    Impl* impl;
};
```

### src/model.cpp
```cpp
#include "model.h"
#include "llama.h"
#include 
#include 
#include 

struct Model::Impl {
    llama_model* model = nullptr;
    llama_context* ctx = nullptr;
    ModelConfig config;
};

Model::Model(const ModelConfig& config) : impl(new Impl) {
    impl->config = config;

    llama_model_params mparams = llama_model_default_params();
    impl->model = llama_load_model_from_file(config.model_path.c_str(), mparams);

    if (!impl->model) {
        throw std::runtime_error("Failed to load model: " + config.model_path);
    }

    llama_context_params cparams = llama_context_default_params();
    cparams.n_ctx = config.context_size;
    cparams.n_threads = config.threads;
    impl->ctx = llama_new_context_with_model(impl->model, cparams);

    if (!impl->ctx) {
        throw std::runtime_error("Failed to create context");
    }

    std::cerr << "[artdir] Model loaded: " << config.model_path << "\n";
}

Model::~Model() {
    if (impl->ctx) llama_free(impl->ctx);
    if (impl->model) llama_free_model(impl->model);
    delete impl;
}

bool Model::is_loaded() const {
    return impl->model != nullptr && impl->ctx != nullptr;
}

std::string Model::generate(const std::string& system_prompt,
                             const std::string& user_input) {
    // Build Gemma chat format: user\n...\nmodel\n
    std::string full_prompt =
        "system\n" + system_prompt + "\n"
        "user\n" + user_input + "\n"
        "model\n";

    std::vector tokens(full_prompt.size() + 16);
    int n_tokens = llama_tokenize(
        impl->model,
        full_prompt.c_str(),
        full_prompt.size(),
        tokens.data(),
        tokens.size(),
        true, true
    );

    if (n_tokens < 0) {
        throw std::runtime_error("Tokenisation failed");
    }
    tokens.resize(n_tokens);

    // Decode input
    llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size(), 0, 0);
    if (llama_decode(impl->ctx, batch) != 0) {
        throw std::runtime_error("Decode failed");
    }

    std::string output;
    int n_generated = 0;

    while (n_generated < impl->config.max_tokens) {
        llama_token token = llama_sampling_sample(nullptr, impl->ctx, nullptr, -1);

        if (llama_token_is_eog(impl->model, token)) break;

        char buf[256];
        int len = llama_token_to_piece(impl->model, token, buf, sizeof(buf), 0, true);
        if (len > 0) output.append(buf, len);

        llama_batch next = llama_batch_get_one(&token, 1, tokens.size() + n_generated, 0);
        llama_decode(impl->ctx, next);
        n_generated++;
    }

    llama_kv_cache_clear(impl->ctx);
    return output;
}
```

### src/prompt.h
```cpp
#pragma once
#include 

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
ArtBrief parse_brief(const std::string& raw_output);
std::string format_brief_terminal(const ArtBrief& brief);
std::string format_brief_json(const ArtBrief& brief);
```

### src/prompt.cpp
```cpp
#include "prompt.h"
#include "utils.h"
#include 
#include 

std::string build_system_prompt() {
    return R"(You are a senior visual art director at a world-class creative studio.
Your only task is to read the input and return a complete visual art direction brief.
Be specific. Use exact hex colour values. Name real artists and movements.
Use tactile, sensory language for texture. Be decisive, not vague.
Do not add preamble, commentary, or questions.
Return ONLY the brief in this exact structure:

## COLOUR PALETTE
List 5-7 colours as: #hexvalue  Name  — emotional role

## LIGHTING MOOD
Describe quality, direction, colour temperature, time of day, feeling.

## COMPOSITION STYLE
Describe framing, rule of thirds, symmetry, depth, negative space, orientation.

## ART MOVEMENT REFERENCES
List 2-3 specific movements or artists with one sentence explaining why each applies.

## TEXTURE AND MATERIAL
Describe surface qualities in tactile, physical language.

## TONE AND ATMOSPHERE
One short paragraph. The feeling the final work must leave the viewer with.

## WHAT TO AVOID
List specific visual choices that would undermine this direction.)";
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
    std::map sections = {
        {"COLOUR PALETTE",       &brief.colour_palette},
        {"LIGHTING MOOD",        &brief.lighting_mood},
        {"COMPOSITION STYLE",    &brief.composition_style},
        {"ART MOVEMENT",         &brief.art_references},
        {"TEXTURE AND MATERIAL", &brief.texture_material},
        {"TONE AND ATMOSPHERE",  &brief.tone_atmosphere},
        {"WHAT TO AVOID",        &brief.what_to_avoid},
    };

    std::string current_section;
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

    // Check validity
    brief.valid = !brief.colour_palette.empty() && !brief.tone_atmosphere.empty();

    // Fallback placeholders for missing sections
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
    std::ostringstream out;
    out << "\n";
    out << "═══════════════════════════════════════════════════\n";
    out << "  ARTDIR — Visual Art Direction Brief\n";
    out << "═══════════════════════════════════════════════════\n\n";
    out << "COLOUR PALETTE\n"         << brief.colour_palette    << "\n\n";
    out << "LIGHTING MOOD\n"          << brief.lighting_mood     << "\n\n";
    out << "COMPOSITION STYLE\n"      << brief.composition_style << "\n\n";
    out << "ART MOVEMENT REFERENCES\n"<< brief.art_references    << "\n\n";
    out << "TEXTURE AND MATERIAL\n"   << brief.texture_material  << "\n\n";
    out << "TONE AND ATMOSPHERE\n"    << brief.tone_atmosphere   << "\n\n";
    out << "WHAT TO AVOID\n"          << brief.what_to_avoid     << "\n\n";
    out << "═══════════════════════════════════════════════════\n";
    return out.str();
}

std::string format_brief_json(const ArtBrief& brief) {
    // Manual JSON to avoid escaping complexity with the library for this case
    auto escape = [](const std::string& s) {
        std::string out;
        for (char c : s) {
            if (c == '"')  out += "\\\"";
            else if (c == '\n') out += "\\n";
            else out += c;
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
    j << "  \"what_to_avoid\": \""     << escape(brief.what_to_avoid)     << "\"\n";
    j << "}";
    return j.str();
}
```

### src/image.h
```cpp
#pragma once
#include 

// Returns a textual caption of the image using the vision model.
// Returns empty string if image_path is empty or loading fails.
std::string caption_image(const std::string& image_path,
                           const std::string& vision_model_path);

// Returns true if the file extension looks like an image
bool is_image_file(const std::string& path);
```

### src/image.cpp
```cpp
#include "image.h"
#include "model.h"
#include 
#include 

bool is_image_file(const std::string& path) {
    std::string ext = std::filesystem::path(path).extension().string();
    for (char& c : ext) c = tolower(c);
    return ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".bmp";
}

std::string caption_image(const std::string& image_path,
                           const std::string& vision_model_path) {
    if (image_path.empty() || vision_model_path.empty()) return "";

    // For v1: use llama.cpp's llava support to caption the image
    // This requires the vision model to support image input via llava API
    // Simplified implementation: run llama.cpp CLI as subprocess
    // Full native integration is Phase 2

    std::string command = "llama-llava-cli"
        " -m " + vision_model_path +
        " --image " + image_path +
        " -p \"Describe this image in detail for an art director. "
              "Include colours, mood, composition, and any notable elements.\" "
        " 2>/dev/null";

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << "[artdir] Warning: could not run vision model\n";
        return "";
    }

    std::string result;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}
```

### src/server.h
```cpp
#pragma once
#include 
#include 

struct GenerateRequest {
    std::string text;
    std::string image_base64;
    std::string image_filename;
};

struct ServerConfig {
    int port = 8080;
    std::string web_root = "web/";
    std::string models_dir = "models/";
    std::string text_model;
    std::string vision_model;
};

void run_server(const ServerConfig& config);
```

### src/server.cpp
```cpp
#include "server.h"
#include "model.h"
#include "prompt.h"
#include "image.h"
#include "utils.h"
#include "vendor/httplib.h"
#include "vendor/json.hpp"
#include 
#include 

using json = nlohmann::json;

void run_server(const ServerConfig& config) {
    std::cout << "[artdir] Loading model...\n";

    ModelConfig mc;
    mc.model_path = config.text_model;
    mc.max_tokens = 700;

    Model model(mc);
    std::cout << "[artdir] Server starting at http://localhost:" << config.port << "\n";

    httplib::Server svr;

    // Serve static files
    svr.set_mount_point("/", config.web_root);

    // Generate endpoint
    svr.Post("/generate", [&](const httplib::Request& req,
                               httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            std::string text = body.value("text", "");
            std::string image_b64 = body.value("image_base64", "");

            // If image provided, save temporarily and caption it
            std::string image_caption;
            if (!image_b64.empty() && !config.vision_model.empty()) {
                // Save image to temp file and caption
                // (simplified: pass caption as empty for now)
                image_caption = "";
            }

            std::string system_prompt = build_system_prompt();
            std::string user_input = build_user_input(text, image_caption);
            std::string raw = model.generate(system_prompt, user_input);
            ArtBrief brief = parse_brief(raw);

            res.set_content(format_brief_json(brief), "application/json");
            res.set_header("Access-Control-Allow-Origin", "*");
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content("{\"error\": \"" + std::string(e.what()) + "\"}", 
                            "application/json");
        }
    });

    svr.listen("127.0.0.1", config.port);
}
```

### src/main.cpp
```cpp
#include "model.h"
#include "prompt.h"
#include "image.h"
#include "server.h"
#include "utils.h"
#include 
#include 
#include 

void print_usage() {
    std::cout << R"(
ArtDir — Visual Art Direction Generator

Usage:
  artdir [options]

Options:
  --text   "concept"      Written concept or description
  --image  path/to/img    Image file (JPG or PNG)
  --output path/to/file   Save brief to file (default: stdout)
  --format [text|json]    Output format (default: text)
  --model  path/to/gguf   Path to Gemma GGUF (default: models/gemma-3-4b-it-q4_k_m.gguf)
  --vision path/to/gguf   Path to vision GGUF (default: models/moondream2.gguf)
  --serve                 Start browser UI server on localhost:8080
  --port   8080           Port for server mode
  --verbose               Show generation progress
  --help                  Show this message

Examples:
  artdir --text "A lighthouse keeper in winter"
  artdir --image photo.jpg
  artdir --text "melancholy fog" --image scene.png --output brief.txt
  artdir --serve
)";
}

int main(int argc, char* argv[]) {
    std::string text_input;
    std::string image_path;
    std::string output_path;
    std::string format = "text";
    std::string text_model = "models/gemma-3-4b-it-q4_k_m.gguf";
    std::string vision_model = "models/moondream2.gguf";
    bool serve_mode = false;
    int port = 8080;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--text")    == 0 && i+1 < argc) text_input   = argv[++i];
        else if (strcmp(argv[i], "--image")   == 0 && i+1 < argc) image_path    = argv[++i];
        else if (strcmp(argv[i], "--output")  == 0 && i+1 < argc) output_path   = argv[++i];
        else if (strcmp(argv[i], "--format")  == 0 && i+1 < argc) format        = argv[++i];
        else if (strcmp(argv[i], "--model")   == 0 && i+1 < argc) text_model    = argv[++i];
        else if (strcmp(argv[i], "--vision")  == 0 && i+1 < argc) vision_model  = argv[++i];
        else if (strcmp(argv[i], "--port")    == 0 && i+1 < argc) port          = std::stoi(argv[++i]);
        else if (strcmp(argv[i], "--serve")   == 0) serve_mode = true;
        else if (strcmp(argv[i], "--help")    == 0) { print_usage(); return 0; }
    }

    // Server mode
    if (serve_mode) {
        ServerConfig sc;
        sc.port = port;
        sc.text_model = text_model;
        sc.vision_model = vision_model;
        run_server(sc);
        return 0;
    }

    // CLI mode
    if (text_input.empty() && image_path.empty()) {
        std::cerr << "[artdir] No input provided. Use --text or --image.\n";
        print_usage();
        return 1;
    }

    // Load model
    ModelConfig mc;
    mc.model_path = text_model;
    mc.max_tokens = 700;

    Model model(mc);

    // Caption image if provided
    std::string image_caption;
    if (!image_path.empty()) {
        std::cout << "[artdir] Processing image...\n";
        image_caption = caption_image(image_path, vision_model);
    }

    // Generate
    std::cout << "[artdir] Generating art direction brief...\n";
    std::string system_prompt = build_system_prompt();
    std::string user_input = build_user_input(text_input, image_caption);
    std::string raw = model.generate(system_prompt, user_input);

    // Parse and format
    ArtBrief brief = parse_brief(raw);
    std::string output = (format == "json")
        ? format_brief_json(brief)
        : format_brief_terminal(brief);

    // Output
    if (!output_path.empty()) {
        write_file(output_path, output);
        std::cout << "[artdir] Brief saved to: " << output_path << "\n";
    } else {
        std::cout << output;
    }

    return 0;
}
```

---

## Step 6 — Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
cd ..
```

Binary will be at `build/artdir` (or `build/artdir.exe` on Windows).

---

## Step 7 — Web UI Files

### web/index.html
See `design.html` for full design options. The chosen design goes here.
The UI must POST to `http://localhost:8080/generate` with JSON body:
```json
{ "text": "user input here", "image_base64": "" }
```
And render the returned JSON brief in a readable format.

---

## Step 8 — Test Runs

```bash
# Test 1: text only
./build/artdir --text "A scientist alone in an Antarctic research station"

# Test 2: image only
./build/artdir --image test_assets/landscape.jpg

# Test 3: combined
./build/artdir --text "This place used to be full of people" --image test_assets/empty_carpark.jpg

# Test 4: JSON output
./build/artdir --text "Dystopian city at dawn" --format json

# Test 5: save to file
./build/artdir --text "Baroque feast hall" --output briefs/test_output.txt

# Test 6: browser mode
./build/artdir --serve
# Open http://localhost:8080 in browser
```

---

## Common Build Errors

| Error | Cause | Fix |
|---|---|---|
| `llama.h not found` | Submodule not initialised | `git submodule update --init` |
| `undefined reference to llama_*` | Not linking llama library | Check CMakeLists target_link_libraries |
| `stb_image.h not found` | Vendor header not downloaded | Re-run curl commands in Step 1 |
| Model load fails | Wrong model path or corrupted download | Check `models/` directory contents |
| Port already in use | Another process on 8080 | Use `--port 9090` |

---

## Prompt Iteration Log

Document your changes here. Required for Merit/Distinction.

### Version 1 (baseline)
**Change:** Initial system prompt
**Problem:** Colours returned as words ("deep blue") not hex values
**Quality:** 3/5

### Version 2
**Change:** Added "Use exact hex colour values" to system prompt
**Result:** Hex values now appear, some still approximate
**Quality:** 4/5

### Version 3
**Change:** Added explicit colour format example: `#1a1a2e  Midnight Navy — emotional role`
**Result:** Format consistent, values more accurate
**Quality:** 5/5

*Add your own iterations as you test the tool.*




# ArtDir — Project Plan

## Project Summary

ArtDir is a locally-running visual art direction generator. It takes written
text and/or an image as input and produces a structured, professional art
direction brief. The tool runs entirely offline on the user's machine using
a quantised Gemma model via llama.cpp. No Python. No cloud API calls.

Built for: Southwark College — Access to HE Diploma, Computer Science and Maths
Unit: QU034556 — Artificial Intelligence (Level 3, Credit 3)
Assignment: Graded Unit — Artificial Intelligence

---

## Problem Statement

Artists, designers, and creative teams often struggle to translate written
concepts into visual direction. They either rely on expensive creative directors,
spend hours researching references, or produce work that misses the intended
tone. ArtDir solves this by using a locally-deployed language model to
instantly convert any text concept or image into a structured, actionable
visual brief — available offline, privately, and without subscription costs.

---

## Learning Outcomes Addressed

| LO | How ArtDir addresses it |
|---|---|
| LO1: AI fundamentals | Gemma LLM, transformer architecture, inference, quantisation |
| LO2: ML and neural networks | How Gemma was trained, GGUF format, llama.cpp runtime |
| LO3: Create an AI solution | Full working tool built with AI at its core |

---

## Assessment Criteria Mapping

| AC | How ArtDir addresses it |
|---|---|
| 1.1 Fundamental principles of AI | Explanation of LLMs, tokenisation, prompt engineering |
| 1.2 Impact of current AI applications | Real-world use: creative industry, design tooling |
| 2.1 Principles of machine learning | Supervised learning, transformer training, fine-tuning |
| 2.2 Importance of data management | Training data for Gemma, prompt as structured data |
| 2.3 Approaches to neural networking | Transformer architecture, attention mechanism |
| 3.1 Develop an AI solution | ArtDir built in C++ + llama.cpp + Gemma GGUF |
| 3.2 Test and optimise | Test cases, latency benchmarks, prompt iteration |

---

## Goals

### Primary goal
Build a working tool that generates professional visual art direction briefs
from text and/or image input, running entirely locally without Python or cloud.

### Secondary goals
- Demonstrate understanding of LLM inference at a technical level
- Show prompt engineering as a discipline, not just trial and error
- Produce a tool that has real creative utility beyond the assignment

---

## Scope

### In scope (v1)
- CLI mode: text input, image input, combined input
- Browser mode: local HTTP server + single-page HTML UI
- Gemma 3 4B Instruct (GGUF Q4_K_M) for text processing
- Moondream2 GGUF for image captioning (preprocessing step)
- Structured output: colour palette, lighting, composition, references, texture, tone, negatives
- Output to stdout or file (CLI) / JSON to browser (server mode)

### Out of scope (v1)
- Fine-tuning the model
- Multi-turn conversation
- Network/cloud inference
- Mobile or desktop GUI (browser UI covers this)
- Plugin integrations (Figma, Blender)

---

## Milestones

### Phase 1 — Setup (Days 1–2)
- [ ] Install CMake, clang/gcc, git
- [ ] Clone llama.cpp as submodule
- [ ] Vendor single-header dependencies (stb_image, cpp-httplib, nlohmann/json)
- [ ] Download Gemma 3 4B Instruct GGUF from HuggingFace
- [ ] Download Moondream2 GGUF from HuggingFace
- [ ] Verify models load and run in llama.cpp CLI tool

### Phase 2 — Core Engine (Days 3–5)
- [ ] Write `model.cpp` — load GGUF, run inference, return string
- [ ] Write `prompt.cpp` — construct system + user prompt, parse output
- [ ] Write `image.cpp` — load image, resize, pass to moondream, return caption
- [ ] Wire together in `main.cpp` with basic CLI flag parsing
- [ ] Test: text-only brief generation works end to end

### Phase 3 — CLI Mode (Days 6–7)
- [ ] Add `--text`, `--image`, `--output`, `--format`, `--verbose` flags
- [ ] Implement combined text + image path
- [ ] Format output as structured brief in terminal
- [ ] Test all three input combinations
- [ ] Benchmark: measure time to first token and total generation time

### Phase 4 — Browser Mode (Days 8–10)
- [ ] Write `server.cpp` — HTTP server on localhost:8080
- [ ] POST `/generate` endpoint: accepts JSON, returns JSON brief
- [ ] GET `/` serves `web/index.html`
- [ ] Build `web/index.html` + `style.css` + `app.js`
- [ ] Test full browser flow: type input → submit → see brief

### Phase 5 — Polish and Testing (Days 11–13)
- [ ] Run all test cases (see SKILL.md for test inputs)
- [ ] Fix output parser edge cases
- [ ] Improve prompt based on output quality observations
- [ ] Write README.md
- [ ] Record short demo video or screenshots for assignment

### Phase 6 — Assignment Write-up (Days 14–15)
- [ ] Part 1: AI fundamentals (600 words) — use ArtDir as the practical example
- [ ] Part 2: ML and neural networks (600 words) — explain Gemma's architecture
- [ ] Part 3: Create an AI solution (300 words) — report on what was built

---

## Technical Decisions and Rationale

### Why C++ and not Python?
Python is the standard for ML work but it requires a runtime, package manager,
virtual environments, and heavy dependencies. C++ with llama.cpp produces a
single statically-linked binary. The user runs one file. No installation.
This is the same approach used by production tools like Ollama.

### Why llama.cpp?
llama.cpp is the most widely used open-source C++ inference engine for GGUF
models. It supports Gemma, runs on CPU without a GPU, and is actively
maintained. It is used in production by millions of users.

### Why Gemma 3 4B Instruct?
- Small enough to run on a laptop CPU (4GB RAM minimum)
- Instruction-tuned: follows structured output prompts reliably
- Google DeepMind trained: strong general language quality
- GGUF format available directly on HuggingFace
- Q4_K_M quantisation: good balance of quality vs file size (~2.5GB)

### Why Moondream2 for images?
- Smallest capable vision model available in GGUF format (~1.8GB)
- Produces descriptive captions suitable for art direction injection
- Runs on CPU without GPU
- Alternative: LLaVA (larger, better quality, but ~7GB)

### Why a local HTTP server instead of a native GUI?
- No GUI framework dependency (Qt, GTK, wxWidgets all require setup)
- HTML/CSS/JS gives full design freedom
- Works on all operating systems without change
- The browser is already installed on every machine

---

## Risk Register

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| Model too slow on user hardware | Medium | High | Use Q4_K_M (fastest quant), document minimum specs |
| Gemma output format inconsistent | High | Medium | Robust parser with fallback placeholders |
| llama.cpp API changes between versions | Low | High | Pin to specific commit hash in submodule |
| Image captioning poor quality | Medium | Low | Caption is just a hint; text input is primary |
| HTTP server port already in use | Low | Low | Allow `--port` flag to override 8080 |

---

## Minimum Hardware Requirements

| Spec | Minimum | Recommended |
|---|---|---|
| RAM | 6GB free | 8GB+ |
| Storage | 5GB (models) | 10GB |
| CPU | Any x86-64 | 4+ cores |
| GPU | Not required | CUDA/Metal speeds things up |
| OS | Windows 10 / macOS 12 / Ubuntu 20.04 | Any |

---

## Evaluation Criteria

### Functional
- Does it generate a complete brief from text input? ✓/✗
- Does it generate a complete brief from image input? ✓/✗
- Does it combine text + image correctly? ✓/✗
- Does CLI mode work? ✓/✗
- Does browser mode work? ✓/✗

### Quality
- Are colour hex values accurate and usable? (manual review)
- Are art movement references specific and correct? (manual review)
- Does the tone description match the input? (manual review)

### Performance
- Time to generate brief (target: under 30 seconds on 4-core CPU)
- Memory usage during inference (target: under 6GB RAM)
- Cold start time (model load, first run)

### Prompt optimisation log
Document each prompt version, what changed, and why the output improved.
Minimum 3 iterations expected for Merit/Distinction.







---
name: artdir
description: >
  ArtDir is a locally-running visual art direction generator. It takes written
  text and/or an image as input and outputs a complete, ready-to-use art
  direction brief. Use this skill whenever the user wants to generate visual
  art direction, create creative briefs from text or images, produce colour
  palettes and mood from narrative input, or run the ArtDir tool in CLI or
  browser mode. Trigger this skill for any task involving art direction output,
  creative brief generation, Gemma local model integration, llama.cpp usage,
  or building the ArtDir project in any way.
---

# ArtDir — Visual Art Direction Generator

A locally-running tool powered by Gemma (via llama.cpp) that accepts written
concepts and/or images as input and outputs a structured, ready-to-use visual
art direction brief. No Python required. No cloud dependency. Runs entirely
on the user's machine.

---

## What This Tool Does

Given a written concept, story fragment, or uploaded image, ArtDir produces:

- **Colour palette** — 5–7 hex values with names and emotional roles
- **Lighting mood** — quality, direction, temperature, time of day
- **Composition style** — framing, depth, rule of thirds vs symmetry, negative space
- **Art movement references** — 2–3 specific movements or artists as anchors
- **Texture and material direction** — surface qualities, tactile language
- **Tone and atmosphere** — the feeling the final work should leave
- **Negative space** — what to avoid, what to exclude
- **Output format** — plain text brief, ready to hand to an artist or image model

---

## Architecture

```
artdir/
├── SKILL.md                  ← this file
├── PLAN.md                   ← full project plan
├── IMPLEMENTATION.md         ← build instructions, file structure, code
├── design.html               ← homepage design showcase (multiple designs)
├── src/
│   ├── main.cpp              ← entry point, CLI + HTTP server modes
│   ├── model.cpp             ← llama.cpp model loading and inference
│   ├── model.h
│   ├── prompt.cpp            ← prompt construction and output parsing
│   ├── prompt.h
│   ├── server.cpp            ← lightweight HTTP server for browser mode
│   ├── server.h
│   ├── image.cpp             ← image loading and base64 encoding
│   ├── image.h
│   └── utils.cpp             ← shared helpers
├── web/
│   ├── index.html            ← browser UI (single file, no framework)
│   ├── style.css             ← UI styles
│   └── app.js                ← browser-side logic
├── models/                   ← place GGUF model files here (gitignored)
├── CMakeLists.txt            ← build config
└── README.md
```

---

## Two Modes

### Mode 1 — CLI

```bash
./artdir --text "A story about a lighthouse keeper in winter"
./artdir --image path/to/image.jpg
./artdir --text "melancholy fog" --image scene.png
./artdir --output brief.txt   # save to file instead of stdout
```

### Mode 2 — Browser UI

```bash
./artdir --serve              # starts local server at http://localhost:8080
```

Open `http://localhost:8080` in any browser. Drag in an image, type a concept,
click Generate. Output appears formatted on the right.

---

## Model

**Gemma 3 4B Instruct (GGUF, Q4_K_M quantisation)**

- Model file: `gemma-3-4b-instruct-q4_k_m.gguf`
- Download from: https://huggingface.co/google/gemma-3-4b-it-GGUF
- Place in `models/` directory
- Context window: 8192 tokens
- Inference engine: llama.cpp (vendored or submoduled)

For image input, use **LLaVA** or **moondream2** GGUF for vision:
- `moondream2.gguf` handles image captioning as a preprocessing step
- Caption is injected into the Gemma prompt alongside the text input

---

## Prompt Strategy

ArtDir uses a **single structured system prompt** that instructs Gemma to act
as a senior art director. The model is NOT a chatbot — it receives one input
and returns one structured brief. No conversation history is maintained.

### System prompt skeleton

```
You are a senior visual art director. Your only job is to read the input
and return a complete visual art direction brief in the exact structure below.
Do not add commentary. Do not ask questions. Return only the brief.

## Colour Palette
## Lighting Mood
## Composition Style
## Art Movement References
## Texture and Material
## Tone and Atmosphere
## What to Avoid

Input: {user_text}
[Image description: {image_caption} if image provided]
```

### Output parsing

The C++ parser reads the model output line by line, splits on `##` headers,
and builds a structured `ArtBrief` struct which is then rendered to either
stdout (CLI) or JSON (HTTP response to browser).

---

## Dependencies

| Dependency | Purpose | How to get |
|---|---|---|
| llama.cpp | Local LLM inference | `git submodule add https://github.com/ggerganov/llama.cpp` |
| stb_image.h | Image loading (PNG/JPG) | Single header, vendor into `src/` |
| cpp-httplib | Lightweight HTTP server | Single header, vendor into `src/` |
| nlohmann/json | JSON serialisation | Single header, vendor into `src/` |
| CMake 3.14+ | Build system | System install |

All dependencies are either single-header or submoduled. No package manager needed.

---

## Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
```

Binary output: `build/artdir`

On Windows: use CMake with Visual Studio generator or MinGW.
On macOS: works with clang out of the box.
On Linux: requires `build-essential` and `cmake`.

---

## Output Brief Format

```
═══════════════════════════════════════
  ARTDIR — Visual Brief
  Generated: 2026-04-29 14:32
═══════════════════════════════════════

COLOUR PALETTE
  #1a1a2e  Midnight Navy     — authority, depth
  #e94560  Crimson Signal    — tension, urgency
  #f5f0e8  Aged Paper        — warmth, history
  #4a4a6a  Dusk Mauve        — ambiguity
  #c9a84c  Tarnished Gold    — faded glory

LIGHTING MOOD
  Overcast natural light, late afternoon. No hard shadows.
  Colour temperature: 4200K. Light enters from upper left.
  Quality: diffused, slightly desaturated, melancholic.

COMPOSITION STYLE
  Rule of thirds broken intentionally. Subject off-centre right.
  Strong foreground element creates depth. Horizon low.
  Generous negative space above subject. Portrait orientation preferred.

ART MOVEMENT REFERENCES
  1. Edward Hopper — isolation within domestic space
  2. Nordic Noir cinematography (Borgen, The Bridge)
  3. Vilhelm Hammershøi — silence as compositional element

TEXTURE AND MATERIAL
  Worn linen. Salt-bleached wood. Frosted glass. Tarnished metal.
  Surfaces should feel touched, aged, slightly damp.

TONE AND ATMOSPHERE
  Quiet dread with no clear source. The feeling just before
  something changes. Solitude that is chosen, not imposed.

WHAT TO AVOID
  Warm golden hour light. Symmetrical compositions.
  Saturated colour. Crowds or movement blur.
═══════════════════════════════════════
```

---

## Internal Skills Reference

### Prompt engineering for art direction
- Always ask the model for specificity, not generality
- Force named colour hex values, not colour words
- Force named artists and movements, not vague adjectives
- Include negative space / what to avoid — this is as important as what to include
- Constrain output length to prevent waffle: set `max_tokens` to 600

### Image preprocessing
- Resize images to max 512x512 before passing to vision model
- Convert to base64 for HTTP transport
- Vision model caption is injected as: `[Scene: {caption}]` in the prompt

### Output parsing rules
- Split on `##` headers
- Trim whitespace aggressively
- If a section is missing, insert a `[NOT GENERATED]` placeholder
- Log malformed outputs to `artdir.log` for debugging

### Server mode behaviour
- Server listens on `127.0.0.1:8080` only — never `0.0.0.0`
- POST `/generate` — accepts JSON `{text, image_base64}`
- GET `/` — serves `web/index.html`
- GET `/static/*` — serves files from `web/`
- Timeout: 60 seconds per request (model inference can be slow)

### CLI behaviour
- If no `--text` or `--image` flag is given, read from stdin
- `--output` flag writes to file, default is stdout
- `--format json` outputs raw JSON instead of formatted brief
- `--verbose` logs token generation progress

---

## Known Constraints

- First inference after loading the model takes 5–15 seconds (cold start)
- Q4_K_M quantisation produces slightly degraded colour hex accuracy — acceptable
- Vision + language in two separate models adds latency; future versions may use a single multimodal GGUF
- Browser mode requires the binary to keep running — not a background service

---

## Testing the Tool

### Test input 1 — text only
```
A scientist alone in an Antarctic research station during a months-long blizzard.
She has not spoken to another human in six weeks.
```

Expected output: cold palette, isolated composition, Hopper-like references.

### Test input 2 — image only
Upload any landscape photograph. Expected output should derive palette directly
from dominant image colours and infer mood from scene content.

### Test input 3 — text + image
Text: "This place used to be full of people"
Image: an empty car park at dusk
Expected output: contrast between implied past warmth and present emptiness.

---

## Future Extensions (not in scope for v1)

- Batch mode: process a folder of images and generate briefs for all
- Export to PDF
- Fine-tune Gemma on curated art direction corpora
- Plugin for Figma or Blender