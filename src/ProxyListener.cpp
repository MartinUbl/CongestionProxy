/* * * * * * * * * * * * * * * * *
 * Congestion proxy application  *
 * Author: Martin Ubl            *
 *         kenny@cetes.cz        *
 * * * * * * * * * * * * * * * * */

#include "general.h"
#include "ProxyListener.h"
#include "QueueManager.h"

#include <cstring>

ProxyListener::ProxyListener(std::string bindTo, uint16_t bindPort) : m_bindAddr(bindTo), m_bindPort(bindPort)
{
    //
}

ProxyListener::~ProxyListener()
{
    //
}

bool ProxyListener::Init()
{
    int res;

#ifdef _WIN32
    // On Windows, initialize WSA just once
    static std::once_flag g_wsaInit;

    std::call_once(g_wsaInit, [=]() {
        WORD version = MAKEWORD(2, 2);
        WSADATA data;
        if (WSAStartup(version, &data) != 0)
            std::cerr << "Unable to start WinSock service for unknown reason" << std::endl;
    });
#endif

    // initialize TCP socket
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket < 0)
        return false;

    // set SO_REUSEADDR
    int param = 1;
    if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&param, sizeof(int)) == -1)
    {
        std::cerr << "Could not set SO_REUSEADDR flag to socket" << std::endl;
        return false;
    }

    // determine bind address from string
    if (m_bindAddr == BindAnyAddr)
        m_sockAddr.sin_addr.s_addr = INADDR_ANY;
    else
        INET_PTON(AF_INET, m_bindAddr.c_str(), &m_sockAddr.sin_addr);

    if (m_sockAddr.sin_addr.s_addr == INADDR_NONE)
    {
        std::cerr << "Could not resolve bind address: " << m_bindAddr << std::endl;
        return false;
    }

    // store address family and port
    m_sockAddr.sin_family = AF_INET;
    m_sockAddr.sin_port = htons(m_bindPort);

    // bind!
    res = bind(m_socket, (sockaddr*)&m_sockAddr, sizeof(sockaddr_in));

    if (res < 0)
    {
        std::cerr << "Could not bind to: " << m_bindAddr << std::endl;
        return false;
    }

    // create listen queue
    res = listen(m_socket, 10);

    if (res < 0)
    {
        std::cerr << "Could not create listen queue for socket" << std::endl;
        return false;
    }

    // set initial state of fdset
    FD_ZERO(&m_rdsocks);
    FD_SET(m_socket, &m_rdsocks);
    m_sockSet.insert(m_socket);
    m_maxSock = (int)m_socket;

    return true;
}

void ProxyListener::Start()
{
    m_thread = std::make_unique<std::thread>(&ProxyListener::_Run, this);

    std::cout << "Proxy listener started at " << m_bindAddr << ":" << m_bindPort << std::endl;
}

void ProxyListener::Stop()
{
    m_running = false;

    // this should interrupt select()
    CLOSESOCKET(m_socket);
}

void ProxyListener::_Run()
{
    m_running = true;
    fd_set rdset;
    int res;
    SOCK resSocket;
    struct timeval tv;
    sockaddr_in inaddr;
    ADDRLEN addrlen;

    char inbuffer[InBufferSize];
    std::set<SOCK> toremove;

    while (m_running)
    {
        memcpy(&rdset, &m_rdsocks, sizeof(fd_set));
        tv.tv_sec = SelectTimeoutSecs;
        tv.tv_usec = SelectTimeoutUSecs;

        res = select(m_maxSock + 1, &rdset, nullptr, nullptr, &tv);

        if (!m_running)
            break;

        if (res == 0)
        {
            // timeout, do nothing
        }
        else if (res < 0)
        {
            std::cerr << "Socket error: " << LASTERROR() << std::endl;
            break;
        }
        else
        {
            for (SOCK const& sockfd : m_sockSet)
            {
                if (!FD_ISSET(sockfd, &rdset))
                    continue;

                // our socket = new connection incoming
                if (sockfd == m_socket)
                {
                    addrlen = sizeof(sockaddr_in);
                    resSocket = accept(m_socket, (sockaddr*)&inaddr, &addrlen);

                    if (resSocket > 0)
                    {
                        FD_SET(resSocket, &m_rdsocks);
                        m_sockSet.insert(resSocket);

                        if ((int)resSocket > m_maxSock)
                            m_maxSock = (int)resSocket;
                    }
                }
                else // others socket = new message incoming
                {
                    res = recv(sockfd, inbuffer, InBufferSize, 0);

                    if (res > 0)
                    {
                        _ProcessIncoming(inbuffer, res);
                    }
                    else
                    {
                        FD_CLR(sockfd, &m_rdsocks);
                        toremove.insert(sockfd);
                    }
                }
            }

            // remove sockets that caused errors (were closed, are in errorneous state, ..)
            for (SOCK const& sockfd : toremove)
                m_sockSet.erase(sockfd);
        }
    }
}

void ProxyListener::_ProcessIncoming(char* buffer, int len)
{
    std::stringstream is(std::string(buffer, len));

    std::string target, message;

    std::getline(is, target);
    std::getline(is, message);

    // discard empty messages/senders
    if (target.empty() || message.empty())
        return;

    // post message to queue
    Queue& q = sQueueMgr->GetQueue(target);
    q.Post(message);
}

void ProxyListener::Join()
{
    if (m_thread && m_thread->joinable())
        m_thread->join();
}
