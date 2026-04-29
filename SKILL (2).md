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
