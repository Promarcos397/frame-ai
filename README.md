---
title: Frame AI
emoji: 🖼️
colorFrom: gray
colorTo: blue
sdk: docker
app_port: 8080
pinned: false
---

# Frame AI — Visual Art Direction Generator

A locally-running visual art direction tool powered by **Gemma 3** via **llama.cpp**.

Takes written text and/or an image and produces a complete, structured creative brief. Runs entirely offline. No Python. No cloud API calls.

---

## What it generates

Given a concept or image, Frame AI produces:

- **Colour palette** — 5–7 hex values with names and emotional roles  
- **Lighting mood** — quality, direction, temperature, time of day  
- **Composition style** — framing, depth, negative space  
- **Art movement references** — 2–3 specific artists or movements  
- **Texture and material** — tactile, sensory language  
- **Tone and atmosphere** — the feeling the work must leave  
- **What to avoid** — precise visual exclusions  

---

## Requirements

- CMake 3.14+
- C++17 compiler (MSVC, GCC, or Clang)
- [Gemma 3 4B GGUF](https://huggingface.co/google/gemma-3-4b-it-GGUF) — download `gemma-3-4b-it-q4_k_m.gguf`
- (Optional) [Moondream2 GGUF](https://huggingface.co/vikhyatk/moondream2) for image input

### Minimum hardware

| Spec | Minimum |
|------|---------|
| RAM  | 6 GB free |
| Storage | 5 GB (models) |
| CPU  | Any x86-64 |

---

## Setup

### 1. Clone and initialise submodule

```bash
git clone <this-repo> frame-ai
cd frame-ai
git submodule update --init --recursive
```

### 2. Download the model

Place in the `models/` directory:

```bash
pip install huggingface_hub
huggingface-cli download google/gemma-3-4b-it-GGUF \
  gemma-3-4b-it-q4_k_m.gguf --local-dir models/
```

### 3. Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release -j4
cd ..
```

---

## Usage

### CLI mode

```bash
# Text only
./build/frame-ai --text "A lighthouse keeper in winter, alone, fog so thick the sea disappears."

# Image only (requires vision model)
./build/frame-ai --image photo.jpg

# Text + image
./build/frame-ai --text "This place used to be full of people" --image empty_carpark.jpg

# Save to file
./build/frame-ai --text "Baroque feast hall" --output brief.txt

# JSON format
./build/frame-ai --text "Dystopian city at dawn" --format json

# Verbose (shows generation progress)
./build/frame-ai --text "Antarctic silence" --verbose
```

### Browser mode

```bash
./build/frame-ai --serve
# Open http://localhost:8080 in your browser
```

### All options

```
--text   "concept"      Written concept or description
--image  path/img       Image file (JPG, PNG, BMP)
--output path/file      Save brief to file (default: stdout)
--format [text|json]    Output format (default: text)
--model  path/gguf      Path to Gemma GGUF
--vision path/gguf      Path to vision model GGUF
--serve                 Start browser UI server
--port   N              Port for server mode (default: 8080)
--threads N             CPU threads (default: 4)
--verbose               Show generation progress
--help                  Show usage
```

---

## Project structure

```
frame-ai/
├── src/
│   ├── main.cpp      — entry point, CLI + server modes
│   ├── model.cpp     — llama.cpp loading and inference
│   ├── prompt.cpp    — prompt construction and output parsing
│   ├── server.cpp    — HTTP server for browser mode
│   ├── image.cpp     — image captioning via llava subprocess
│   └── utils.cpp     — shared helpers
├── web/
│   ├── index.html    — browser UI
│   ├── style.css     — UI styles
│   └── app.js        — browser logic
├── models/           — place GGUF files here (gitignored)
├── vendor/
│   └── llama.cpp/    — inference engine (submodule)
└── CMakeLists.txt
```

---

## Built for

Southwark College — Access to HE Diploma, Computer Science and Maths  
Unit: QU034556 — Artificial Intelligence (Level 3, Credit 3)
