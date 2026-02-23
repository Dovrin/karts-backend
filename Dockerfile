# Build stage
FROM debian:bullseye-slim AS build

# Install C++ compiler
RUN apt-get update && apt-get install -y g++ make cmake

WORKDIR /app
COPY . .

# Compile C++ backend (No SSL flags needed now, solving the build error!)
RUN g++ -std=c++17 -O3 main.cc -o carts-server -lpthread

# Run stage
FROM debian:bullseye-slim

# Install Python and Requests for cloud sync
RUN apt-get update && apt-get install -y python3 python3-requests ca-certificates && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /app/carts-server .
COPY supabase_sync.py .

EXPOSE 8080

CMD ["./carts-server"]
