# Build stage
FROM debian:bullseye-slim AS build

# Install compiler and SSL libraries
RUN apt-get update && apt-get install -y g++ make cmake libssl-dev pkg-config

WORKDIR /app
COPY . .

# 1. We use -std=c++17 because modern JSON and Networking libraries need it.
# 2. -DCPPHTTPLIB_OPENSSL_SUPPORT tells the code to enable HTTPS.
# 3. We link ssl, crypto, and pthread at the very end.
RUN g++ -std=c++17 -O3 main.cc -o carts-server -DCPPHTTPLIB_OPENSSL_SUPPORT -lssl -lcrypto -lpthread

# Run stage
FROM debian:bullseye-slim
# Install runtime SSL support
RUN apt-get update && apt-get install -y libssl1.1 ca-certificates && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /app/carts-server .

EXPOSE 8080

# Run the server
CMD ["./carts-server"]
