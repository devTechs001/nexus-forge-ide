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
    std::string uri;
    int line = 0;
    int column = 0;
    std::string functionName;
    std::string instructionAddress;
    
    // Conditions
    std::string condition;
    std::string hitCondition;
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
    int addBreakpoint(const std::string& source, int line, int column = 0);
    int addConditionalBreakpoint(const std::string& source, int line,
                                  const std::string& condition);
    int addFunctionBreakpoint(const std::string& functionName);
    int addExceptionBreakpoint(const std::string& exceptionType, bool breakOnThrow = true);
    
    bool removeBreakpoint(int id);
    bool enableBreakpoint(int id, bool enabled = true);
    bool updateBreakpoint(int id, const std::string& condition = "",
                          const std::string& hitCondition = "");
    
    // Toggle
    int toggleBreakpoint(const std::string& source, int line);
    
    // Lookup
    Breakpoint* getBreakpoint(int id);
    const Breakpoint* getBreakpoint(int id) const;
    
    std::vector<Breakpoint*> getBreakpointsForSource(const std::string& source);
    std::vector<const Breakpoint*> getBreakpointsForSource(const std::string& source) const;
    
    std::vector<Breakpoint*> getEnabledBreakpoints();
    std::vector<const Breakpoint*> getEnabledBreakpoints() const;
    
    const std::unordered_map<int, Breakpoint>& getAllBreakpoints() const { return breakpoints_; }
    
    // Verification
    void verifyBreakpoint(int id, bool verified, const std::string& message = "");
    void setActualLocation(int id, int actualLine, int actualColumn);
    
    // Hit tracking
    void recordHit(int id);
    bool shouldBreak(int id) const;
    int getHitCount(int id) const;
    void resetHitCounts();
    
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
    void addBreakpointAddedListener(BreakpointCallback callback);
    void addBreakpointRemovedListener(BreakpointCallback callback);
    void addBreakpointChangedListener(BreakpointCallback callback);
    
private:
    std::unordered_map<int, Breakpoint> breakpoints_;
    int nextId_ = 1;
    
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
