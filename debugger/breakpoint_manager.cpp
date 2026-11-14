// debugger/breakpoint_manager.cpp
// Breakpoint Manager Implementation

#include "breakpoint_manager.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace NexusForge::Debugger {

BreakpointManager::BreakpointManager() : nextBreakpointId_(1) {
}

BreakpointManager::~BreakpointManager() = default;

Breakpoint* BreakpointManager::addBreakpoint(const std::string& filePath, int line,
                                              const std::string& condition,
                                              int hitCondition,
                                              bool enabled) {
    Breakpoint bp;
    bp.id = nextBreakpointId_++;
    bp.filePath = filePath;
    bp.source = filePath;
    bp.line = line;
    bp.lineNumber = line;
    bp.condition = condition;
    bp.hitCondition = hitCondition;
    bp.enabled = enabled;
    bp.verified = false;
    bp.hitCount = 0;

    int addedId = bp.id;
    breakpoints_.push_back(bp);

    // Sort by file and line
    std::sort(breakpoints_.begin(), breakpoints_.end(),
        [](const Breakpoint& a, const Breakpoint& b) {
            if (a.filePath != b.filePath) return a.filePath < b.filePath;
            return a.line < b.line;
        });

    notifyAdded(bp);

    return getBreakpoint(addedId);
}

void BreakpointManager::removeBreakpoint(int id) {
    for (auto it = breakpoints_.begin(); it != breakpoints_.end(); ++it) {
        if (it->id == id) {
            Breakpoint removed = *it;
            breakpoints_.erase(it);
            
            if (onBreakpointRemoved) {
                onBreakpointRemoved(removed);
            }
            return;
        }
    }
}

void BreakpointManager::removeBreakpoint(const std::string& filePath, int line) {
    for (auto it = breakpoints_.begin(); it != breakpoints_.end(); ++it) {
        if (it->filePath == filePath && it->line == line) {
            Breakpoint removed = *it;
            breakpoints_.erase(it);
            
            if (onBreakpointRemoved) {
                onBreakpointRemoved(removed);
            }
            return;
        }
    }
}

void BreakpointManager::removeAllBreakpoints() {
    std::vector<Breakpoint> removed = breakpoints_;
    breakpoints_.clear();
    
    for (const auto& bp : removed) {
        if (onBreakpointRemoved) {
            onBreakpointRemoved(bp);
        }
    }
}

void BreakpointManager::removeBreakpointsInFile(const std::string& filePath) {
    for (auto it = breakpoints_.begin(); it != breakpoints_.end();) {
        if (it->filePath == filePath) {
            Breakpoint removed = *it;
            it = breakpoints_.erase(it);
            
            if (onBreakpointRemoved) {
                onBreakpointRemoved(removed);
            }
        } else {
            ++it;
        }
    }
}

Breakpoint* BreakpointManager::getBreakpoint(int id) {
    for (auto& bp : breakpoints_) {
        if (bp.id == id) {
            return &bp;
        }
    }
    return nullptr;
}

const Breakpoint* BreakpointManager::getBreakpoint(int id) const {
    for (const auto& bp : breakpoints_) {
        if (bp.id == id) {
            return &bp;
        }
    }
    return nullptr;
}

Breakpoint* BreakpointManager::getBreakpointAt(const std::string& filePath, int line) {
    for (auto& bp : breakpoints_) {
        if (bp.filePath == filePath && bp.line == line) {
            return &bp;
        }
    }
    return nullptr;
}

const std::vector<Breakpoint>& BreakpointManager::getBreakpoints() const {
    return breakpoints_;
}

std::vector<Breakpoint> BreakpointManager::getBreakpointsInFile(const std::string& filePath) const {
    std::vector<Breakpoint> result;
    
    for (const auto& bp : breakpoints_) {
        if (bp.filePath == filePath) {
            result.push_back(bp);
        }
    }
    
    return result;
}

std::vector<Breakpoint> BreakpointManager::getEnabledBreakpoints() const {
    std::vector<Breakpoint> result;
    
    for (const auto& bp : breakpoints_) {
        if (bp.enabled) {
            result.push_back(bp);
        }
    }
    
    return result;
}

void BreakpointManager::enableBreakpoint(int id, bool enabled) {
    for (auto& bp : breakpoints_) {
        if (bp.id == id) {
            bp.enabled = enabled;
            
            if (onBreakpointChanged) {
                onBreakpointChanged(bp);
            }
            return;
        }
    }
}

void BreakpointManager::toggleBreakpoint(int id) {
    for (auto& bp : breakpoints_) {
        if (bp.id == id) {
            bp.enabled = !bp.enabled;
            
            if (onBreakpointChanged) {
                onBreakpointChanged(bp);
            }
            return;
        }
    }
}

