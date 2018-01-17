//
// Created by borelset on 1/10/18.
//

#include "EventManager.h"
#include "EpollHandler.h"
#include "TimerQueue.h"
#include "../Utils/CurrentThread.h"
#include <sys/eventfd.h>

using namespace ep;

EventManager::EventManager():
        mEpollHandlerPtr(new EpollHandler()),
        mQuit(true),
        mTimerQueue(this),
        mThreadId(Utils::CurrentThread::gettid()),
        mEventFd(eventfd(0, EFD_NONBLOCK|EFD_CLOEXEC)),
        mWakeupChannel(this, mEventFd.getFd()),
        mEventfdCallbackProcessing(false)
{
    mWakeupChannel.setReadCallback(std::bind(&EventManager::handleRead, this));
    mWakeupChannel.enableRead();
}

EventManager::~EventManager() {

}

void EventManager::loop() {
    ChannelList wakedChannel;
    while(mQuit){
        wakedChannel.clear();
        //-1 for infinity wait
        mEpollHandlerPtr->epoll(-1, &wakedChannel);
        for(auto it = wakedChannel.begin();
                it != wakedChannel.end();
                it++){
            (*it)->process();
        }
        eventfdCallbackProcess();
    }
}

void EventManager::updateChannel(Channel * channel) {
    mEpollHandlerPtr->updateChannel(channel);
}

void EventManager::setQuit() {
    mQuit = false;
    if(!isLocalThread())
        wakeup();
}

void EventManager::runAt(std::function<void()> Callback,
                         time_t when,
                         int interval) {
    time_t time_ = Utils::getTime();
    if(when - time_ > 0)
        mTimerQueue.addTimer(Callback, when-Utils::getTime(), interval);
    else
        std::cout << "EventManger::runAt() invalid param"
                  << std::endl
                  << "when == " << when << " smaller than getTime() == " << time_ << std::endl;
}

void EventManager::runAfter(std::function<void()> Callback,
                            time_t when,
                            int interval) {
    if(when <=0){
        std::cout << "EventManger::runAfter() invalid param"
                  << std::endl
                  << "when == " << when << " smaller than 0" << std::endl;
    }
    mTimerQueue.addTimer(Callback, when, interval);
}

bool EventManager::isLocalThread() {
    bool result = mThreadId == Utils::CurrentThread::gettid();
    if(!result){
        std::cout << "EventManager::runInLoop==>"
                  << "EventManager created in:" << mThreadId << "isLocalThread called in:" << Utils::CurrentThread::gettid() << std::endl;
    }
    return result;
}

void EventManager::runInLoop(const EventManager::Callback &callback) {
    if(isLocalThread()){
        callback();
    }else{
        std::cout << "EventManager::runInLoop==>"
                  << "called by foreign thread:" << Utils::CurrentThread::gettid() << std::endl;
        queueInLoop(callback);
    }
}

void EventManager::queueInLoop(const EventManager::Callback &callback) {
    {
        Utils::MutexLockGuard localGuard(mMutexLock);
        mCallbackQueue.push_back(callback);
    }

    if(mEventfdCallbackProcessing || !isLocalThread())
        wakeup();
}

void EventManager::handleRead() {
    uint64_t many;
    read(mEventFd.getFd(), &many, sizeof many);
    std::cout << "EventManager::handleRead==>"
              << "Eventfd found " << many << " item(s)" << std::endl;
}

void EventManager::wakeup() {
    uint64_t one = 1;
    write(mEventFd.getFd(), &one, sizeof one);
    std::cout << "EventManager::wakeup==>"
              << "send a signal to wakeup" << std::endl;
}

void EventManager::eventfdCallbackProcess() {
    std::cout << "EventManager::eventfdCallbackProcess==>"
              << std::endl;
    mEventfdCallbackProcessing = true;
    CallbackList callbackList;
    {
        Utils::MutexLockGuard localGuard(mMutexLock);
        mCallbackQueue.swap(callbackList);
    }

    for(int i=0; i<callbackList.size(); i++){
        callbackList[i]();
    }
    mEventfdCallbackProcessing = false;
}

void EventManager::removeChannel(Channel * channel) {
    std::cout << "ep::EventManager::removeChannel==>"
              << "Channel " << channel->getFd() << " removed" << std::endl;
    mEpollHandlerPtr->removeChannel(channel);
}
