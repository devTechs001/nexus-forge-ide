// core/engine/event_system.cpp
#include "event_system.hpp"
#include <algorithm>
#include <chrono>

namespace NexusForge::Core {

EventSystem::EventSystem() = default;

EventSystem::~EventSystem() {
    shutdown();
}

bool EventSystem::initialize() {
    return true;
}

void EventSystem::shutdown() {
    std::lock_guard<std::mutex> lock(handlerMutex_);
    handlers_.clear();
    
    std::lock_guard<std::mutex> queueLock(queueMutex_);
    eventQueue_ = std::queue<std::unique_ptr<Event>>();
    deferredEvents_.clear();
}

void EventSystem::unsubscribe(size_t handlerId) {
    std::lock_guard<std::mutex> lock(handlerMutex_);

    for (auto& [type, handlerList] : handlers_) {
        handlerList.erase(
            std::remove_if(handlerList.begin(), handlerList.end(),
                [handlerId](const HandlerEntry& entry) {
                    return entry.id == handlerId;
                }),
            handlerList.end()
        );
    }
}

void EventSystem::dispatch(Event& event) {
    std::lock_guard<std::mutex> lock(handlerMutex_);

    auto it = handlers_.find(event.getType());
    if (it != handlers_.end()) {
        for (const auto& entry : it->second) {
            if (event.handled) break;
            entry.handler(event);
        }
    }
}

void EventSystem::dispatchImmediate(Event& event) {
    event.timestamp = std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
    dispatch(event);
}

void EventSystem::queueEvent(std::unique_ptr<Event> event) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    event->timestamp = std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
    eventQueue_.push(std::move(event));
}

void EventSystem::processEvents() {
    std::vector<std::unique_ptr<Event>> eventsToProcess;

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        while (!eventQueue_.empty()) {
            eventsToProcess.push_back(std::move(eventQueue_.front()));
            eventQueue_.pop();
        }
    }

    for (auto& event : eventsToProcess) {
        dispatch(*event);
    }
}

void EventSystem::deferEvent(std::unique_ptr<Event> event, double delay) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    deferredEvents_.push_back({
        std::move(event),
        delay
    });
}

void EventSystem::updateDeferred(double deltaTime) {
    std::lock_guard<std::mutex> lock(queueMutex_);

    for (auto it = deferredEvents_.begin(); it != deferredEvents_.end();) {
        it->remainingTime -= deltaTime;
        if (it->remainingTime <= 0) {
            eventQueue_.push(std::move(it->event));
            it = deferredEvents_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace NexusForge::Core