void BreakpointManager::setBreakpointCondition(int id, const std::string& condition) {
    for (auto& bp : breakpoints_) {
        if (bp.id == id) {
            bp.condition = condition;
            
            if (onBreakpointChanged) {
                onBreakpointChanged(bp);
            }
            return;
        }
    }
}

void BreakpointManager::setBreakpointHitCondition(int id, int hitCondition) {
    for (auto& bp : breakpoints_) {
        if (bp.id == id) {
            bp.hitCondition = hitCondition;
            
            if (onBreakpointChanged) {
                onBreakpointChanged(bp);
            }
            return;
        }
    }
}

void BreakpointManager::incrementHitCount(int id) {
    for (auto& bp : breakpoints_) {
        if (bp.id == id) {
            bp.hitCount++;
            return;
        }
    }
}

void BreakpointManager::resetHitCounts() {
    for (auto& bp : breakpoints_) {
        bp.hitCount = 0;
    }
}

bool BreakpointManager::shouldBreak(int id) const {
    const Breakpoint* bp = getBreakpoint(id);
    if (!bp || !bp->enabled) return false;
    
    // Check hit condition
    if (bp->hitCondition > 0) {
        if (bp->hitCount < bp->hitCondition) {
            return false;
        }
    }
    
    // Condition would be evaluated by the debug adapter
    return true;
}

void BreakpointManager::verifyBreakpoint(int id, bool verified, const std::string& message) {
    for (auto& bp : breakpoints_) {
        if (bp.id == id) {
            bp.verified = verified;
            bp.message = message;
            
            if (onBreakpointChanged) {
                onBreakpointChanged(bp);
            }
            return;
        }
    }
}

void BreakpointManager::loadBreakpoints(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return;
    
    std::string line;
    while (std::getline(file, line)) {
        // Parse breakpoint line (simplified format)
        // Format: file:line:condition:hitCondition:enabled
        std::istringstream iss(line);
        std::string token;
        std::vector<std::string> tokens;
        
        while (std::getline(iss, token, ':')) {
            tokens.push_back(token);
        }
        
        if (tokens.size() >= 2) {
            try {
                std::string bpFile = tokens[0];
                int bpLine = std::stoi(tokens[1]);
                std::string condition = tokens.size() > 2 ? tokens[2] : "";
                int hitCondition = tokens.size() > 3 && !tokens[3].empty() ? std::stoi(tokens[3]) : 0;
                bool enabled = tokens.size() <= 4 || tokens[4] == "true";
                addBreakpoint(bpFile, bpLine, condition, hitCondition, enabled);
            } catch (...) {}
        }
    }
}

void BreakpointManager::saveBreakpoints(const std::string& filePath) {
    std::ofstream file(filePath);
    if (!file.is_open()) return;
    
    for (const auto& bp : breakpoints_) {
        file << bp.filePath << ":"
             << bp.line << ":"
             << bp.condition << ":"
             << bp.hitCondition << ":"
             << (bp.enabled ? "true" : "false")
             << "\n";
    }
}

size_t BreakpointManager::getBreakpointCount() const {
    return breakpoints_.size();
}

size_t BreakpointManager::getEnabledBreakpointCount() const {
    size_t count = 0;
    for (const auto& bp : breakpoints_) {
        if (bp.enabled) count++;
    }
    return count;
}

void BreakpointManager::clearAllBreakpoints() {
    breakpoints_.clear();
    nextBreakpointId_ = 1;
}

int BreakpointManager::addConditionalBreakpoint(const std::string& source, int line,
                                                 const std::string& condition) {
    Breakpoint* bp = addBreakpoint(source, line, condition);
    if (bp) {
        bp->type = BreakpointType::Conditional;
        return bp->id;
    }
    return 0;
}

int BreakpointManager::addFunctionBreakpoint(const std::string& functionName) {
    Breakpoint bp;
    bp.id = nextBreakpointId_++;
    bp.type = BreakpointType::Function;
    bp.functionName = functionName;
    breakpoints_.push_back(bp);
    notifyAdded(bp);
    return bp.id;
}

int BreakpointManager::addExceptionBreakpoint(const std::string& exceptionType, bool breakOnThrow) {
    Breakpoint bp;
    bp.id = nextBreakpointId_++;
    bp.type = BreakpointType::Exception;
    bp.exceptionType = exceptionType;
    bp.breakOnThrow = breakOnThrow;
    breakpoints_.push_back(bp);
    notifyAdded(bp);
    return bp.id;
}

bool BreakpointManager::updateBreakpoint(int id, const std::string& condition,
                                          const std::string& hitCondition) {
    Breakpoint* bp = getBreakpoint(id);
    if (!bp) return false;
    bp->condition = condition;
    bp->hitConditionExpr = hitCondition;
    if (!hitCondition.empty()) {
        try { bp->hitCondition = std::stoi(hitCondition); } catch (...) {}
    }
    notifyChanged(*bp);
    return true;
}

