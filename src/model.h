#pragma once
#include <string>

struct ModelConfig {
    std::string model_path;
    int   context_size = 8192;
    int   max_tokens   = 700;
    float temperature  = 0.7f;
    int   threads      = 4;
    bool  verbose      = false;
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
