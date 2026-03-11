# Stage 1: Build
FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ make libssl-dev linux-libc-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .
RUN make clean && make -j$(nproc)

# Stage 2: Runtime
FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 netcat-openbsd \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/loginserver
COPY --from=builder /src/build/login ./login
COPY tibia.pem ./tibia.pem
COPY config.cfg.dist ./config.cfg.dist

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

EXPOSE 7171

ENTRYPOINT ["/entrypoint.sh"]
