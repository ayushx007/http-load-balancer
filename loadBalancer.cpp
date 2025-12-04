#include <iostream>
#include <sys/socket.h> // Core socket functions
#include <netinet/in.h> // AF_INET, sockaddr_in
#include <unistd.h>     // close()
#include <cstring>      // memset

#define PORT 8080

int main() {
    // 1. Create Server Socket
    // AF_INET = IPv4, SOCK_STREAM = TCP
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }

    // 2. Bind Socket to Port 8080
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    address.sin_port = htons(PORT); // Host to Network Short

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed. Port might be busy." << std::endl;
        return -1;
    }

    // 3. Listen for Connections (Queue size 10)
    if (listen(server_fd, 10) < 0) {
        std::cerr << "Listen failed" << std::endl;
        return -1;
    }

    std::cout << "Load Balancer listening on port " << PORT << "..." << std::endl;

    // 4. Accept Loop (Keep running forever)
    while (true) {
        int new_socket = accept(server_fd, nullptr, nullptr);
        if (new_socket < 0) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }

        // Just print that we got a connection and close it for now
        std::cout << "Received a connection!" << std::endl;
        
        // Simple response so the browser doesn't hang
        const char* hello = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello World!";
        send(new_socket, hello, strlen(hello), 0);
        
        close(new_socket);
    }

    return 0;
}