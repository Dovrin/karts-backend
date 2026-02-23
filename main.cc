#ifdef _WIN32
  #ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0A00
  #endif
#endif

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include "httplib.h"
#include "json.hpp"

using json = nlohmann::json;

// --- STEP 1: THE PYTHON BRIDGE (PIPE) ---
// This function runs the Python script and captures its output.
// It completely removes the need for local .json files.
std::string run_python_bridge(const std::string& method, const std::string& table, const std::string& extra = "") {
    std::string command = "python3 supabase_sync.py " + method + " " + table;
    if (!extra.empty()) {
        // We wrap the extra data in quotes for the command line
        command += " '" + extra + "'";
    }

    std::array<char, 2048> buffer;
    std::string result;
    
    // Open a pipe to run the command
    #ifdef _WIN32
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(command.c_str(), "r"), _pclose);
    #else
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    #endif

    if (!pipe) {
        return "[]";
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

int main() {
    httplib::Server svr;

    // --- CORS HANDLER ---
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

    // --- LOGIN (Simple) ---
    svr.Post("/api/login", [](const httplib::Request& req, httplib::Response& res) {
        auto j_req = json::parse(req.body);
        if (j_req["username"] == "admin" && j_req["password"] == "admin123") {
            res.set_content("{\"status\":\"success\",\"token\":\"secure\",\"role\":\"admin\"}", "application/json");
        } else {
            res.status = 401;
        }
    });

    // --- EVENTS ---
    // C++ calls Python -> Python calls Supabase -> C++ gets JSON -> C++ sends to Website
    svr.Get("/api/events", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content(run_python_bridge("GET", "events"), "application/json");
    });

    svr.Post("/api/events", [](const httplib::Request& req, httplib::Response& res) {
        // We add an ID to the body before sending to python
        json j = json::parse(req.body);
        j["id"] = (int)time(NULL);
        res.set_content(run_python_bridge("POST", "events", j.dump()), "application/json");
    });

    svr.Delete(R"(/api/events/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content(run_python_bridge("DELETE", "events", req.matches[1]), "application/json");
    });

    // --- ANNOUNCEMENTS ---
    svr.Get("/api/announcements", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content(run_python_bridge("GET", "announcements"), "application/json");
    });

    svr.Post("/api/announcements", [](const httplib::Request& req, httplib::Response& res) {
        json j = json::parse(req.body);
        j["id"] = (int)time(NULL);
        res.set_content(run_python_bridge("POST", "announcements", j.dump()), "application/json");
    });

    svr.Delete(R"(/api/announcements/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content(run_python_bridge("DELETE", "announcements", req.matches[1]), "application/json");
    });

    int port = 8080;
    if (std::getenv("PORT")) port = std::stoi(std::getenv("PORT"));
    std::cout << "C++ Sustainable Pipe Server running on port " << port << std::endl;
    svr.listen("0.0.0.0", port);
    return 0;
}