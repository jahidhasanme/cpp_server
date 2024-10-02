#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <map>
#include <thread>
#include <unordered_map>
#include <functional>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/exception/exception.hpp>
#include <dotenv.h>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

typedef websocketpp::server<websocketpp::config::asio> server;

mongocxx::instance instance{};

namespace websocketpp
{
    struct connection_hdl_hash
    {
        std::size_t operator()(const connection_hdl &hdl) const
        {
            return std::hash<uintptr_t>()(reinterpret_cast<uintptr_t>(hdl.lock().get()));
        }
    };

    struct connection_hdl_equal
    {
        bool operator()(const connection_hdl &lhs, const connection_hdl &rhs) const
        {
            return lhs.lock() == rhs.lock();
        }
    };
}

class WebSocketServer
{
public:
    WebSocketServer(mongocxx::database &db) : db(db)
    {
        ws_server.set_access_channels(websocketpp::log::alevel::all);
        ws_server.clear_access_channels(websocketpp::log::alevel::frame_payload);
        ws_server.init_asio();

        ws_server.set_open_handler(std::bind(&WebSocketServer::on_open, this, std::placeholders::_1));
        ws_server.set_close_handler(std::bind(&WebSocketServer::on_close, this, std::placeholders::_1));
        ws_server.set_message_handler(std::bind(&WebSocketServer::on_message, this, std::placeholders::_1, std::placeholders::_2));
    }

    void run(uint16_t port)
    {
        ws_server.listen(port);
        ws_server.start_accept();
        std::cout << "WebSocket server listening on port " << port << "..." << std::endl;
        ws_server.run();
    }

private:
    server ws_server;
    std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> connections;
    mongocxx::database &db;

    std::unordered_map<websocketpp::connection_hdl, std::string,
                       websocketpp::connection_hdl_hash,
                       websocketpp::connection_hdl_equal>
        user_connections;

    void on_open(websocketpp::connection_hdl hdl)
    {
        connections.insert(hdl);
        std::cout << "New connection opened: " << hdl.lock().get() << std::endl;
    }

    void on_close(websocketpp::connection_hdl hdl)
    {
        user_connections.erase(hdl);
        connections.erase(hdl);
        std::cout << "Connection closed: " << hdl.lock().get() << std::endl;
    }

    void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg)
    {
        auto payload = msg->get_payload();
        auto message_data = bsoncxx::from_json(payload);

        std::string action = std::string{message_data["action"].get_string().value.data()};

        if (action == "register")
        {
            handle_registration(message_data);
        }
        else if (action == "login")
        {
            handle_login(hdl, message_data);
        }
        else if (action == "send_message")
        {
            handle_send_message(hdl, message_data);
        }
        else
        {
            std::cerr << "Unknown action: " << action << std::endl;
        }
    }

    void handle_registration(bsoncxx::document::view message_data)
    {
        std::string username = std::string{message_data["username"].get_string().value.data()};
        std::string password = std::string{message_data["password"].get_string().value.data()};

        auto users_collection = db["users"];
        bsoncxx::builder::stream::document builder;
        auto document = builder << "username" << username
                                << "password" << password
                                << bsoncxx::builder::stream::finalize;

        try
        {
            users_collection.insert_one(document.view());
            std::cout << "User registered: " << username << std::endl;
        }
        catch (const mongocxx::exception &e)
        {
            std::cerr << "Registration failed: " << e.what() << std::endl;
        }
    }

    void handle_login(websocketpp::connection_hdl hdl, bsoncxx::document::view message_data)
    {
        std::string username = std::string{message_data["username"].get_string().value.data()};
        std::string password = std::string{message_data["password"].get_string().value.data()};

        auto users_collection = db["users"];

        auto user = users_collection.find_one(bsoncxx::builder::stream::document{}
                                              << "username" << username
                                              << bsoncxx::builder::stream::finalize);

        if (!user)
        {
            std::string error_msg = "User not found.";
            ws_server.send(hdl, error_msg, websocketpp::frame::opcode::text);
            return;
        }

        auto user_doc = user->view();

        if (user_doc["password"].get_string().value == password)
        {
            user_connections[hdl] = username;
            ws_server.send(hdl, "{\"type\":\"welcome\"}", websocketpp::frame::opcode::text);
        }

        std::string error_msg = "Login failed. Username or password is incorrect.";
        ws_server.send(hdl, error_msg, websocketpp::frame::opcode::text);
    }

    void handle_send_message(websocketpp::connection_hdl hdl, bsoncxx::document::view message_data)
    {
        std::string recipient = std::string{message_data["recipient"].get_string().value.data()};
        std::string text = std::string{message_data["text"].get_string().value.data()};
        std::string sender = user_connections[hdl];

        for (const auto &conn : connections)
        {
            if (user_connections[conn] == recipient)
            {
                std::string full_msg = sender + ": " + text;
                ws_server.send(conn, full_msg, websocketpp::frame::opcode::text);
                break;
            }
        }
    }
};

