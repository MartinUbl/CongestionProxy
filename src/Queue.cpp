/* * * * * * * * * * * * * * * * *
 * Congestion proxy application  *
 * Author: Martin Ubl            *
 *         kenny@cetes.cz        *
 * * * * * * * * * * * * * * * * */

#include "general.h"
#include "Queue.h"
#include "Config.h"

#include <cstdlib>

Queue::Queue(std::string recipient) : m_recipient(recipient)
{
    m_congestionMode = false;
}

Queue::~Queue()
{
}

void Queue::SetRecipient(std::string recipient)
{
    m_recipient = recipient;
}

bool Queue::Update()
{
    std::unique_lock<std::mutex> lck(m_accessMtx);

    // this will make sure we hold just specified time window in queue
    while (!m_msgTimestamps.empty() && (time(nullptr) - m_msgTimestamps.front() > sConfig->GetIntValue(ConfigKeys::MESSAGE_RATE_INTERVAL)))
        m_msgTimestamps.pop();

    if (m_congestionMode)
    {
        // time to flush?
        if (m_congestionNextFlush <= time(nullptr))
        {
            Flush(true);
            m_congestionGroupCount++;
            m_congestionNextFlush = time(nullptr) + sConfig->GetIntValue(ConfigKeys::GROUPING_DELAY);
        }

        // switch back to normal mode if congestion ended
        if (m_msgTimestamps.size() < (size_t)sConfig->GetIntValue(ConfigKeys::MESSAGE_RATE_LIMIT) /*&& m_congestionGroupCount > 0*/)
        {
            Flush(true);
            m_congestionMode = false;
        }
    }

    return true;
}

bool Queue::IsCongestion() const
{
    // 1) message limit exceeded
    if (m_msgTimestamps.size() >= (size_t)sConfig->GetIntValue(ConfigKeys::MESSAGE_RATE_LIMIT))
        return true;

    // 2) already in congestion mode
    if (m_congestionMode)
        return true;

    return false;
}

void Queue::Flush(bool send)
{
    if (send)
    {
        // retrieve keywords and try to count all keyword presence in all messages
        std::vector<std::string> const& keywords = sConfig->GetKeywords();

        std::vector<size_t> keywordCounts;
        keywordCounts.resize(keywords.size());

        for (size_t i = 0; i < keywordCounts.size(); i++)
            keywordCounts[i] = 0;

        for (size_t i = 0; i < keywords.size(); i++)
        {
            for (std::string& msg : m_congestionMessages)
            {
                if (msg.find(keywords[i].c_str()) != std::string::npos)
                    keywordCounts[i]++;
            }
        }

        size_t pos;

        std::string congMsg = sConfig->GetStringValue(ConfigKeys::CONGESTION_MESSAGE);

        // replace all $N occurences in message with number of messages
        while ((pos = congMsg.find(StrMessageCountFormatter)) != std::string::npos)
            congMsg.replace(congMsg.begin() + pos, congMsg.begin() + pos + 2, std::to_string(m_congestionMessages.size()));

        // prepare output message
        std::ostringstream os;

        os << congMsg;

        for (size_t i = 0; i < keywords.size(); i++)
        {
            if (keywordCounts[i] > 0)
                os << ", " << keywords[i] << ": " << keywordCounts[i];
        }

        std::string cmd = sConfig->GetStringValue(ConfigKeys::PROXY_COMMAND);

        // replace all $R occurences with recipient
        while ((pos = cmd.find(StrRecipientFormatter)) != std::string::npos)
            cmd.replace(cmd.begin() + pos, cmd.begin() + pos + 2, "\"" + m_recipient + "\"");

        // replace all $M occurences with message
        while ((pos = cmd.find(StrMessageFormatter)) != std::string::npos)
            cmd.replace(cmd.begin() + pos, cmd.begin() + pos + 2, "\"" + os.str() + "\"");

        // TODO: use ShellExecute / fork+exec instead of system command
        system(cmd.c_str());
    }

    m_congestionMessages.clear();
}

void Queue::Post(std::string& message)
{
    std::unique_lock<std::mutex> lck(m_accessMtx);

    // push timestamp to queue
    m_msgTimestamps.push(time(nullptr));

    // no congestion = just pass through
    if (!IsCongestion())
    {
        std::string cmd = sConfig->GetStringValue(ConfigKeys::PROXY_COMMAND) + " \"" + m_recipient + "\" \"" + message + "\"";

        // TODO: use ShellExecute / fork+exec instead of system command
        system(cmd.c_str());
    }
    else
    {
        // if we just entered congestion mode, set the indicators and other variables
        if (!m_congestionMode)
        {
            m_congestionMode = true;
            m_congestionGroupCount = 0;
            m_congestionNextFlush = time(nullptr) + sConfig->GetIntValue(ConfigKeys::GROUPING_DELAY);
            m_congestionDetectedAt = time(nullptr);
        }

        // store message to list, so it could be sent later in mass message
        m_congestionMessages.push_back(message);
    }
}

bool Queue::Empty() const
{
    std::unique_lock<std::mutex> lck(m_accessMtx);

    return m_msgTimestamps.size();
}
