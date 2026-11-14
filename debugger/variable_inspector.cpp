// debugger/variable_inspector.cpp
// Variable Inspector Implementation

#include "variable_inspector.hpp"
#include <algorithm>
#include <sstream>
#include <regex>

namespace NexusForge::Debugger {

VariableInspector::VariableInspector() = default;

VariableInspector::~VariableInspector() = default;

void VariableInspector::setVariables(const std::vector<Variable>& variables) {
    variables_ = variables;
    buildVariableTree();
}

void VariableInspector::addVariable(const Variable& var) {
    variables_.push_back(var);
    buildVariableTree();
}

void VariableInspector::updateVariable(const std::string& name, const std::string& value) {
    for (auto& var : variables_) {
        if (var.name == name) {
            var.value = value;
            break;
        }
    }
}

void VariableInspector::removeVariable(const std::string& name) {
    variables_.erase(
        std::remove_if(variables_.begin(), variables_.end(),
            [&name](const Variable& v) { return v.name == name; }),
        variables_.end()
    );
    buildVariableTree();
}

void VariableInspector::clear() {
    variables_.clear();
    variableTree_.clear();
}

const std::vector<Variable>& VariableInspector::getVariables() const {
    return variables_;
}

const std::vector<VariableTreeNode>& VariableInspector::getVariableTree() const {
    return variableTree_;
}

Variable* VariableInspector::getVariable(const std::string& name) {
    for (auto& var : variables_) {
        if (var.name == name) {
            return &var;
        }
    }
    return nullptr;
}

std::vector<Variable> VariableInspector::filterVariables(VariableScope scope) const {
    std::vector<Variable> result;
    
    for (const auto& var : variables_) {
        if (var.scope == scope) {
            result.push_back(var);
        }
    }
    
    return result;
}

std::vector<Variable> VariableInspector::filterByType(const std::string& type) const {
    std::vector<Variable> result;
    
    for (const auto& var : variables_) {
        if (var.type.find(type) != std::string::npos) {
            result.push_back(var);
        }
    }
    
    return result;
}

std::vector<Variable> VariableInspector::searchVariables(const std::string& query) const {
    std::vector<Variable> result;
    
    for (const auto& var : variables_) {
        if (var.name.find(query) != std::string::npos ||
            var.value.find(query) != std::string::npos ||
            var.type.find(query) != std::string::npos) {
            result.push_back(var);
        }
    }
    
    return result;
}

void VariableInspector::expandVariable(const std::string& name) {
    expandedVariables_.insert(name);
    
    // Load child variables if not already loaded
    for (auto& var : variables_) {
        if (var.name == name && var.childrenReference > 0 && var.children.empty()) {
            // In production, would request children from debug adapter
            loadChildren(var);
        }
    }
}

void VariableInspector::collapseVariable(const std::string& name) {
    expandedVariables_.erase(name);
}

bool VariableInspector::isExpanded(const std::string& name) const {
    return expandedVariables_.find(name) != expandedVariables_.end();
}

void VariableInspector::loadChildren(Variable& parent) {
    // In production, would request children from debug adapter
    // using the childrenReference
    
    // Simulated children for common types
    if (parent.type.find("vector") != std::string::npos ||
        parent.type.find("[]") != std::string::npos) {
        // Array-like type
        for (int i = 0; i < 5; ++i) {
            Variable child;
            child.name = "[" + std::to_string(i) + "]";
            child.value = "?";
            child.type = parent.type.substr(0, parent.type.find('['));
            child.scope = VariableScope::Local;
            parent.children.push_back(child);
        }
    } else if (parent.type.find("map") != std::string::npos ||
               parent.type.find("{") != std::string::npos) {
        // Map-like type
        Variable child1;
        child1.name = "[key1]";
        child1.value = "value1";
        parent.children.push_back(child1);
    } else if (parent.type.find("struct") != std::string::npos ||
               parent.type.find("class") != std::string::npos) {
        // Object type - would get members from debug info
    }
}

void VariableInspector::buildVariableTree() {
    variableTree_.clear();
    
    // Group variables by scope
    std::unordered_map<VariableScope, std::vector<Variable>> byScope;
    
    for (const auto& var : variables_) {
        byScope[var.scope].push_back(var);
    }
    
    // Build tree nodes for each scope
    for (const auto& [scope, vars] : byScope) {
        VariableTreeNode scopeNode;
        scopeNode.name = getScopeName(scope);
        scopeNode.expanded = true;
        scopeNode.children = vars;
        
        variableTree_.push_back(scopeNode);
    }
}

std::string VariableInspector::getScopeName(VariableScope scope) const {
    switch (scope) {
        case VariableScope::Local: return "Local";
        case VariableScope::Static: return "Static";
        case VariableScope::Global: return "Global";
        case VariableScope::Argument: return "Arguments";
        case VariableScope::Register: return "Registers";
        case VariableScope::Special: return "Special";
        default: return "Unknown";
    }
}

std::string VariableInspector::formatValue(const Variable& var, ValueFormat format) const {
    switch (format) {
        case ValueFormat::Default:
            return var.value;
            
        case ValueFormat::Decimal:
            return formatAsDecimal(var.value);
            
        case ValueFormat::Hexadecimal:
            return formatAsHex(var.value);
            
        case ValueFormat::Binary:
            return formatAsBinary(var.value);
            
        case ValueFormat::Character:
            return formatAsCharacter(var.value);
            
        default:
            return var.value;
    }
}

std::string VariableInspector::formatAsDecimal(const std::string& value) const {
    // Try to parse and format as decimal
    try {
        if (value.substr(0, 2) == "0x") {
            long long val = std::stoll(value.substr(2), nullptr, 16);
            return std::to_string(val);
        }
    } catch (...) {}
    return value;
}

std::string VariableInspector::formatAsHex(const std::string& value) const {
    try {
        long long val;
        if (value.substr(0, 2) == "0x") {
            val = std::stoll(value.substr(2), nullptr, 16);
        } else {
            val = std::stoll(value);
        }
        
        std::ostringstream oss;
        oss << "0x" << std::hex << val;
        return oss.str();
    } catch (...) {}
    return value;
}

std::string VariableInspector::formatAsBinary(const std::string& value) const {
    try {
        long long val;
        if (value.substr(0, 2) == "0x") {
            val = std::stoll(value.substr(2), nullptr, 16);
        } else {
            val = std::stoll(value);
        }
        
        std::string binary;
        while (val > 0) {
            binary = (val % 2 == 0 ? "0" : "1") + binary;
            val /= 2;
        }
        
        return "0b" + binary;
    } catch (...) {}
    return value;
}

std::string VariableInspector::formatAsCharacter(const std::string& value) const {
    try {
        long long val = std::stoll(value);
        if (val >= 32 && val < 127) {
            return "'" + std::string(1, static_cast<char>(val)) + "'";
        }
    } catch (...) {}
    return value;
}

std::string VariableInspector::getTypeIcon(const std::string& type) const {
    if (type.find("int") != std::string::npos ||
        type.find("float") != std::string::npos ||
        type.find("double") != std::string::npos) {
        return "🔢";
    } else if (type.find("char") != std::string::npos ||
               type.find("string") != std::string::npos) {
        return "🔤";
    } else if (type.find("bool") != std::string::npos) {
        return "⚑";
    } else if (type.find("vector") != std::string::npos ||
               type.find("[]") != std::string::npos) {
        return "📋";
    } else if (type.find("map") != std::string::npos ||
               type.find("{") != std::string::npos) {
        return "🗂";
    } else if (type.find("*") != std::string::npos ||
               type.find("&") != std::string::npos) {
        return "🔗";
    } else if (type.find("struct") != std::string::npos ||
               type.find("class") != std::string::npos) {
        return "📦";
    }
    return "📄";
}

void VariableInspector::setWatchExpression(const std::string& expression) {
    WatchExpression watch;
    watch.expression = expression;
    watch.id = nextWatchId_++;
    watches_.push_back(watch);
}

void VariableInspector::removeWatchExpression(int id) {
    watches_.erase(
        std::remove_if(watches_.begin(), watches_.end(),
            [id](const WatchExpression& w) { return w.id == id; }),
        watches_.end()
    );
}

const std::vector<WatchExpression>& VariableInspector::getWatchExpressions() const {
    return watches_;
}

void VariableInspector::updateWatchExpressions(const std::function<void(WatchExpression&)>& evaluator) {
    for (auto& watch : watches_) {
        evaluator(watch);
    }
}

std::string VariableInspector::evaluateExpression(const std::string& expression) {
    // In production, would send evaluate request to debug adapter
    return "<evaluate result>";
}

} // namespace NexusForge::Debugger
