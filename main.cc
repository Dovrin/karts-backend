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
#include <ctime>
#include <algorithm>
#include "httplib.h"
#include "json.hpp"

using json = nlohmann::json;

// --- STEP 1: SUSTAINABILITY CONFIG (SUPABASE) ---
// We read these from Environment Variables set in Render.com
std::string get_env_var(const char* key, const std::string& default_val = "") {
    const char* val = std::getenv(key);
    return val ? std::string(val) : default_val;
}

// These will be configured in the Render Dashboard
std::string SUPABASE_URL = get_env_var("SUPABASE_URL"); 
std::string SUPABASE_KEY = get_env_var("SUPABASE_KEY");

// Helper to make requests to Supabase Cloud
std::string call_supabase(const std::string& table, const std::string& method = "GET", const std::string& body = "", const std::string& query = "") {
    // Example: https://xyz.supabase.co/rest/v1/events
    std::string host = SUPABASE_URL;
    if (host.find("https://") == 0) host.erase(0, 8);
    
    httplib::Client cli(host);
    cli.enable_server_certificate_verification(false); // For school project simplicity

    httplib::Headers headers = {
        {"apikey", SUPABASE_KEY},
        {"Authorization", "Bearer " + SUPABASE_KEY},
        {"Content-Type", "application/json"},
        {"Prefer", "return=representation"}
    };

    httplib::Result res;
    std::string path = "/rest/v1/" + table + query;

    if (method == "GET") res = cli.Get(path.c_str(), headers);
    else if (method == "POST") res = cli.Post(path.c_str(), headers, body, "application/json");
    else if (method == "DELETE") res = cli.Delete(path.c_str(), headers);

    if (res && res->status >= 200 && res->status < 300) {
        return res->body;
    }
    std::cerr << "Supabase Error: " << (res ? std::to_string(res->status) : "Connection Failed") << " on " << path << std::endl;
    return "[]";
}

int main() {
    httplib::Server svr;

    // --- CORS HANDLER ---
    svr.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, GET, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, apikey, Authorization");
        if (req.method == "OPTIONS") {
            res.status = 204;
            return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });

    // --- STEP 2: LOGIN (STILL LOCAL/SIMPLE) ---
    svr.Post("/api/login", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j_req = json::parse(req.body);
            if (j_req["username"] == "admin" && j_req["password"] == "admin123") {
                json response = {{"status", "success"}, {"token", "secure_token"}, {"role", "admin"}};
                res.set_content(response.dump(), "application/json");
            } else {
                res.status = 401;
                res.set_content("{\"status\":\"error\",\"message\":\"Wrong admin password\"}", "application/json");
            }
        } catch (...) {
            res.status = 400;
        }
    });

    // --- STEP 3: EVENTS (FROM SUPABASE) ---
    svr.Get("/api/events", [](const httplib::Request& req, httplib::Response& res) {
        std::string data = call_supabase("events", "GET");
        res.set_content(data, "application/json");
    });

    svr.Post("/api/events", [](const httplib::Request& req, httplib::Response& res) {
        std::string data = call_supabase("events", "POST", req.body);
        res.set_content("{\"status\":\"success\"}", "application/json");
    });

    svr.Delete(R"(/api/events/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        std::string id = req.matches[1];
        call_supabase("events", "DELETE", "", "?id=eq." + id);
        res.set_content("{\"status\":\"success\"}", "application/json");
    });

    // --- STEP 4: ANNOUNCEMENTS (FROM SUPABASE) ---
    svr.Get("/api/announcements", [](const httplib::Request& req, httplib::Response& res) {
        std::string data = call_supabase("announcements", "GET");
        res.set_content(data, "application/json");
    });

    svr.Post("/api/announcements", [](const httplib::Request& req, httplib::Response& res) {
        std::string data = call_supabase("announcements", "POST", req.body);
        res.set_content("{\"status\":\"success\"}", "application/json");
    });

    svr.Delete(R"(/api/announcements/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        std::string id = req.matches[1];
        call_supabase("announcements", "DELETE", "", "?id=eq." + id);
        res.set_content("{\"status\":\"success\"}", "application/json");
    });

    // --- START SERVER ---
    int port = 8080;
    const char* port_env = std::getenv("PORT");
    if (port_env) port = std::stoi(port_env);

    std::cout << "C++ Sustainable Server Bridge Running on Port " << port << std::endl;
    svr.listen("0.0.0.0", port);
    return 0;
}