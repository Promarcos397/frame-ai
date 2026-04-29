#pragma once
#include <string>
#include <memory>

struct GenerateRequest {
    std::string text;
    std::string image_base64;
    std::string image_filename;
};

struct ServerConfig {
    int         port        = 8080;
    std::string web_root    = "web/";
    std::string models_dir  = "models/";
    std::string text_model;
    std::string vision_model;
    bool        verbose     = false;
};

void run_server(const ServerConfig& config);
