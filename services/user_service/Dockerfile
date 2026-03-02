FROM debian:bookworm

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    cmake \
    g++ \
    git \
    pkg-config \
    libgrpc++-dev \
    libprotobuf-dev \
    protobuf-compiler \
    protobuf-compiler-grpc \
    default-libmysqlclient-dev \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  && cmake --build build -j

EXPOSE 50051

CMD ["./build/user_service", "--config", "/app/config/user_service.yaml"]
