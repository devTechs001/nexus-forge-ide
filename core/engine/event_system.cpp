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
    eventQueue_.swap(std::queue<std::unique_ptr<Event>>());
    deferredEvents_.clear();
}

void EventSystem::dispatch(Event& event) {
    std::lock_guard<std::mutex> lock(handlerMutex_);
    
    event.timestamp = std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
    
    auto it = handlers_.find(event.getType());
    if (it != handlers_.end()) {
        for (auto& entry : it->second) {
            if (event.handled) break;
            event.handled = entry.handler(event);
        }
    }
}

void EventSystem::dispatchImmediate(Event& event) {
    dispatch(event);
}

void EventSystem::queueEvent(std::unique_ptr<Event> event) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    eventQueue_.push(std::move(event));
}

void EventSystem::processEvents() {
    std::queue<std::unique_ptr<Event>> localQueue;
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        eventQueue_.swap(localQueue);
    }
    
    while (!localQueue.empty()) {
        Event* event = localQueue.front().get();
        dispatch(*event);
        localQueue.pop();
    }
    
    // Process deferred events
    updateDeferred(0.016);  // Assume ~60fps
}

void EventSystem::deferEvent(std::unique_ptr<Event> event, double delay) {
    std::lock_guard<std::mutex> lock(handlerMutex_);
    deferredEvents_.push_back({
        std::move(event),
        delay
    });
}

void EventSystem::updateDeferred(double deltaTime) {
    std::lock_guard<std::mutex> lock(handlerMutex_);
    
    for (auto it = deferredEvents_.begin(); it != deferredEvents_.end();) {
        it->remainingTime -= deltaTime;
        if (it->remainingTime <= 0) {
            dispatch(*it->event);
            it = deferredEvents_.erase(it);
        } else {
            ++it;
        }
    }
}

void EventSystem::unsubscribe(size_t handlerId) {
    std::lock_guard<std::mutex> lock(handlerMutex_);
    
    for (auto& [type, handlers] : handlers_) {
        handlers.erase(
            std::remove_if(handlers.begin(), handlers.end(),
                [handlerId](const HandlerEntry& entry) {
                    return entry.id == handlerId;
                }),
            handlers.end()
        );
    }
}

} // namespace NexusForge::Core
