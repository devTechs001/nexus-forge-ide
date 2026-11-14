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
    std::scoped_lock lock(handlerMutex_, queueMutex_);
    handlers_.clear();
    eventQueue_ = std::queue<std::unique_ptr<Event>>();
    deferredEvents_.clear();
}

void EventSystem::queueEvent(std::unique_ptr<Event> event) {
    if (!event) return;
    std::lock_guard<std::mutex> lock(queueMutex_);
    event->timestamp = std::chrono::duration<double>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
    eventQueue_.push(std::move(event));
}

void EventSystem::dispatch(Event& event) {
    // Copy matching handlers under lock so callbacks can subscribe/unsubscribe/dispatch without deadlock
    std::vector<HandlerEntry> snapshot;
    {
        std::lock_guard<std::mutex> lock(handlerMutex_);
        auto it = handlers_.find(event.getType());
        if (it != handlers_.end()) {
            snapshot = it->second;
        }
    }

    for (auto& entry : snapshot) {
        if (event.handled) break;
        try {
            if (entry.handler) {
                entry.handler(event);
            }
        } catch (const std::exception&) {
            // Log exception
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
    // Drain queue under lock, then dispatch unlocked to prevent re-entrant deadlocks
    std::queue<std::unique_ptr<Event>> localQueue;
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        std::swap(localQueue, eventQueue_);
    }

    while (!localQueue.empty()) {
        auto event = std::move(localQueue.front());
        localQueue.pop();

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
    if (!event) return;
    std::lock_guard<std::mutex> lock(queueMutex_);
    deferredEvents_.push_back({ std::move(event), delay });
}

void EventSystem::updateDeferred(double deltaTime) {
    std::vector<std::unique_ptr<Event>> readyEvents;
    {
        std::lock_guard<std::mutex> lock(queueMutex_);

        for (auto it = deferredEvents_.begin(); it != deferredEvents_.end(); ) {
            it->remainingTime -= deltaTime;

            if (it->remainingTime <= 0.0) {
                if (it->event) {
                    readyEvents.push_back(std::move(it->event));
                }
                it = deferredEvents_.erase(it);
            } else {
                ++it;
            }
        }
    }

    for (auto& ev : readyEvents) {
        if (ev) {
            dispatch(*ev);
        }
    }
}

} // namespace NexusForge::Core
