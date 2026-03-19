// Server side implementation of UDP client-server model
#include <iostream>
#include <array>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cstdint>
#include <iomanip>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define MAXLINE 1024

void clearScreen();
std::array<double, 21> packedData(const char *buffer, int n);
int udpServer();
void printPackedData(const std::array<double, 21> &data, char *buffer, int n);
void throtle(double packedLY);
void yaw(double packedLX);
void roll(double packedRX);
void pitch(double packedRY);

// Driver code
int main()
{
    udpServer();
    return 0;
}

void clearScreen()
{
// Check the operating system and use the appropriate command
#ifdef _WIN32
    std::system("cls"); // For Windows
#else
    std::system("clear"); // For Linux and macOS
#endif
}

int udpServer()
{
    std::cout << "UDP Server is running on port " << PORT << "..." << std::endl;
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed" << std::endl;
        return EXIT_FAILURE;
    }

    SOCKET sockfd;
    char buffer[MAXLINE];
    sockaddr_in servaddr{}, cliaddr{};

    // Creating socket file descriptor
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == INVALID_SOCKET)
    {
        std::cerr << "socket creation failed" << std::endl;
        WSACleanup();
        return EXIT_FAILURE;
    }

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // Bind the socket with the server address
    if (bind(sockfd, reinterpret_cast<const sockaddr *>(&servaddr), sizeof(servaddr)) == SOCKET_ERROR)
    {
        std::cerr << "bind failed" << std::endl;
        closesocket(sockfd);
        WSACleanup();
        return EXIT_FAILURE;
    }

    // from here ----------------------------------------------------------------------------------
    while (true)
    {
        int len = sizeof(cliaddr);
        int n = recvfrom(sockfd, buffer, MAXLINE - 1, 0, reinterpret_cast<sockaddr *>(&cliaddr), &len);

        if (n == SOCKET_ERROR)
        {
            std::cerr << "recvfrom failed" << std::endl;
            continue;
        }

        auto packedDataArray = packedData(buffer, n);

        printPackedData(packedDataArray, buffer, n);

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
                len) == SOCKET_ERROR)
        {
            std::cerr << "sendto failed" << std::endl;
        }
    }

    closesocket(sockfd);
    WSACleanup();
    return 0;
}

std::array<double, 21> packedData(const char *buffer, int n)
{
    // Inisialisasi array dengan 0.0
    std::array<double, 21> arr = {0.0};
    if (n < 4)
        return arr; // Proteksi minimal jika buffer terlalu pendek

    // Helper untuk membaca Int16 Little-Endian
    auto readInt16LE = [&](int offset) -> int16_t
    {
        uint16_t lo = static_cast<uint8_t>(buffer[offset]);
        uint16_t hi = static_cast<uint8_t>(buffer[offset + 1]);
        return static_cast<int16_t>((hi << 8) | lo);
    };

    // Helper untuk normalisasi axis (-32768 s/d 32767 menjadi -1.0 s/d 1.0)
    auto normalizeAxis = [](int16_t raw) -> double
    {
        return (raw == -32768) ? -1.0 : static_cast<double>(raw) / 32767.0;
    };

    // 1. Ekstraksi Tombol (Byte 0 & 1)
    uint8_t b0 = static_cast<uint8_t>(buffer[0]);
    uint8_t b1 = static_cast<uint8_t>(buffer[1]);

    // 3. Ekstraksi Analog Sticks (Byte 4-11)
    if (n >= 12)
    {
        arr[0] = normalizeAxis(readInt16LE(4));  // LX
        arr[1] = normalizeAxis(readInt16LE(6));  // LY
        arr[2] = normalizeAxis(readInt16LE(8));  // RX
        arr[3] = normalizeAxis(readInt16LE(10)); // RY
    }

    // 2. Ekstraksi Trigger (Byte 2 & 3) - Biasanya 0-255
    arr[4] = static_cast<double>(static_cast<uint8_t>(buffer[2])) / 255.0; // RT
    arr[5] = static_cast<double>(static_cast<uint8_t>(buffer[3])) / 255.0; // LT

    // Pemetaan bit ke double (1.0 jika ditekan, 0.0 jika tidak)
    arr[6] = (b0 & 0x01) ? 1.0 : 0.0;  // A
    arr[7] = (b0 & 0x02) ? 1.0 : 0.0;  // B
    arr[8] = (b0 & 0x04) ? 1.0 : 0.0;  // X
    arr[9] = (b0 & 0x08) ? 1.0 : 0.0;  // Y
    arr[10] = (b0 & 0x10) ? 1.0 : 0.0; // LB
    arr[11] = (b0 & 0x20) ? 1.0 : 0.0; // RB
    arr[12] = (b0 & 0x40) ? 1.0 : 0.0; // Back
    arr[13] = (b0 & 0x80) ? 1.0 : 0.0; // Start
    arr[14] = (b1 & 0x40) ? 1.0 : 0.0; // Home
    arr[15] = (b1 & 0x04) ? 1.0 : 0.0; // D-Up
    arr[16] = (b1 & 0x08) ? 1.0 : 0.0; // D-Down
    arr[17] = (b1 & 0x10) ? 1.0 : 0.0; // D-Left
    arr[18] = (b1 & 0x20) ? 1.0 : 0.0; // D-Right
    arr[19] = (b1 & 0x01) ? 1.0 : 0.0; // LS
    arr[20] = (b1 & 0x02) ? 1.0 : 0.0; // RS

    return arr;
}

