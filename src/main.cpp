/* * * * * * * * * * * * * * * * *
 * Congestion proxy application  *
 * Author: Martin Ubl            *
 *         kenny@cetes.cz        *
 * * * * * * * * * * * * * * * * */

#include "general.h"
#include "Config.h"
#include "ProxyListener.h"
#include "QueueManager.h"

#include <csignal>

// global instance of proxy listener
static std::unique_ptr<ProxyListener> g_proxy;

void sighandler(int signo)
{
    if (signo == SIGINT)
    {
        std::cout << "Interrupted. Exiting..." << std::endl;

        // shut down proxy listener
        if (g_proxy)
        {
            g_proxy->Stop();
            g_proxy->Join();
        }

        // shut down queues
        sQueueMgr->Stop();
        sQueueMgr->Join();

        exit(0);
    }
}

int main(int argc, char** argv)
{
    signal(SIGINT, &sighandler);

    if (!sConfig->Load())
        return 1;

    // setup proxy
    g_proxy = std::make_unique<ProxyListener>(sConfig->GetStringValue(ConfigKeys::PROXY_BIND), (uint16_t)sConfig->GetIntValue(ConfigKeys::PROXY_PORT));
    if (!g_proxy->Init())
        return 2;

    // start queue manager and proxy
    sQueueMgr->Start();
    g_proxy->Start();

    // loop indefinitelly - the main work is done in queue mgr and proxy threads
    while (true)
        std::this_thread::sleep_for(std::chrono::seconds(3600));

    return 0;
}
