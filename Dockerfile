# Stage 1: Build
FROM alpine:3.21 AS builder

RUN apk add --no-cache build-base cmake openssl-dev linux-headers zlib-dev

WORKDIR /src
COPY . .
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc)

# Stage 2: Runtime
FROM alpine:3.21

RUN apk add --no-cache libssl3 libstdc++ zlib netcat-openbsd

WORKDIR /opt/loginserver
COPY --from=builder /src/build/login ./login
COPY tibia.pem ./tibia.pem
COPY config.cfg.dist ./config.cfg.dist
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

EXPOSE 7171 7172

ENTRYPOINT ["/entrypoint.sh"]
