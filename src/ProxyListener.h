/* * * * * * * * * * * * * * * * *
 * Congestion proxy application  *
 * Author: Martin Ubl            *
 *         kenny@cetes.cz        *
 * * * * * * * * * * * * * * * * */

#pragma once

// platform-depended macro definitions and includes

#ifdef _WIN32
#include <WS2tcpip.h>
#include <Windows.h>
#define SOCK SOCKET
#define ADDRLEN int

#define SOCKETWOULDBLOCK WSAEWOULDBLOCK
#define SOCKETCONNRESET  WSAECONNRESET
#define SOCKETCONNABORT  WSAECONNABORTED
#define SOCKETINPROGRESS WSAEINPROGRESS
#define LASTERROR() WSAGetLastError()
#define INET_PTON(fam,addrptr,buff) InetPtonA(fam,addrptr,buff)
#define INET_NTOP(fam,addrptr,buff,socksize) InetNtopA(fam,addrptr,buff,socksize)
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

#define INVALID_SOCKET -1

#define SOCKETWOULDBLOCK EAGAIN
#define SOCKETCONNABORT ECONNABORTED
#define SOCKETCONNRESET ECONNRESET
#define SOCKETINPROGRESS EINPROGRESS
#define LASTERROR() errno
#define INET_PTON(fam,addrptr,buff) inet_pton(fam,addrptr,buff)
#define INET_NTOP(fam,addrptr,buff,socksize) inet_ntop(fam,addrptr,buff,socksize)
#define CLOSESOCKET close
#endif

#ifndef MSG_NOSIGNAL
# define MSG_NOSIGNAL 0
#endif

// universal ("any") bind address 
constexpr const char* BindAnyAddr = "0.0.0.0";

// size of input buffer in bytes
constexpr size_t InBufferSize = 2048; /* 2kB */
// seconds for select timeout
constexpr long SelectTimeoutSecs = 1;
// microseconds for select timeout
constexpr long SelectTimeoutUSecs = 1;

// proxy listener class
class ProxyListener
{
    public:
        // constructor - needs address and port to be bound to
        ProxyListener(std::string bindTo, uint16_t bindPort);
        virtual ~ProxyListener();

        // initializes proxy listener; returns true on success, false on failure
        bool Init();
        // starts proxy listener thread
        void Start();
        // stops proxy listener
        void Stop();
        // joins proxy listener thread
        void Join();

    protected:
        // thread function executed as proxy listener task
        void _Run();
        // processes incoming message
        void _ProcessIncoming(char* buffer, int len);

    private:
        // proxy listener thread
        std::unique_ptr<std::thread> m_thread;

        // address to bind proxy listener to
        std::string m_bindAddr;
        // port to bind proxy listener to
        uint16_t m_bindPort;

        // socket of proxy listener
        SOCK m_socket;
        // socket address of proxy listener
        sockaddr_in m_sockAddr;

        // is thread running?
        bool m_running;

        // FD set of sockets connected to us, including our socket
        fd_set m_rdsocks;
        // custom set for faster iteration through listened sockets
        std::set<SOCK> m_sockSet;
        // maximal number of socket
        int m_maxSock;
};
