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
#include "httplib.h"
#include "json.hpp"

using json = nlohmann::json;

// --- CONFIGURATION ---
const std::string FILE_EVENTS = "events.json";
const std::string FILE_ANNOUNCEMENTS = "announcements.json";
const std::string FILE_USERS = "users.json";

// --- HELPERS ---
void initialize_files() {
    auto setup_file = [](const std::string& filename, const json& default_val) {
        std::ifstream f(filename);
        if (!f.good()) {
            std::ofstream of(filename);
            of << default_val.dump(4);
            std::cout << "[Init] Created " << filename << std::endl;
        }
    };

    setup_file(FILE_EVENTS, json::array());
    setup_file(FILE_ANNOUNCEMENTS, json::array());
    
    // Default admin user
    json default_users = json::array({
        {{"username", "admin"}, {"password", "admin123"}, {"role", "admin"}}
    });
    setup_file(FILE_USERS, default_users);
}

json read_json(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return json::array();
    json j;
    file >> j;
    return j;
}

void write_json(const std::string& filename, const json& j) {
    std::ofstream file(filename);
    file << j.dump(4);
}

int main() {
    initialize_files();
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

    // --- AUTH ---
    svr.Post("/api/login", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j_req = json::parse(req.body);
            std::string user = j_req.value("username", "");
            std::string pass = j_req.value("password", "");

            json users = read_json(FILE_USERS);
            for (auto& u : users) {
                if (u["username"] == user && u["password"] == pass) {
                    json response = {{"status", "success"}, {"token", "token_" + user}, {"role", u["role"]}};
                    res.set_content(response.dump(), "application/json");
                    return;
                }
            }
            res.status = 401;
            res.set_content("{\"status\":\"error\",\"message\":\"Invalid credentials\"}", "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content("{\"status\":\"error\",\"message\":\"Bad Request\"}", "application/json");
        }
    });

    // --- EVENTS ---
    svr.Get("/api/events", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content(read_json(FILE_EVENTS).dump(), "application/json");
    });

    svr.Post("/api/events", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto new_event = json::parse(req.body);
            new_event["id"] = (int)time(NULL);
            auto data = read_json(FILE_EVENTS);
            data.push_back(new_event);
            write_json(FILE_EVENTS, data);
            res.set_content("{\"status\":\"success\"}", "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content("{\"status\":\"error\"}", "application/json");
        }
    });

    svr.Delete(R"(/api/events/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        auto data = read_json(FILE_EVENTS);
        auto it = std::remove_if(data.begin(), data.end(), [&](const json& item) { return item["id"] == id; });
        if (it != data.end()) {
            data.erase(it, data.end());
            write_json(FILE_EVENTS, data);
            res.set_content("{\"status\":\"success\"}", "application/json");
        } else {
            res.status = 404;
            res.set_content("{\"status\":\"error\"}", "application/json");
        }
    });

    // --- ANNOUNCEMENTS ---
    svr.Get("/api/announcements", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content(read_json(FILE_ANNOUNCEMENTS).dump(), "application/json");
    });

    svr.Post("/api/announcements", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto new_ann = json::parse(req.body);
            new_ann["id"] = (int)time(NULL);
            auto data = read_json(FILE_ANNOUNCEMENTS);
            data.push_back(new_ann);
            write_json(FILE_ANNOUNCEMENTS, data);
            res.set_content("{\"status\":\"success\"}", "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content("{\"status\":\"error\"}", "application/json");
        }
    });

    svr.Delete(R"(/api/announcements/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        int id = std::stoi(req.matches[1]);
        auto data = read_json(FILE_ANNOUNCEMENTS);
        auto it = std::remove_if(data.begin(), data.end(), [&](const json& item) { return item["id"] == id; });
        if (it != data.end()) {
            data.erase(it, data.end());
            write_json(FILE_ANNOUNCEMENTS, data);
            res.set_content("{\"status\":\"success\"}", "application/json");
        } else {
            res.status = 404;
            res.set_content("{\"status\":\"error\"}", "application/json");
        }
    });

    // --- START SERVER ---
    int port = 8080;
    if (const char* env_p = std::getenv("PORT")) {
        port = std::stoi(env_p);
    }

    std::cout << "Server starting on port " << port << std::endl;
    svr.listen("0.0.0.0", port);
    return 0;
}