// Server side implementation of UDP client-server model
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")
  
#define PORT 8080
#define MAXLINE 1024
  
// Driver code
int main() {
    std::cout << "UDP Server is running on port " << PORT << "..." << std::endl;
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return EXIT_FAILURE;
    }

    SOCKET sockfd;
    char buffer[MAXLINE];
    sockaddr_in servaddr{}, cliaddr{};

    // Creating socket file descriptor
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == INVALID_SOCKET) {
        std::cerr << "socket creation failed" << std::endl;
        WSACleanup();
        return EXIT_FAILURE;
    }

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // Bind the socket with the server address
    if (bind(sockfd, reinterpret_cast<const sockaddr *>(&servaddr), sizeof(servaddr)) == SOCKET_ERROR) {
        std::cerr << "bind failed" << std::endl;
        closesocket(sockfd);
        WSACleanup();
        return EXIT_FAILURE;
    }

    while (true) {
        int len = sizeof(cliaddr);
        int n = recvfrom(
            sockfd,
            buffer,
            MAXLINE - 1,
            0,
            reinterpret_cast<sockaddr *>(&cliaddr),
            &len
        );

        if (n == SOCKET_ERROR) {
            std::cerr << "recvfrom failed" << std::endl;
            continue;
        }

        //print buffer length
        std::cout << "Received " << n << " bytes" << std::endl;
        buffer[n] = '\0';
        //print loop print buffer per byte data raw
        std::cout << "Raw data: ";
        for (int i = 0; i < n; i++) {
            std::cout << static_cast<int>(static_cast<unsigned char>(buffer[i])) << " ";
        }
        std::cout << std::endl;

        const char *clientIp = inet_ntoa(cliaddr.sin_addr);
        int clientPort = ntohs(cliaddr.sin_port);
        std::cout << "From " << clientIp << ":" << clientPort << " -> " << buffer << std::endl;

        std::string reply = "ACK: ";
        reply += buffer;

        if (sendto(
                sockfd,
                reply.c_str(),
                static_cast<int>(reply.size()),
                0,
                reinterpret_cast<const sockaddr *>(&cliaddr),
                len
            ) == SOCKET_ERROR) {
            std::cerr << "sendto failed" << std::endl;
        }
    }

    closesocket(sockfd);
    WSACleanup();
    return 0;
}

