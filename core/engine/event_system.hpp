// core/engine/event_system.hpp
#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <queue>
#include <mutex>
#include <typeindex>
#include <memory>
#include <any>
#include <string>

namespace NexusForge::Core {

// Base Event class
class Event {
public:
    virtual ~Event() = default;
    virtual std::type_index getType() const = 0;
    virtual const char* getName() const = 0;

    bool handled = false;
    double timestamp = 0.0;
};

// Event type registration macro
#define NEXUS_EVENT_TYPE(EventClass) \
    static std::type_index staticType() { return typeid(EventClass); } \
    std::type_index getType() const override { return staticType(); } \
    const char* getName() const override { return #EventClass; }

// Common Events
class WindowResizeEvent : public Event {
public:
    NEXUS_EVENT_TYPE(WindowResizeEvent)

    int width, height;
    WindowResizeEvent(int w, int h) : width(w), height(h) {}
};

class WindowCloseEvent : public Event {
public:
    NEXUS_EVENT_TYPE(WindowCloseEvent)
};

class KeyEvent : public Event {
public:
    int keyCode;
    int scanCode;
    int modifiers;
    bool repeat;

protected:
    KeyEvent(int key, int scan, int mods, bool rep)
        : keyCode(key), scanCode(scan), modifiers(mods), repeat(rep) {}
};

class KeyPressEvent : public KeyEvent {
public:
    NEXUS_EVENT_TYPE(KeyPressEvent)
    KeyPressEvent(int key, int scan, int mods, bool rep = false)
        : KeyEvent(key, scan, mods, rep) {}
};

class KeyReleaseEvent : public KeyEvent {
public:
    NEXUS_EVENT_TYPE(KeyReleaseEvent)
    KeyReleaseEvent(int key, int scan, int mods)
        : KeyEvent(key, scan, mods, false) {}
};

class CharInputEvent : public Event {
public:
    NEXUS_EVENT_TYPE(CharInputEvent)
    uint32_t codepoint;
    CharInputEvent(uint32_t cp) : codepoint(cp) {}
};

class MouseMoveEvent : public Event {
public:
    NEXUS_EVENT_TYPE(MouseMoveEvent)
    double x, y;
    double deltaX, deltaY;
    MouseMoveEvent(double px, double py, double dx = 0, double dy = 0)
        : x(px), y(py), deltaX(dx), deltaY(dy) {}
};

class MouseButtonEvent : public Event {
public:
    int button;
    double x, y;

protected:
    MouseButtonEvent(int btn, double px, double py)
        : button(btn), x(px), y(py) {}
};

class MouseButtonPressEvent : public MouseButtonEvent {
public:
    NEXUS_EVENT_TYPE(MouseButtonPressEvent)
    MouseButtonPressEvent(int btn, double x, double y)
        : MouseButtonEvent(btn, x, y) {}
};

class MouseButtonReleaseEvent : public MouseButtonEvent {
public:
    NEXUS_EVENT_TYPE(MouseButtonReleaseEvent)
    MouseButtonReleaseEvent(int btn, double x, double y)
        : MouseButtonEvent(btn, x, y) {}
};

class ScrollEvent : public Event {
public:
    NEXUS_EVENT_TYPE(ScrollEvent)
    double xOffset, yOffset;
    ScrollEvent(double x, double y) : xOffset(x), yOffset(y) {}
};

// Touch Events (Mobile)
struct TouchPoint {
    int id;
    double x, y;
    double pressure;
    double size;
};

class TouchEvent : public Event {
public:
    NEXUS_EVENT_TYPE(TouchEvent)
    std::vector<TouchPoint> touches;
    enum class Action { Down, Up, Move, Cancel } action;
};

class GestureEvent : public Event {
public:
    NEXUS_EVENT_TYPE(GestureEvent)
    enum class Type { Tap, DoubleTap, LongPress, Swipe, Pinch, Rotate } type;
    double x, y;
    double scale;      // For pinch
    double rotation;   // For rotate
    double velocityX, velocityY;  // For swipe
};

// File Events
class FileDropEvent : public Event {
public:
    NEXUS_EVENT_TYPE(FileDropEvent)
    std::vector<std::string> paths;
};

class FileChangedEvent : public Event {
public:
    NEXUS_EVENT_TYPE(FileChangedEvent)
    std::string path;
    enum class ChangeType { Created, Modified, Deleted, Renamed } changeType;
    std::string oldPath;  // For rename
};

// Editor Events
class TextChangedEvent : public Event {
public:
    NEXUS_EVENT_TYPE(TextChangedEvent)
    std::string documentId;
    size_t startLine, startColumn;
    size_t endLine, endColumn;
    std::string insertedText;
    std::string deletedText;
};

class CursorMovedEvent : public Event {
public:
    NEXUS_EVENT_TYPE(CursorMovedEvent)
    std::string documentId;
    size_t line, column;
};

class SelectionChangedEvent : public Event {
public:
    NEXUS_EVENT_TYPE(SelectionChangedEvent)
    std::string documentId;
    size_t startLine, startColumn;
    size_t endLine, endColumn;
};

// Event Handler
using EventHandler = std::function<bool(Event&)>;

class EventSystem {
public:
    EventSystem();
    ~EventSystem();

    bool initialize();
    void shutdown();

    // Event subscription
    template<typename T>
    size_t subscribe(std::function<bool(T&)> handler, int priority = 0);

    void unsubscribe(size_t handlerId);

    // Event dispatch
    void dispatch(Event& event);
    void dispatchImmediate(Event& event);

    // Queue events for later processing
    void queueEvent(std::unique_ptr<Event> event);
    void processEvents();

    // Deferred events
    void deferEvent(std::unique_ptr<Event> event, double delay);
    void updateDeferred(double deltaTime);

private:
    struct HandlerEntry {
        size_t id;
        int priority;
        EventHandler handler;
        std::type_index eventType;
    };

    struct DeferredEvent {
        std::unique_ptr<Event> event;
        double remainingTime;
    };

    std::unordered_map<std::type_index, std::vector<HandlerEntry>> handlers_;
    std::queue<std::unique_ptr<Event>> eventQueue_;
    std::vector<DeferredEvent> deferredEvents_;

    mutable std::mutex handlerMutex_;
    mutable std::mutex queueMutex_;

    size_t nextHandlerId_ = 1;
};

template<typename T>
size_t EventSystem::subscribe(std::function<bool(T&)> handler, int priority) {
    std::lock_guard<std::mutex> lock(handlerMutex_);

    size_t id = nextHandlerId_++;
    std::type_index type = T::staticType();

    HandlerEntry entry{
        id,
        priority,
        [handler](Event& e) -> bool {
            return handler(static_cast<T&>(e));
        },
        type
    };

    auto& handlerList = handlers_[type];
    handlerList.push_back(std::move(entry));

    // Sort by priority (higher priority first)
    std::sort(handlerList.begin(), handlerList.end(),
              [](const HandlerEntry& a, const HandlerEntry& b) {
                  return a.priority > b.priority;
              });

    return id;
}

} // namespace NexusForge::Core
