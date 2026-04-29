FROM ubuntu:22.04

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    wget \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy project files
COPY . .

# Download the Gemma 3 4B model (using Q4_K_M for speed/memory balance)
RUN mkdir -p models && \
    wget -q https://huggingface.co/google/gemma-3-4b-it-GGUF/resolve/main/gemma-3-4b-it-Q4_K_M.gguf -O models/gemma-3-4b-it.gguf

# Build the C++ server
RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build --config Release -j4

# Expose the port
EXPOSE 8080

# Run the server bound to all interfaces
CMD ["./build/frame-ai", "--serve", "--model", "models/gemma-3-4b-it.gguf", "--host", "0.0.0.0", "--port", "8080"]
