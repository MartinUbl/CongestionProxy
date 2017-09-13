/* * * * * * * * * * * * * * * * *
 * Congestion proxy application  *
 * Author: Martin Ubl            *
 *         kenny@cetes.cz        *
 * * * * * * * * * * * * * * * * */

#pragma once

#include <queue>

// config string formatter for message count
constexpr const char* StrMessageCountFormatter = "$N";
// config string formatter for recipient address
constexpr const char* StrRecipientFormatter = "$R";
// config string formatter for message parameter
constexpr const char* StrMessageFormatter = "$M";

// class representing single recipient queue
class Queue
{
    public:
        // default constructor - recipient may be supplier, and/or later changes using SetRecipient
        Queue(std::string recipient = "");
        virtual ~Queue();

        // sets new recipient of this queue
        void SetRecipient(std::string recipient);

        // updates queue, returns true if everything's OK, returns false as flag for QueueManager to tear down queue
        bool Update();
        // posts new message to queue
        void Post(std::string& message);
        // is queue empty?
        bool Empty() const;

    protected:
        // is congestion phase active?
        bool IsCongestion() const;
        // flushes queue, sends messages as one mass message if specified
        void Flush(bool send = true);

    private:
        // stored recipient of this queue
        std::string m_recipient;
        // timestamps of messages that came in time; holds just single time window of <message-rate-interval> seconds
        std::queue<time_t> m_msgTimestamps;
        // messages held back due to congestion
        std::list<std::string> m_congestionMessages;

        // are we in congestion phase?
        bool m_congestionMode;
        // count of grouped messages during current congestion
        size_t m_congestionGroupCount;
        // time of congestion detection
        time_t m_congestionDetectedAt;
        // time of next flush during congestion phase
        time_t m_congestionNextFlush;

        // mutex for exclusive access to queue contents
        mutable std::mutex m_accessMtx;
};
