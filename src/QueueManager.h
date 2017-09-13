/* * * * * * * * * * * * * * * * *
 * Congestion proxy application  *
 * Author: Martin Ubl            *
 *         kenny@cetes.cz        *
 * * * * * * * * * * * * * * * * */

#pragma once

#include "Singleton.h"
#include "Queue.h"

// amount of milliseconds between queue updates
constexpr long long QueueUpdateDelayMs = 5000;

// queue manager singleton class holding and updating queues of all recipients
class QueueManager
{
    public:
        QueueManager();
        virtual ~QueueManager();

        // retrieves queue for recipient; creates one if not present
        Queue& GetQueue(std::string recipient);

        // starts queue manager thread
        void Start();
        // stops queue manager
        void Stop();
        // joins queue manager thread
        void Join();

    protected:
        // thread function
        void _Run();

    private:
        // map of all queues
        std::map<std::string, Queue> m_queueMap;

        // queue manager thread
        std::unique_ptr<std::thread> m_thread;
        // is queue manager running?
        bool m_running;

        // queue update mutex; so the queue map is in consistent state
        mutable std::mutex m_queueUpdateMtx;
        // condition variable to sleep onto - also allows interrupting queue manager updates to exit program properly
        mutable std::condition_variable m_queueUpdateCv;
};

#define sQueueMgr Singleton<QueueManager>::getInstance()
