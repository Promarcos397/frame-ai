#include "server.h"
#include "model.h"
#include "prompt.h"
#include "image.h"
#include "utils.h"
#include "vendor/httplib.h"
#include "vendor/json.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <mutex>

using json = nlohmann::json;

// ── Save base64 image to a temp file and return its path ─────────────────────
static std::string save_temp_image(const std::string& b64, const std::string& filename) {
    // Decode base64 — strip data URI header if present
    std::string raw_b64 = b64;
    auto comma = raw_b64.find(',');
    if (comma != std::string::npos) raw_b64 = raw_b64.substr(comma + 1);

    // Determine extension from original filename or default to .jpg
    std::string ext = ".jpg";
    if (!filename.empty()) {
        auto dot = filename.rfind('.');
        if (dot != std::string::npos) ext = filename.substr(dot);
    }

    std::string tmp_path = std::filesystem::temp_directory_path().string()
                         + "/artdir_upload" + ext;

    // Decode base64
    const std::string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<unsigned char> out;
    int val = 0, valb = -8;
    for (unsigned char c : raw_b64) {
        if (c == '=') break;
        auto pos = chars.find((char)c);
        if (pos == std::string::npos) continue;
        val = (val << 6) + (int)pos;
        valb += 6;
        if (valb >= 0) {
            out.push_back((unsigned char)((val >> valb) & 0xFF));
            valb -= 8;
        }
    }

    std::ofstream f(tmp_path, std::ios::binary);
    f.write(reinterpret_cast<const char*>(out.data()), (std::streamsize)out.size());
    return tmp_path;
}

// ─────────────────────────────────────────────────────────────────────────────
void run_server(const ServerConfig& config) {
    std::cout << "[artdir] Loading model: " << config.text_model << "\n";

    ModelConfig mc;
    mc.model_path = config.text_model;
    mc.max_tokens = 700;
    mc.verbose    = config.verbose;

    auto model = std::make_unique<Model>(mc);
    std::mutex model_mutex;   // llama.cpp contexts are not thread-safe

    std::cout << "[artdir] Model ready.\n";
    std::cout << "[artdir] Server running at http://localhost:" << config.port << "\n";
    std::cout << "[artdir] Open your browser and go to http://localhost:" << config.port << "\n";

    httplib::Server svr;

    // ── Serve static web files ──
    if (!svr.set_mount_point("/", config.web_root)) {
        std::cerr << "[artdir] Warning: web root not found: " << config.web_root
                  << " — static files will not be served\n";
    }

    // ── POST /generate ──────────────────────────────────────────────────────
    svr.Post("/generate", [&](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");

        try {
            auto body        = json::parse(req.body);
            std::string text = body.value("text", "");
            std::string b64  = body.value("image_base64", "");
            std::string fname= body.value("image_filename", "");

            if (text.empty() && b64.empty()) {
                res.status = 400;
                res.set_content("{\"error\":\"No text or image provided\"}", "application/json");
                return;
            }

            std::string image_caption;
            std::string tmp_path;

            if (!b64.empty() && !config.vision_model.empty()) {
                tmp_path     = save_temp_image(b64, fname);
                image_caption= caption_image(tmp_path, config.vision_model);
            }

            std::string system_prompt = build_system_prompt();
            std::string user_input    = build_user_input(text, image_caption);

            std::string raw;
            {
                std::lock_guard<std::mutex> lock(model_mutex);
                raw = model->generate(system_prompt, user_input);
            }

            ArtBrief brief = parse_brief(raw);
            res.set_content(format_brief_json(brief), "application/json");

            // Clean up temp file
            if (!tmp_path.empty()) {
                std::error_code ec;
                std::filesystem::remove(tmp_path, ec);
            }

        } catch (const std::exception& e) {
            res.status = 500;
            std::string err = std::string("{\"error\":\"") + e.what() + "\"}";
            res.set_content(err, "application/json");
        }
    });

    // ── OPTIONS /generate (CORS preflight) ──────────────────────────────────
    svr.Options("/generate", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 204;
    });

    // ── GET /health ──────────────────────────────────────────────────────────
    svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });

    svr.listen("127.0.0.1", config.port);
}
