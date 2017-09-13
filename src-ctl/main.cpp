/* * * * * * * * * * * * * * * * *
 * Congestion proxy application  *
 * Author: Martin Ubl            *
 *         kenny@cetes.cz        *
 * * * * * * * * * * * * * * * * */

#include <iostream>

// platform-dependend macro definitions

#ifdef _WIN32
#include <WS2tcpip.h>
#include <Windows.h>
#define SOCK SOCKET
#define ADDRLEN int

#define LASTERROR() WSAGetLastError()
#define INET_PTON(fam,addrptr,buff) InetPtonA(fam,addrptr,buff)
#define CLOSESOCKET closesocket
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <netdb.h>
#include <fcntl.h>

#define SOCK int
#define ADDRLEN socklen_t

#define LASTERROR() errno
#define INET_PTON(fam,addrptr,buff) inet_pton(fam,addrptr,buff)
#define CLOSESOCKET close
#endif

// TODO: move these values to config

// daemon address
constexpr char* DaemonAddr = "127.0.0.1";
// daemon port
constexpr uint16_t DaemonPort = 9967;

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "Invalid parameter count" << std::endl
                  << "Usage: congestion-proxy-ctl <recipient> <message>" << std::endl
                  << std::endl;

        return -1;
    }

#ifdef _WIN32
    // startup WSA on Windows
    WORD version = MAKEWORD(2, 2);
    WSADATA data;
    if (WSAStartup(version, &data) != 0)
        std::cerr << "Unable to start WinSock service for unknown reason" << std::endl;
#endif

    SOCK dsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (!dsock)
    {
        std::cerr << "Could not initialize socket, error = " << LASTERROR() << std::endl;
        return 1;
    }

    sockaddr_in saddr;

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(DaemonPort);

    INET_PTON(AF_INET, DaemonAddr, &saddr.sin_addr);

    if (saddr.sin_addr.s_addr == INADDR_NONE)
    {
        std::cerr << "Could not resolve daemon address" << std::endl;
        return 2;
    }

    int res;

    res = connect(dsock, (sockaddr*)&saddr, sizeof(sockaddr_in));
    if (res < 0)
    {
        std::cerr << "Could not connect to daemon, result = " << res << ", error = " << LASTERROR() << std::endl;
        return 3;
    }

    std::string toSend = std::string(argv[1]) + "\n" + argv[2];

    res = send(dsock, toSend.c_str(), (int)toSend.length(), 0);

    if (res < 0)
    {
        std::cerr << "Could not send payload to daemon, result = " << res << ", error = " << LASTERROR() << std::endl;
        return 4;
    }

    CLOSESOCKET(dsock);

    return 0;
}
