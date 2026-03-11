// debugger/variable_inspector.hpp
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>

namespace NexusForge::Debugger {

// Variable type
enum class VariableType {
    Unknown,
    Primitive,
    String,
    Array,
    Object,
    Function,
    Class,
    Null,
    Undefined,
    Boolean,
    Number,
    Symbol
};

// Variable representation
struct Variable {
    std::string name;
    std::string value;
    std::string type;
    VariableType variableType = VariableType::Unknown;
    std::string memoryReference;
    int variablesReference = 0;  // For child variables
    int namedVariables = 0;
    int indexedVariables = 0;
    bool hasChildren = false;
    std::string evaluateName;
    std::string presentationHint;
    bool readOnly = false;
};

// Scope representation
struct Scope {
    std::string name;
    int variablesReference = 0;
    bool namedVariables = false;
    int indexedVariables = 0;
    bool expensive = false;  // Expensive to compute
    std::string source;
    int line = 0;
    int column = 0;
    int endLine = 0;
    int endColumn = 0;
};

// Watch expression
struct WatchExpression {
    std::string id;
    std::string expression;
    Variable result;
    bool error = false;
    std::string errorMessage;
};

// Call stack frame
struct CallStackFrame {
    int id = 0;
    std::string name;
    std::string source;
    int line = 0;
    int column = 0;
    int endLine = 0;
    int endColumn = 0;
    bool canRestart = false;
    std::string instructionPointerReference;
    std::string moduleId;
    std::string presentationHint;
};

// Variable inspector
class VariableInspector {
public:
    VariableInspector();
    ~VariableInspector();

    // Scopes
    std::vector<Scope> getScopes(int frameId);
    std::vector<Variable> getVariables(int variablesReference,
                                        int start = 0, int count = 0);

    // Evaluate
    Variable evaluate(const std::string& expression, int frameId = 0,
                      const std::string& context = "repl");

    // Watch expressions
    std::string addWatch(const std::string& expression);
    void removeWatch(const std::string& id);
    void updateWatches();
    const std::vector<WatchExpression>& getWatches() const { return watches_; }

    // Call stack
    std::vector<CallStackFrame> getCallStack(int threadId,
                                              int startFrame = 0, int levels = 20);

    // Set variable value
    bool setVariable(int variablesReference, const std::string& name,
                     const std::string& value);

    // Set expression value
    bool setExpression(const std::string& expression, const std::string& value);

    // Data tips (hover)
    Variable getDataTip(const std::string& expression, int frameId = 0);

    // Configuration
    void setMaxStringLength(size_t length) { maxStringLength_ = length; }
    void setMaxArrayItems(size_t count) { maxArrayItems_ = count; }
    void setMaxObjectProperties(size_t count) { maxObjectProperties_ = count; }

private:
    std::vector<WatchExpression> watches_;
    size_t maxStringLength_ = 10000;
    size_t maxArrayItems_ = 100;
    size_t maxObjectProperties_ = 100;

    Variable createPrimitiveVariable(const std::string& name,
                                      const std::string& value,
                                      const std::string& type);
    Variable createObjectVariable(const std::string& name,
                                   int variablesReference);
    Variable createArrayVariable(const std::string& name,
                                  int variablesReference, int size);
};

// Expression evaluator
class ExpressionEvaluator {
public:
    ExpressionEvaluator();
    ~ExpressionEvaluator();

    Variable evaluate(const std::string& expression, int frameId = 0);

    // Type detection
    VariableType detectType(const std::string& value);
    std::string getTypeName(VariableType type);

private:
    Variable evaluateIdentifier(const std::string& name, int frameId);
    Variable evaluateBinaryOp(const std::string& left, const std::string& op,
                              const std::string& right, int frameId);
    Variable evaluateMemberAccess(const std::string& object, const std::string& member,
                                   int frameId);
    Variable evaluateFunctionCall(const std::string& func,
                                   const std::vector<std::string>& args, int frameId);
};

} // namespace NexusForge::Debugger
