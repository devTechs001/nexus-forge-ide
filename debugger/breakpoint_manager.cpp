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
    bp.line = line;
    bp.condition = condition;
    bp.hitCondition = hitCondition;
    bp.enabled = enabled;
    bp.verified = false;
    bp.hitCount = 0;
    
    breakpoints_.push_back(bp);
    
    // Sort by file and line
    std::sort(breakpoints_.begin(), breakpoints_.end(),
        [](const Breakpoint& a, const Breakpoint& b) {
            if (a.filePath != b.filePath) return a.filePath < b.filePath;
            return a.line < b.line;
        });
    
    if (onBreakpointAdded) {
        onBreakpointAdded(bp);
    }
    
    return &breakpoints_.back();
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
            std::string bpFile = tokens[0];
            int bpLine = std::stoi(tokens[1]);
            std::string condition = tokens.size() > 2 ? tokens[2] : "";
            int hitCondition = tokens.size() > 3 ? std::stoi(tokens[3]) : 0;
            bool enabled = tokens.size() <= 4 || tokens[4] == "true";
            
            addBreakpoint(bpFile, bpLine, condition, hitCondition, enabled);
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

} // namespace NexusForge::Debugger
