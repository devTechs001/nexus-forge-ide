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
    nextHandlerId_ = 1;
    handlers_.clear();
    eventQueue_ = std::queue<std::unique_ptr<Event>>();
    deferredEvents_.clear();
    return true;
}

void EventSystem::shutdown() {
    std::lock_guard<std::mutex> lock(handlerMutex_);
    handlers_.clear();
    
    std::lock_guard<std::mutex> queueLock(queueMutex_);
    eventQueue_ = std::queue<std::unique_ptr<Event>>();
    deferredEvents_.clear();
}

void EventSystem::queueEvent(std::unique_ptr<Event> event) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    event->timestamp = std::chrono::duration<double>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
    eventQueue_.push(std::move(event));
}

void EventSystem::dispatch(Event& event) {
    std::lock_guard<std::mutex> lock(handlerMutex_);
    
    auto it = handlers_.find(event.getType());
    if (it != handlers_.end()) {
        for (auto& entry : it->second) {
            if (event.handled) break;
            try {
                entry.handler(event);
            } catch (const std::exception& e) {
                // Log exception
            }
        }
    }
}

void EventSystem::dispatchImmediate(Event& event) {
    event.timestamp = std::chrono::duration<double>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
    dispatch(event);
}

void EventSystem::processEvents() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    while (!eventQueue_.empty()) {
        auto event = std::move(eventQueue_.front());
        eventQueue_.pop();
        
        if (event) {
            dispatch(*event);
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

void EventSystem::deferEvent(std::unique_ptr<Event> event, double delay) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    deferredEvents_.push_back({ std::move(event), delay });
}

void EventSystem::updateDeferred(double deltaTime) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    for (auto it = deferredEvents_.begin(); it != deferredEvents_.end(); ) {
        it->remainingTime -= deltaTime;
        
        if (it->remainingTime <= 0) {
            if (it->event) {
                dispatch(*it->event);
            }
            it = deferredEvents_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace NexusForge::Core
