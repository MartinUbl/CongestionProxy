/* * * * * * * * * * * * * * * * *
 * Congestion proxy application  *
 * Author: Martin Ubl            *
 *         kenny@cetes.cz        *
 * * * * * * * * * * * * * * * * */

#include "general.h"
#include "QueueManager.h"

QueueManager::QueueManager()
{
    m_running = false;
}

QueueManager::~QueueManager()
{
}

Queue& QueueManager::GetQueue(std::string recipient)
{
    std::unique_lock<std::mutex> lck(m_queueUpdateMtx);

    // no queue present - construct one
    if (m_queueMap.find(recipient) == m_queueMap.end())
        m_queueMap[recipient].SetRecipient(recipient);

    return m_queueMap[recipient];
}

void QueueManager::Start()
{
    if (m_running)
        return;

    std::cout << "Starting congestion queue manager." << std::endl;
    m_thread = std::make_unique<std::thread>(&QueueManager::_Run, this);
}

void QueueManager::Stop()
{
    if (!m_running)
        return;

    std::unique_lock<std::mutex> lck(m_queueUpdateMtx);

    m_running = false;

    // wake up queue manager thread
    m_queueUpdateCv.notify_all();
}

void QueueManager::Join()
{
    if (m_thread && m_thread->joinable())
        m_thread->join();
}

void QueueManager::_Run()
{
    m_running = true;

    while (m_running)
    {
        std::unique_lock<std::mutex> lck(m_queueUpdateMtx);

        std::list<std::string> todelete;

        // update all queues, store queues to be deleted
        for (auto& q : m_queueMap)
        {
            if (!q.second.Update())
                todelete.push_back(q.first);
        }

        // delete stored queues, which returned "false" from their update method
        for (auto& rec : todelete)
            m_queueMap.erase(rec);

        // wait for specified time
        m_queueUpdateCv.wait_for(lck, std::chrono::milliseconds(QueueUpdateDelayMs));
    }
}
