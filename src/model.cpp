#include "model.h"
#include "llama.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>

struct Model::Impl {
    llama_model*   model   = nullptr;
    llama_context* ctx     = nullptr;
    llama_sampler* sampler = nullptr;
    ModelConfig    config;
};

Model::Model(const ModelConfig& config) : impl(new Impl) {
    impl->config = config;

    llama_model_params mparams = llama_model_default_params();
    impl->model = llama_model_load_from_file(config.model_path.c_str(), mparams);

    if (!impl->model) {
        throw std::runtime_error("Failed to load model: " + config.model_path);
    }

    llama_context_params cparams = llama_context_default_params();
    cparams.n_ctx     = config.context_size;
    cparams.n_threads = config.threads;
    impl->ctx = llama_init_from_model(impl->model, cparams);

    if (!impl->ctx) {
        throw std::runtime_error("Failed to create llama context");
    }

    // Build a greedy sampler chain
    impl->sampler = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(impl->sampler,
        llama_sampler_init_temp(config.temperature));
    llama_sampler_chain_add(impl->sampler,
        llama_sampler_init_greedy());

    if (config.verbose) {
        std::cerr << "[artdir] Model loaded: " << config.model_path << "\n";
    }
}

Model::~Model() {
    if (impl->sampler) llama_sampler_free(impl->sampler);
    if (impl->ctx)     llama_free(impl->ctx);
    if (impl->model)   llama_model_free(impl->model);
    delete impl;
}

bool Model::is_loaded() const {
    return impl->model != nullptr && impl->ctx != nullptr;
}

std::string Model::generate(const std::string& system_prompt,
                              const std::string& user_input) {
    // Build Gemma chat format
    std::string full_prompt =
        "<start_of_turn>user\n" +
        system_prompt + "\n\n" +
        user_input +
        "<end_of_turn>\n"
        "<start_of_turn>model\n";

    const llama_vocab* vocab = llama_model_get_vocab(impl->model);

    // Tokenise
    std::vector<llama_token> tokens(full_prompt.size() + 16);
    int n_tokens = llama_tokenize(
        vocab,
        full_prompt.c_str(),
        (int)full_prompt.size(),
        tokens.data(),
        (int)tokens.size(),
        /*add_special=*/true,
        /*parse_special=*/true
    );

    if (n_tokens < 0) {
        // Retry with larger buffer
        tokens.resize(-n_tokens + 16);
        n_tokens = llama_tokenize(
            vocab,
            full_prompt.c_str(),
            (int)full_prompt.size(),
            tokens.data(),
            (int)tokens.size(),
            true, true
        );
        if (n_tokens < 0) throw std::runtime_error("Tokenisation failed");
    }
    tokens.resize(n_tokens);

    // Decode prompt
    llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size());
    if (llama_decode(impl->ctx, batch) != 0) {
        throw std::runtime_error("Prompt decode failed");
    }

    // Sample loop
    std::string output;
    int n_cur = (int)tokens.size();

    for (int i = 0; i < impl->config.max_tokens; i++) {
        llama_token token = llama_sampler_sample(impl->sampler, impl->ctx, -1);

        if (llama_vocab_is_eog(vocab, token)) break;

        char buf[256];
        int len = llama_token_to_piece(vocab, token, buf, sizeof(buf), 0, true);
        if (len > 0) output.append(buf, len);

        if (impl->config.verbose) {
            std::cerr << buf << std::flush;
        }

        llama_batch next = llama_batch_get_one(&token, 1);
        llama_decode(impl->ctx, next);
        n_cur++;
    }

    // Clear memory and sampler state for the next call
    llama_memory_clear(llama_get_memory(impl->ctx), true);
    llama_sampler_reset(impl->sampler);

    if (impl->config.verbose) {
        std::cerr << "\n[artdir] Generated " << output.size() << " chars\n";
    }

    return output;
}
