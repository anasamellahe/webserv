#include "main.hpp"
#include "HTTP/Request.hpp"
#include "Socket/socket.hpp"
#include "Server/monitorClient.hpp"
#include <iostream>
#include <string>
#include <unistd.h>
#include <cstring>

// Simple HTTP request for testing
const char* TEST_HTTP_REQUEST = 
    "GET /test/path/index.html HTTP/1.1\r\n"
    "Host: localhost:8080\r\n"
    "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36\r\n"
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
    "Accept-Language: en-US,en;q=0.5\r\n"
    "Accept-Encoding: gzip, deflate, br\r\n"
    "Connection: keep-alive\r\n"
    "Referer: http://localhost:8080/\r\n"
    "Cookie: sessionID=abc123; userToken=xyz789\r\n"
    "Cache-Control: max-age=0\r\n"
    "Content-Type: application/x-www-form-urlencoded\r\n"
    "Content-Length: 0 \r\n"
    "\r\n";

// Test request parsing
void test_request_parsing() {
    std::cout << "=== Testing HTTP Request Parsing ===" << std::endl;

    Request req; // Create request object with dummy client FD
    std::string req_str = TEST_HTTP_REQUEST;

    bool result = req.parseFromSocket(0, req_str.c_str(), req_str.size());
    std::cout << "Parse result: " << (result ? "SUCCESS" : "FAILED") << std::endl;
    if (result) {
        std::cout << "Method: " << req.getMethod() << std::endl;
        std::cout << "Path: " << req.getPath() << std::endl;
        std::cout << "Host header: " << req.getHeader("host") << std::endl;

        std::cout << "\nAll Headers:" << std::endl;
        auto headers = req.getAllHeaders();
        for (auto it = headers.begin(); it != headers.end(); ++it) {
            std::cout << " - " << it->first << ": " << it->second << std::endl;
        }
    } else {
        std::cout << "Error code: " << req.getErrorCode() << std::endl;
    }
}

// Test socket creation
void test_socket_creation() {
    std::cout << "\n=== Testing Socket Creation ===" << std::endl;
    
    try {
        std::vector<std::pair<std::string, int> > hosts;
        hosts.push_back(std::make_pair("127.0.0.1", 8080));
        
        sock test_socket(hosts);
        std::vector<int> fds = test_socket.getFDs();
        
        std::cout << "Created " << fds.size() << " socket(s)" << std::endl;
        for (size_t i = 0; i < fds.size(); i++) {
            std::cout << "Socket FD " << i << ": " << fds[i] << std::endl;
        }
        
        // Close the sockets to not leave them hanging
        test_socket.closeFDs("Test completed");
    } catch (const std::exception& e) {
        std::cout << "Socket creation error: " << e.what() << std::endl;
    }
}

// Test monitor client with manual connection
void test_monitor_client(bool run_event_loop = false) {
    std::cout << "\n=== Testing Monitor Client ===" << std::endl;
    
    try {
        std::vector<std::pair<std::string, int> > hosts;
        hosts.push_back(std::make_pair("127.0.0.1", 8080));
        
        sock test_socket(hosts);
        std::vector<int> fds = test_socket.getFDs();
        
        monitorClient mc(fds);
        std::cout << "Monitor client created successfully" << std::endl;
        
        if (run_event_loop) {
            std::cout << "Starting event loop (will run until terminated)..." << std::endl;
            mc.startEventLoop();
        }
        
        // If not running event loop, close the sockets
        if (!run_event_loop) {
            test_socket.closeFDs("Test completed");
        }
    } catch (const std::exception& e) {
        std::cout << "Monitor client error: " << e.what() << std::endl;
    }
}

int main(int argc, char** argv) {
    std::cout << "=== Webserver Component Testing ===" << std::endl;
    
    // Process command line arguments
    bool run_full_server = false;
    bool test_sockets = true;
    bool test_request = true;
    bool test_monitor = true;
    
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--full-server") {
            run_full_server = true;
            test_sockets = false;
            test_request = false;
            test_monitor = false;
        } else if (arg == "--sockets") {
            test_sockets = true;
            test_request = false;
            test_monitor = false;
        } else if (arg == "--request") {
            test_sockets = false;
            test_request = true;
            test_monitor = false;
        } else if (arg == "--monitor") {
            test_sockets = false;
            test_request = false;
            test_monitor = true;
        }
    }
    
    // Run selected tests
    if (test_request) {
        test_request_parsing();
    }
    
    if (test_sockets) {
        test_socket_creation();
    }
    
    if (test_monitor) {
        test_monitor_client(false); // Set to true to run the event loop
    }
    
    // Run the full server with config if requested
    if (run_full_server) {
        std::cout << "\n=== Starting Full Server ===" << std::endl;
        
        try {
            ConfigParser config_parser;
            Config config;
            
            // Use default config file
            const char* config_file = "src/config.conf";
            if (argc > 2) {
                config_file = argv[2];
            }
            
            int parse_config = config_parser.parseConfigFile(config_file, config);
            if (parse_config == 0) {
                config_parser.initializeServerListenAddresses(config);
                config_parser.printConfig(config);
                
                sock socketCreate(config_parser.getServerListenAddresses());
                std::cout << "Sockets created successfully" << std::endl;
                
                monitorClient mc(socketCreate.getFDs());
                std::cout << "Starting event loop..." << std::endl;
                mc.startEventLoop();
            } else {
                std::cerr << "Error parsing configuration file." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << std::endl;
            return 1;
        }
    }
    
    std::cout << "\n=== Testing Complete ===" << std::endl;
    return 0;
}