// debugger/breakpoint_manager.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>

namespace NexusForge::Debugger {

// Breakpoint types
enum class BreakpointType {
    Line,
    Function,
    Conditional,
    Data,
    Exception,
    Instruction
};

// Breakpoint state
enum class BreakpointState {
    Pending,
    Verified,
    Unverified,
    Disabled
};

// Breakpoint hit condition
enum class HitCondition {
    Always,
    Equal,
    GreaterEqual,
    LessEqual
};

// Breakpoint
struct Breakpoint {
    int id = 0;
    BreakpointType type = BreakpointType::Line;
    BreakpointState state = BreakpointState::Pending;

    // Location
    std::string source;
    std::string filePath;
    std::string uri;
    int line = 0;
    int column = 0;
    std::string functionName;
    std::string instructionAddress;

    // Conditions
    std::string condition;
    int hitCondition = 0;
    std::string hitConditionExpr;
    HitCondition hitConditionType = HitCondition::Always;
    std::string logMessage;
    bool logBreakpoint = false;

    // Exception
    std::string exceptionType;
    bool breakOnThrow = false;

    // State
    bool enabled = true;
    int hitCount = 0;
    bool verified = false;
    std::string message;

    // Visual
    int lineNumber = 0;
};

// Breakpoint manager
class BreakpointManager {
public:
    BreakpointManager();
    ~BreakpointManager();

    // Breakpoint management
    Breakpoint* addBreakpoint(const std::string& filePath, int line,
                              const std::string& condition = "",
                              int hitCondition = 0,
                              bool enabled = true);
    int addConditionalBreakpoint(const std::string& source, int line,
                                  const std::string& condition);
    int addFunctionBreakpoint(const std::string& functionName);
    int addExceptionBreakpoint(const std::string& exceptionType, bool breakOnThrow = true);

    void removeBreakpoint(int id);
    void removeBreakpoint(const std::string& filePath, int line);
    void removeAllBreakpoints();
    void removeBreakpointsInFile(const std::string& filePath);
    void enableBreakpoint(int id, bool enabled = true);
    bool updateBreakpoint(int id, const std::string& condition = "",
                          const std::string& hitCondition = "");

    // Toggle
    void toggleBreakpoint(int id);
    int toggleBreakpoint(const std::string& source, int line);

    // Conditions & hits
    void setBreakpointCondition(int id, const std::string& condition);
    void setBreakpointHitCondition(int id, int hitCondition);
    void incrementHitCount(int id);
    void recordHit(int id) { incrementHitCount(id); }
    bool shouldBreak(int id) const;
    int getHitCount(int id) const;
    void resetHitCounts();

    // Lookup
    Breakpoint* getBreakpoint(int id);
    const Breakpoint* getBreakpoint(int id) const;
    Breakpoint* getBreakpointAt(const std::string& filePath, int line);

    const std::vector<Breakpoint>& getBreakpoints() const;
    std::vector<Breakpoint> getBreakpointsInFile(const std::string& filePath) const;
    std::vector<Breakpoint> getEnabledBreakpoints() const;
    std::vector<Breakpoint*> getBreakpointsForSource(const std::string& source);
    std::vector<const Breakpoint*> getBreakpointsForSource(const std::string& source) const;

    // Verification
    void verifyBreakpoint(int id, bool verified, const std::string& message = "");
    void setActualLocation(int id, int actualLine, int actualColumn);

    // Persistence
    void loadBreakpoints(const std::string& filePath);
    void saveBreakpoints(const std::string& filePath);
    size_t getBreakpointCount() const;
    size_t getEnabledBreakpointCount() const;

    // Clear
    void clearAllBreakpoints();
    void clearBreakpointsForSource(const std::string& source);
    void clearFunctionBreakpoints();
    void clearExceptionBreakpoints();

    // Export/Import
    std::string exportBreakpoints() const;
    bool importBreakpoints(const std::string& json);

    // Events
    using BreakpointCallback = std::function<void(const Breakpoint&)>;
    BreakpointCallback onBreakpointAdded;
    BreakpointCallback onBreakpointRemoved;
    BreakpointCallback onBreakpointChanged;

    void addBreakpointAddedListener(BreakpointCallback callback);
    void addBreakpointRemovedListener(BreakpointCallback callback);
    void addBreakpointChangedListener(BreakpointCallback callback);

private:
    std::vector<Breakpoint> breakpoints_;
    int nextBreakpointId_ = 1;

    std::vector<BreakpointCallback> addedListeners_;
    std::vector<BreakpointCallback> removedListeners_;
    std::vector<BreakpointCallback> changedListeners_;

    void notifyAdded(const Breakpoint& bp);
    void notifyRemoved(const Breakpoint& bp);
    void notifyChanged(const Breakpoint& bp);

    bool evaluateCondition(const Breakpoint& bp) const;
    bool evaluateHitCondition(const Breakpoint& bp) const;
};

// Breakpoint marker for UI
struct BreakpointMarker {
    int breakpointId;
    int lineNumber;
    bool enabled;
    bool verified;
    bool conditional;
    bool hit;
};

} // namespace NexusForge::Debugger
