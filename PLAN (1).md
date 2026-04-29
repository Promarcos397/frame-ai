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