class HttpServer
{
public:
    HttpServer(uint16_t port) : acceptor(io_context, {tcp::v4(), port})
    {
        std::cout << "HTTP server listening on port " << port << "..." << std::endl;
    }

    void run()
    {
        while (true)
        {
            tcp::socket socket(io_context);
            acceptor.accept(socket);
            std::thread(&HttpServer::handle_request, this, std::move(socket)).detach();
        }
    }

private:
    net::io_context io_context;
    tcp::acceptor acceptor;

    void handle_request(tcp::socket socket)
    {
        beast::flat_buffer buffer;
        http::request<http::string_body> req;

        beast::error_code ec;
        http::read(socket, buffer, req, ec);
        if (ec)
        {
            std::cerr << "Error reading request: " << ec.message() << std::endl;
            return;
        }

        std::string path(req.target().data(), req.target().size());
        if (path == "/")
        {
            path = "/index.html";
        }
        std::string full_path = "static" + path;
        std::string response_body = read_file(full_path);

        http::response<http::string_body> res;
        if (response_body.empty())
        {
            res.result(http::status::not_found);
            res.set(http::field::content_type, "text/plain");
            res.body() = "404 Not Found";
        }
        else
        {
            res.result(http::status::ok);
            res.set(http::field::content_type, get_content_type(full_path));
            res.body() = response_body;
        }
        res.prepare_payload();

        http::write(socket, res, ec);
        if (ec)
        {
            std::cerr << "Failed to write response: " << ec.message() << std::endl;
        }
    }

    std::string read_file(const std::string &path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            return {};
        }
        return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }

    std::string get_content_type(const std::string &path)
    {
        static const std::map<std::string, std::string> mime_types{
            {".html", "text/html"},
            {".css", "text/css"},
            {".js", "application/javascript"},
            {".png", "image/png"},
            {".jpg", "image/jpeg"},
            {".jpeg", "image/jpeg"},
            {".gif", "image/gif"},
            {".json", "application/json"},
            {".svg", "image/svg+xml"},
            {".woff", "font/woff"},
            {".woff2", "font/woff2"},
            {".ttf", "font/ttf"},
            {".ico", "image/x-icon"},
            {".txt", "text/plain"},
        };

        std::string ext = path.substr(path.find_last_of('.'));
        auto it = mime_types.find(ext);
        return (it != mime_types.end()) ? it->second : "application/octet-stream";
    }
};

int main()
{
    try
    {
        mongocxx::uri uri{dotenv::getValue("DB_URL")};
        mongocxx::client client(uri);
        std::cout << "Connected to MongoDB" << std::endl;

        mongocxx::database db = client["chat_app"];

        WebSocketServer ws_server(db);
        std::thread ws_thread([&]()
                              { ws_server.run(std::stoi(dotenv::getValue("WS_PORT"))); });

        HttpServer http_server(std::stoi(dotenv::getValue("PORT")));
        http_server.run();

        ws_thread.join();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}