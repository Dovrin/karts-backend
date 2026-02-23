# Build stage
FROM debian:bullseye-slim AS build
RUN apt-get update && apt-get install -y g++ make cmake
WORKDIR /app
COPY . .
RUN g++ -O3 main.cc -o carts-server -lpthread

# Run stage
FROM debian:bullseye-slim
WORKDIR /app
COPY --from=build /app/carts-server .
EXPOSE 8080
CMD ["./carts-server"]