int BreakpointManager::toggleBreakpoint(const std::string& source, int line) {
    Breakpoint* existing = getBreakpointAt(source, line);
    if (existing) {
        int id = existing->id;
        removeBreakpoint(id);
        return 0;
    }
    Breakpoint* added = addBreakpoint(source, line);
    return added ? added->id : 0;
}

std::vector<Breakpoint*> BreakpointManager::getBreakpointsForSource(const std::string& source) {
    std::vector<Breakpoint*> result;
    for (auto& bp : breakpoints_) {
        if (bp.filePath == source || bp.source == source) result.push_back(&bp);
    }
    return result;
}

std::vector<const Breakpoint*> BreakpointManager::getBreakpointsForSource(const std::string& source) const {
    std::vector<const Breakpoint*> result;
    for (const auto& bp : breakpoints_) {
        if (bp.filePath == source || bp.source == source) result.push_back(&bp);
    }
    return result;
}

void BreakpointManager::setActualLocation(int id, int actualLine, int actualColumn) {
    Breakpoint* bp = getBreakpoint(id);
    if (!bp) return;
    bp->line = actualLine;
    bp->lineNumber = actualLine;
    bp->column = actualColumn;
    notifyChanged(*bp);
}

int BreakpointManager::getHitCount(int id) const {
    const Breakpoint* bp = getBreakpoint(id);
    return bp ? bp->hitCount : 0;
}

void BreakpointManager::clearBreakpointsForSource(const std::string& source) {
    removeBreakpointsInFile(source);
}

void BreakpointManager::clearFunctionBreakpoints() {
    breakpoints_.erase(std::remove_if(breakpoints_.begin(), breakpoints_.end(),
        [](const Breakpoint& bp) { return bp.type == BreakpointType::Function; }),
        breakpoints_.end());
}

void BreakpointManager::clearExceptionBreakpoints() {
    breakpoints_.erase(std::remove_if(breakpoints_.begin(), breakpoints_.end(),
        [](const Breakpoint& bp) { return bp.type == BreakpointType::Exception; }),
        breakpoints_.end());
}

std::string BreakpointManager::exportBreakpoints() const {
    std::ostringstream ss;
    for (const auto& bp : breakpoints_) {
        ss << bp.filePath << ":" << bp.line << ":" << bp.condition << ":"
           << bp.hitCondition << ":" << (bp.enabled ? "true" : "false") << "\n";
    }
    return ss.str();
}

bool BreakpointManager::importBreakpoints(const std::string& data) {
    std::istringstream iss(data);
    std::string line;
    while (std::getline(iss, line)) {
        std::istringstream lss(line);
        std::string token;
        std::vector<std::string> tokens;
        while (std::getline(lss, token, ':')) tokens.push_back(token);
        if (tokens.size() >= 2) {
            try {
                addBreakpoint(tokens[0], std::stoi(tokens[1]),
                              tokens.size() > 2 ? tokens[2] : "",
                              tokens.size() > 3 && !tokens[3].empty() ? std::stoi(tokens[3]) : 0,
                              tokens.size() <= 4 || tokens[4] == "true");
            } catch (...) {}
        }
    }
    return true;
}

void BreakpointManager::addBreakpointAddedListener(BreakpointCallback callback) {
    if (callback) addedListeners_.push_back(std::move(callback));
}

void BreakpointManager::addBreakpointRemovedListener(BreakpointCallback callback) {
    if (callback) removedListeners_.push_back(std::move(callback));
}

void BreakpointManager::addBreakpointChangedListener(BreakpointCallback callback) {
    if (callback) changedListeners_.push_back(std::move(callback));
}

void BreakpointManager::notifyAdded(const Breakpoint& bp) {
    if (onBreakpointAdded) onBreakpointAdded(bp);
    for (const auto& cb : addedListeners_) if (cb) cb(bp);
}

void BreakpointManager::notifyRemoved(const Breakpoint& bp) {
    if (onBreakpointRemoved) onBreakpointRemoved(bp);
    for (const auto& cb : removedListeners_) if (cb) cb(bp);
}

void BreakpointManager::notifyChanged(const Breakpoint& bp) {
    if (onBreakpointChanged) onBreakpointChanged(bp);
    for (const auto& cb : changedListeners_) if (cb) cb(bp);
}

bool BreakpointManager::evaluateCondition(const Breakpoint& bp) const {
    return bp.condition.empty();
}

bool BreakpointManager::evaluateHitCondition(const Breakpoint& bp) const {
    return bp.hitCondition <= 0 || bp.hitCount >= bp.hitCondition;
}

} // namespace NexusForge::Debugger
