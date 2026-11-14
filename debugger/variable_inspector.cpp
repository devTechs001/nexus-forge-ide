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
    try {
        if (value.size() >= 2 && value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) {
            long long val = std::stoll(value.substr(2), nullptr, 16);
            return std::to_string(val);
        }
    } catch (...) {}
    return value;
}

std::string VariableInspector::formatAsHex(const std::string& value) const {
    try {
        long long val;
        if (value.size() >= 2 && value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) {
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
        if (value.size() >= 2 && value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) {
            val = std::stoll(value.substr(2), nullptr, 16);
        } else {
            val = std::stoll(value);
        }

        if (val == 0) {
            return "0b0";
        }
        unsigned long long uval = static_cast<unsigned long long>(val);
        std::string binary;
        while (uval > 0) {
            binary.push_back((uval & 1ULL) ? '1' : '0');
            uval >>= 1;
        }
        std::reverse(binary.begin(), binary.end());
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
    (void)expression;
    return "<evaluate result>";
}

std::vector<Scope> VariableInspector::getScopes(int frameId) {
    (void)frameId;
    Scope local;
    local.name = "Local";
    local.variablesReference = 1;
    return {local};
}

std::vector<Variable> VariableInspector::getVariables(int variablesReference, int start, int count) {
    (void)variablesReference;
    if (start < 0 || static_cast<size_t>(start) >= variables_.size()) return {};
    size_t begin = static_cast<size_t>(start);
    size_t end = (count > 0) ? std::min(variables_.size(), begin + static_cast<size_t>(count)) : variables_.size();
    return std::vector<Variable>(variables_.begin() + begin, variables_.begin() + end);
}

Variable VariableInspector::evaluate(const std::string& expression, int frameId, const std::string& context) {
    (void)frameId;
    (void)context;
    Variable* existing = getVariable(expression);
    if (existing) return *existing;
    Variable v;
    v.name = expression;
    v.value = evaluateExpression(expression);
    return v;
}

std::string VariableInspector::addWatch(const std::string& expression) {
    setWatchExpression(expression);
    return std::to_string(watches_.back().id);
}

void VariableInspector::removeWatch(const std::string& id) {
    try {
        removeWatchExpression(std::stoi(id));
    } catch (...) {}
}

void VariableInspector::updateWatches() {
    for (auto& w : watches_) {
        w.result = evaluate(w.expression);
    }
}

std::vector<CallStackFrame> VariableInspector::getCallStack(int threadId, int startFrame, int levels) {
    (void)threadId; (void)startFrame; (void)levels;
    return {};
}

bool VariableInspector::setVariable(int variablesReference, const std::string& name, const std::string& value) {
    (void)variablesReference;
    Variable* v = getVariable(name);
    if (!v) return false;
    v->value = value;
    return true;
}

bool VariableInspector::setExpression(const std::string& expression, const std::string& value) {
    return setVariable(0, expression, value);
}

Variable VariableInspector::getDataTip(const std::string& expression, int frameId) {
    return evaluate(expression, frameId, "hover");
}

Variable VariableInspector::createPrimitiveVariable(const std::string& name, const std::string& value, const std::string& type) {
    Variable v;
    v.name = name;
    v.value = value;
    v.type = type;
    v.variableType = VariableType::Primitive;
    return v;
}

Variable VariableInspector::createObjectVariable(const std::string& name, int variablesReference) {
    Variable v;
    v.name = name;
    v.type = "object";
    v.variableType = VariableType::Object;
    v.variablesReference = variablesReference;
    v.childrenReference = variablesReference;
    v.hasChildren = variablesReference > 0;
    return v;
}

Variable VariableInspector::createArrayVariable(const std::string& name, int variablesReference, int size) {
    Variable v;
    v.name = name;
    v.type = "array";
    v.variableType = VariableType::Array;
    v.variablesReference = variablesReference;
    v.childrenReference = variablesReference;
    v.indexedVariables = size;
    v.hasChildren = size > 0;
    return v;
}

// ExpressionEvaluator implementation
ExpressionEvaluator::ExpressionEvaluator() = default;
ExpressionEvaluator::~ExpressionEvaluator() = default;

Variable ExpressionEvaluator::evaluate(const std::string& expression, int frameId) {
    return evaluateIdentifier(expression, frameId);
}

VariableType ExpressionEvaluator::detectType(const std::string& value) {
    if (value == "true" || value == "false") return VariableType::Boolean;
    if (value == "null" || value == "nullptr") return VariableType::Null;
    if (value.size() >= 2 && ((value.front() == '"' && value.back() == '"') ||
                              (value.front() == '\'' && value.back() == '\''))) {
        return VariableType::String;
    }
    try {
        size_t idx = 0;
        std::stod(value, &idx);
        if (idx == value.size()) return VariableType::Number;
    } catch (...) {}
    return VariableType::Unknown;
}

std::string ExpressionEvaluator::getTypeName(VariableType type) {
    switch (type) {
        case VariableType::Primitive: return "primitive";
        case VariableType::String: return "string";
        case VariableType::Array: return "array";
        case VariableType::Object: return "object";
        case VariableType::Function: return "function";
        case VariableType::Class: return "class";
        case VariableType::Null: return "null";
        case VariableType::Undefined: return "undefined";
        case VariableType::Boolean: return "boolean";
        case VariableType::Number: return "number";
        case VariableType::Symbol: return "symbol";
        default: return "unknown";
    }
}

Variable ExpressionEvaluator::evaluateIdentifier(const std::string& name, int frameId) {
    (void)frameId;
    Variable v;
    v.name = name;
    v.value = name;
    v.variableType = detectType(name);
    v.type = getTypeName(v.variableType);
    return v;
}

Variable ExpressionEvaluator::evaluateBinaryOp(const std::string& left, const std::string& op,
                                                const std::string& right, int frameId) {
    (void)frameId;
    Variable v;
    v.name = left + " " + op + " " + right;
    return v;
}

Variable ExpressionEvaluator::evaluateMemberAccess(const std::string& object, const std::string& member,
                                                    int frameId) {
    (void)frameId;
    Variable v;
    v.name = object + "." + member;
    return v;
}

Variable ExpressionEvaluator::evaluateFunctionCall(const std::string& func,
                                                    const std::vector<std::string>& args, int frameId) {
    (void)args;
    (void)frameId;
    Variable v;
    v.name = func + "()";
    return v;
}

} // namespace NexusForge::Debugger
