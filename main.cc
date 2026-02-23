#ifdef _WIN32
  #ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0A00
  #endif
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <array>
#include <ctime>
#include "httplib.h"
#include "json.hpp"

using json = nlohmann::json;

// --- STEP 1: THE PYTHON BRIDGE CAPTURE ---
std::string run_python_get(const std::string& table) {
    std::string command = "python3 supabase_sync.py GET " + table;
    std::array<char, 4096> buffer;
    std::string result;
    
    #ifdef _WIN32
    FILE* pipe = _popen(command.c_str(), "r");
    #else
    FILE* pipe = popen(command.c_str(), "r");
    #endif

    if (!pipe) return "[]";
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    #ifdef _WIN32
    _pclose(pipe);
    #else
    pclose(pipe);
    #endif
    return result;
}

// For POST: We write the body to a temp file and tell python to read it
void run_python_post(const std::string& table, const std::string& body) {
    // Save body to temp file to avoid shell escape issues
    std::ofstream ofs("temp_payload.json");
    ofs << body;
    ofs.close();

    std::string command = "python3 supabase_sync.py POST " + table + " < temp_payload.json";
    std::system(command.c_str());
    #ifdef _WIN32
    std::system("del temp_payload.json");
    #else
    std::system("rm temp_payload.json");
    #endif
}

void run_python_delete(const std::string& table, const std::string& id) {
    std::string command = "python3 supabase_sync.py DELETE " + table + " " + id;
    std::system(command.c_str());
}

int main() {
    httplib::Server svr;

    // --- CORS ---
    svr.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, GET, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        if (req.method == "OPTIONS") {
            res.status = 204;
            return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });

    // --- LOGIN ---
    svr.Post("/api/login", [](const httplib::Request& req, httplib::Response& res) {
        auto j_req = json::parse(req.body);
        if (j_req["username"] == "admin" && j_req["password"] == "admin123") {
            res.set_content("{\"status\":\"success\",\"token\":\"secure\",\"role\":\"admin\"}", "application/json");
        } else {
            res.status = 401;
        }
    });

    // --- API ROUTES ---
    svr.Get("/api/events", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content(run_python_get("events"), "application/json");
    });

    svr.Post("/api/events", [](const httplib::Request& req, httplib::Response& res) {
        json j = json::parse(req.body);
        j["id"] = (int)time(NULL);
        run_python_post("events", j.dump());
        res.set_content("{\"status\":\"success\"}", "application/json");
    });

    svr.Delete(R"(/api/events/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        run_python_delete("events", req.matches[1]);
        res.set_content("{\"status\":\"success\"}", "application/json");
    });

    svr.Get("/api/announcements", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content(run_python_get("announcements"), "application/json");
    });

    svr.Post("/api/announcements", [](const httplib::Request& req, httplib::Response& res) {
        json j = json::parse(req.body);
        j["id"] = (int)time(NULL);
        run_python_post("announcements", j.dump());
        res.set_content("{\"status\":\"success\"}", "application/json");
    });

    svr.Delete(R"(/api/announcements/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        run_python_delete("announcements", req.matches[1]);
        res.set_content("{\"status\":\"success\"}", "application/json");
    });

    int port = 8080;
    if (std::getenv("PORT")) port = std::stoi(std::getenv("PORT"));
    std::cout << "Backend Pipe Server active on port " << port << std::endl;
    svr.listen("0.0.0.0", port);
    return 0;
}