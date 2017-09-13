/* * * * * * * * * * * * * * * * *
 * Congestion proxy application  *
 * Author: Martin Ubl            *
 *         kenny@cetes.cz        *
 * * * * * * * * * * * * * * * * */

#pragma once

#include <memory>
#include <mutex>

// singleton template class
template<class T>
class Singleton
{
    private:
        // static instance
        static std::unique_ptr<T> m_instance;
        // call_once flag for instance initialization
        static std::once_flag m_initflag;

    public:
        // static method retrieving static instance; creates one if not exists
        static T* getInstance()
        {
            // create instance if not already created; thread-safe
            std::call_once(Singleton<T>::m_initflag, [=]() {
                Singleton<T>::m_instance = std::make_unique<T>();
            });

            return Singleton<T>::m_instance.get();
        }
};

// instantiation

template<class T>
std::unique_ptr<T> Singleton<T>::m_instance = nullptr;

template<class T>
std::once_flag Singleton<T>::m_initflag;