void printPackedData(const std::array<double, 21> &data, char *buffer, int n)
{
    auto [
        packedLX,
        packedLY,
        packedRX,
        packedRY,
        packedRT,
        packedLT,
        packedA,
        packedB,
        packedX,
        packedY,
        packedLB,
        packedRB,
        packedBack,
        packedStart,
        packedHome,
        packedDUp,
        packedDDown,
        packedDLeft,
        packedDRight,
        packedLS,
        packedRS
    ] = data;
    // print buffer length
    //  std::cout << "Received " << n << " bytes" << std::endl;
    // buffer[n] = '\0';
    // print loop print buffer per byte data raw
    clearScreen();
    std::cout << "Raw data: ";
    for (int i = 0; i < n; i++)
    {
        std::cout << static_cast<int>(static_cast<unsigned char>(buffer[i])) << " ";
    }
    std::cout << std::fixed << std::setprecision(2);
    std::cout << std::endl;
    if (n >= 12)
    {
        std::cout << "Normalized Data" << std::endl
                  << "A: " << packedA << ", B: " << packedB
                  << ", X: " << packedX << ", Y: " << packedY
                  << ", LB: " << packedLB << ", RB: " << packedRB
                  << ", LS: " << packedLS << ", RS: " << packedRS
                  << ", Back: " << packedBack << ", Start: " << packedStart
                  << ", D-Up: " << packedDUp << ", D-Down: " << packedDDown
                  << ", D-Left: " << packedDLeft << ", D-Right: " << packedDRight
                  << ", Home: " << packedHome << std::endl;
        std::cout << "Axis:" << std::endl
                  << "L(" << packedLX << ", " << packedLY
                  << ")" << std::endl
                  << "R(" << packedRX << ", " << packedRY << ")"
                  << std::endl
                  << "RT: " << packedRT << ", LT: " << packedLT<< std::endl;
    }
    else
    {
        std::cout << "Axis packet terlalu pendek";
    }
    std::cout << "--------------------" << std::endl;
    throtle(packedLY);
    yaw(packedLX);
    roll(packedRX);
    pitch(packedRY);
}

void throtle(double packedLY){
    std::cout << "Throtle: ";
    if(packedLY > 0){
        std::cout << "DECREASED(" << std::abs(packedLY) << ")";
    } else if(packedLY < 0){
        std::cout << "INCREASED(" << std::abs(packedLY) << ")";
    }
    std::cout << std::endl;
}

void yaw(double packedLX){
    std::cout << "Yaw: ";
    if(packedLX > 0){
        std::cout << "Right(" << std::abs(packedLX) << ")";
    } else if(packedLX < 0){
        std::cout << "Left(" << std::abs(packedLX) << ")";
    }
    std::cout << std::endl;
}

void roll(double packedRX){
    std::cout << "Roll: ";
    if(packedRX > 0){
        std::cout << "Right(" << std::abs(packedRX) << ")";
    } else if(packedRX < 0){
        std::cout << "Left(" << std::abs(packedRX) << ")";
    }
    std::cout << std::endl;

}

void pitch(double packedRY){
    std::cout << "Pitch: ";
    if(packedRY > 0){
        std::cout << "Down(" << std::abs(packedRY) << ")";
    } else if(packedRY < 0){
        std::cout << "Up(" << std::abs(packedRY) << ")";
    }
    std::cout << std::endl;
}

