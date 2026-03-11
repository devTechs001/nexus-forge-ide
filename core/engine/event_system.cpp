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
    // Reserve space for common event types
    handlers_.reserve(32);
    return true;
}

void EventSystem::shutdown() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    eventQueue_ = std::queue<std::unique_ptr<Event>>();
    deferredEvents_.clear();
}

void EventSystem::dispatch(Event& event) {
    std::lock_guard<std::mutex> lock(handlerMutex_);

    auto it = handlers_.find(event.getType());
    if (it == handlers_.end()) {
        return;
    }

    // Set timestamp
    event.timestamp = std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();

    // Call handlers in priority order
    for (auto& entry : it->second) {
        if (event.handled) break;
        try {
            entry.handler(event);
        } catch (const std::exception& e) {
            // Log exception
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
        std::swap(localQueue, eventQueue_);
    }

    while (!localQueue.empty()) {
        dispatch(*localQueue.front());
        localQueue.pop();
    }

    // Process deferred events
    updateDeferred(0.016); // Assume ~60fps
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

void EventSystem::unsubscribe(size_t handlerId) {
    std::lock_guard<std::mutex> lock(handlerMutex_);

    for (auto& [type, handlers] : handlers_) {
        auto it = std::find_if(handlers.begin(), handlers.end(),
            [handlerId](const HandlerEntry& entry) {
                return entry.id == handlerId;
            });

        if (it != handlers.end()) {
            handlers.erase(it);
            return;
        }
    }
}

} // namespace NexusForge::Core
