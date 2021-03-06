//
// Created by borelset on 1/10/18.
//

#ifndef LIBEP_EP_EVENTMANAGER_H
#define LIBEP_EP_EVENTMANAGER_H

#include <bits/unique_ptr.h>
#include <vector>
#include <functional>
#include "TimerQueue.h"
#include "../Utils/Mutex.h"


namespace ep{
    class Channel;
    class EpollHandler;

    class EventManager : Utils::noncopyable{
    public:
        typedef std::vector<Channel*> ChannelList;
        typedef std::function<void()> Callback;
        typedef std::vector<Callback> CallbackList;
        EventManager();
        ~EventManager();
        void loop();
        void updateChannel(Channel*);
        void removeChannel(Channel*);
        void setQuit();
        std::weak_ptr<Timer> runAt(std::function<void()> Callback, time_t when, int interval);
        std::weak_ptr<Timer> runAfter(std::function<void()> Callback, time_t when, int interval);
        bool isLocalThread();
        void runInLoop(const Callback& callback);
	void stopTimer(std::weak_ptr<Timer>);
    private:
        std::unique_ptr<EpollHandler> mEpollHandlerPtr;
        bool mQuit;
        TimerQueue mTimerQueue;
        const pid_t mThreadId;
        CallbackList mCallbackQueue;
        Utils::MutexLock mMutexLock;
        Utils::FD mEventFd;
        Channel mWakeupChannel;
        bool mEventfdCallbackProcessing;

        void queueInLoop(const Callback& callback);
        void handleRead();
        void eventfdCallbackProcess();
        void wakeup();
    };
}


#endif //LIBEP_EP_EVENTMANAGER_H
