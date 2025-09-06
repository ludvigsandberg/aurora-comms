########################################
# Stage 1: build (debug/release)
########################################
FROM ubuntu:24.04 AS builder

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    clang \
    cmake \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# Default build type is Release, can be overridden at build time
ARG BUILD_TYPE=Release
ENV CC=clang

# Build the project
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=$BUILD_TYPE && \
    cmake --build build -- -j$(nproc)

########################################
# Stage 2: runtime (release)
########################################
FROM ubuntu:24.04 AS release

WORKDIR /app
COPY --from=builder /app/build/server .

CMD ["./server"]

########################################
# Stage 3: runtime (debug)
########################################
FROM ubuntu:24.04 AS debug

RUN apt-get update && apt-get install -y \
    gdb \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy built binary + source code for debugging
COPY --from=builder /app/build/server . 
COPY --from=builder /app/src ./src
COPY --from=builder /app/inc ./inc

CMD ["./server"]

########################################
# Stage 4: test (Ceedling)
########################################
FROM ubuntu:24.04 AS test

RUN apt-get update && apt-get install -y \
    ruby ruby-dev build-essential clang cmake make git \
    && gem install ceedling \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

ENV CC=clang
ARG BUILD_TYPE=Debug

# Default command: run tests
CMD ["ceedling", "test:all"]