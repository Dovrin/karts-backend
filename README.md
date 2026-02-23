# CARTS Backend (C++)

This is the C++ backend for the Calendar & Announcement Real-Time System (CARTS).

## 🚀 Features
- High-performance C++ server.
- JSON-based local storage (auto-initializing).
- REST API for events, announcements, and admin login.
- Render.com and Docker ready.

## 🛠️ Setup
1. **Prerequisites**: C++ Compiler (GCC/Clang) and CMake.
2. **Build**:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```
3. **Run**: `./carts-server`
4. **Ports**: Defaults to `8080`.

## 🐳 Docker Deployment
```bash
docker build -t carts-backend .
docker run -p 8080:8080 carts-backend
```
