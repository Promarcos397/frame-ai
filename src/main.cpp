#include "model.h"
#include "prompt.h"
#include "image.h"
#include "server.h"
#include "utils.h"
#include <iostream>
#include <cstring>
#include <string>

// ── Usage ─────────────────────────────────────────────────────────────────────
static void print_usage() {
    std::cout << R"(
Frame AI — Visual Art Direction Generator
Powered by Gemma 3 4B (via llama.cpp). Runs entirely offline.

Usage:
  frame-ai [options]

Options:
  --text   "concept"      Written concept or description
  --image  path/to/img    Image file (JPG or PNG) — requires vision model
  --output path/to/file   Save brief to file (default: stdout)
  --format [text|json]    Output format (default: text)
  --model  path/to/gguf   Path to Gemma GGUF
                          (default: models/gemma-3-4b-it-q4_k_m.gguf)
  --vision path/to/gguf   Path to vision model GGUF
                          (default: models/moondream2.gguf)
  --serve                 Start browser UI server on localhost:8080
  --host   IP             Host IP for server mode (default: 127.0.0.1)
  --port   N              Port for server mode (default: 8080)
  --threads N             CPU threads to use (default: 4)
  --verbose               Show token generation progress
  --help                  Show this message

Examples:
  frame-ai --text "A lighthouse keeper in winter"
  frame-ai --image photo.jpg
  frame-ai --text "melancholy fog" --image scene.png --output brief.txt
  frame-ai --text "Dystopian city at dawn" --format json
  frame-ai --serve
  frame-ai --serve --port 9090

Notes:
  - Model must be placed in models/ or specified with --model
  - Download Gemma 3 4B from https://huggingface.co/google/gemma-3-4b-it-GGUF
  - Download Moondream2 from https://huggingface.co/vikhyatk/moondream2
  - Generation takes 15-60s on CPU (cold start included)
)";
}

// ── Entry point ───────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    std::string text_input;
    std::string image_path;
    std::string output_path;
    std::string format       = "text";
    std::string text_model   = "models/gemma-3-4b-it-q4_k_m.gguf";
    std::string vision_model = "models/moondream2.gguf";
    bool serve_mode = false;
    bool verbose    = false;
    std::string host = "127.0.0.1";
    int  port       = 8080;
    int  threads    = 4;

    // ── Parse arguments ───────────────────────────────────────────────────────
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if      (arg == "--text"    && i+1 < argc) text_input   = argv[++i];
        else if (arg == "--image"   && i+1 < argc) image_path   = argv[++i];
        else if (arg == "--output"  && i+1 < argc) output_path  = argv[++i];
        else if (arg == "--format"  && i+1 < argc) format       = argv[++i];
        else if (arg == "--model"   && i+1 < argc) text_model   = argv[++i];
        else if (arg == "--vision"  && i+1 < argc) vision_model = argv[++i];
        else if (arg == "--host"    && i+1 < argc) host         = argv[++i];
        else if (arg == "--port"    && i+1 < argc) port         = std::stoi(argv[++i]);
        else if (arg == "--threads" && i+1 < argc) threads      = std::stoi(argv[++i]);
        else if (arg == "--serve")    serve_mode = true;
        else if (arg == "--verbose")  verbose    = true;
        else if (arg == "--help")   { print_usage(); return 0; }
        else {
            std::cerr << "[frame-ai] Unknown argument: " << arg << "\n";
            print_usage();
            return 1;
        }
    }

    // ── Server mode ───────────────────────────────────────────────────────────
    if (serve_mode) {
        ServerConfig sc;
        sc.host         = host;
        sc.port         = port;
        sc.text_model   = text_model;
        sc.vision_model = vision_model;
        sc.verbose      = verbose;
        run_server(sc);
        return 0;
    }

    // ── CLI mode ──────────────────────────────────────────────────────────────
    if (text_input.empty() && image_path.empty()) {
        // Fallback: read from stdin
        std::cerr << "[frame-ai] No --text or --image provided. Reading from stdin...\n";
        std::string line;
        while (std::getline(std::cin, line)) {
            if (!text_input.empty()) text_input += '\n';
            text_input += line;
        }
        if (text_input.empty()) {
            std::cerr << "[frame-ai] No input. Use --text, --image, or pipe text to stdin.\n";
            print_usage();
            return 1;
        }
    }

    // ── Load model ────────────────────────────────────────────────────────────
    std::cerr << "[frame-ai] Loading model: " << text_model << "\n";
    ModelConfig mc;
    mc.model_path = text_model;
    mc.max_tokens = 700;
    mc.threads    = threads;
    mc.verbose    = verbose;

    Model model(mc);
    std::cerr << "[frame-ai] Model ready.\n";

    // ── Caption image if provided ─────────────────────────────────────────────
    std::string image_caption;
    if (!image_path.empty()) {
        std::cerr << "[frame-ai] Processing image: " << image_path << "\n";
        image_caption = caption_image(image_path, vision_model);
        if (image_caption.empty()) {
            std::cerr << "[frame-ai] Warning: image captioning failed — "
                         "proceeding with text only\n";
        }
    }

    // ── Generate ──────────────────────────────────────────────────────────────
    std::cerr << "[frame-ai] Generating art direction brief...\n";
    std::string system_prompt = build_system_prompt();
    std::string user_input    = build_user_input(text_input, image_caption);
    std::string raw           = model.generate(system_prompt, user_input);

    // ── Parse and format ──────────────────────────────────────────────────────
    ArtBrief brief = parse_brief(raw);

    if (!brief.valid) {
        std::cerr << "[frame-ai] Warning: output may be incomplete — "
                     "check raw output with --verbose\n";
    }

    std::string output = (format == "json")
        ? format_brief_json(brief)
        : format_brief_terminal(brief);

    // ── Write output ──────────────────────────────────────────────────────────
    if (!output_path.empty()) {
        write_file(output_path, output);
        std::cerr << "[frame-ai] Brief saved to: " << output_path << "\n";
    } else {
        std::cout << output;
    }

    return 0;
}
