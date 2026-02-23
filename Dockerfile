# Build stage
FROM debian:bullseye-slim AS build
RUN apt-get update && apt-get install -y g++ make cmake libssl-dev
WORKDIR /app
COPY . .
# Linking order is extremely important for g++
RUN g++ -O3 main.cc -o carts-server -DCPPHTTPLIB_OPENSSL_SUPPORT -lpthread -lssl -lcrypto

# Run stage
FROM debian:bullseye-slim
RUN apt-get update && apt-get install -y libssl1.1 ca-certificates && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=build /app/carts-server .
EXPOSE 8080
CMD ["./carts-server"]
