// ai/features/ai_autocomplete.cpp
// AI Autocomplete Implementation

#include "ai_autocomplete.hpp"
#include <algorithm>
#include <chrono>

namespace NexusForge::AI {

AIAutocomplete::AIAutocomplete(ModelManager* modelManager) 
    : modelManager_(modelManager), currentSuggestionIndex_(0) {
}

AIAutocomplete::~AIAutocomplete() = default;

void AIAutocomplete::requestCompletion(const CompletionContext& context) {
    if (!config_.enabled) return;
    
    // Debounce check
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - lastRequestTime_).count();
    
    if (elapsed < config_.debounceMs) {
        return;
    }
    
    lastRequestTime_ = now;
    requestPending_ = true;
    
    // Build completion request
    CompletionRequest request;
    request.prompt = context.prefix;
    request.suffix = context.suffix;
    request.language = context.language;
    request.filePath = context.filePath;
    request.cursorLine = static_cast<int>(context.cursorLine);
    request.cursorColumn = static_cast<int>(context.cursorColumn);
    request.maxTokens = config_.maxSuggestionLength;
    request.temperature = 0.2f;
    
    // Request from model manager
    if (modelManager_) {
        auto future = modelManager_->getCompletions(request);
        
        // Process results asynchronously
        std::thread([this, future = std::move(future)]() mutable {
            auto results = future.get();
            processCompletions(results, {});
            requestPending_ = false;
        }).detach();
    }
}

void AIAutocomplete::cancelPendingRequests() {
    requestPending_ = false;
}

void AIAutocomplete::processCompletions(
    const std::vector<CompletionResult>& results,
    const CompletionContext& context) {
    
    std::lock_guard<std::mutex> lock(suggestionMutex_);
    
    suggestions_.clear();
    
    for (const auto& result : results) {
        InlineSuggestion suggestion;
        suggestion.text = result.text;
        suggestion.displayText = result.displayText;
        suggestion.confidence = result.confidence;
        suggestion.source = "ai";
        suggestion.timestamp = std::chrono::steady_clock::now();
        
        if (suggestion.confidence >= config_.minConfidence) {
            suggestions_.push_back(suggestion);
        }
    }
    
    // Sort by confidence
    std::sort(suggestions_.begin(), suggestions_.end(),
        [](const InlineSuggestion& a, const InlineSuggestion& b) {
            return a.confidence > b.confidence;
        });
    
    // Limit suggestions
    if (suggestions_.size() > config_.maxSuggestions) {
        suggestions_.resize(config_.maxSuggestions);
    }
    
    // Set current suggestion
    if (!suggestions_.empty()) {
        currentSuggestion_ = suggestions_[0];
        currentSuggestionIndex_ = 0;
    }
    
    if (onSuggestionReady) {
        onSuggestionReady(currentSuggestion_);
    }
}

void AIAutocomplete::nextSuggestion() {
    std::lock_guard<std::mutex> lock(suggestionMutex_);
    
    if (suggestions_.empty()) return;
    
    currentSuggestionIndex_ = (currentSuggestionIndex_ + 1) % suggestions_.size();
    currentSuggestion_ = suggestions_[currentSuggestionIndex_];
}

void AIAutocomplete::previousSuggestion() {
    std::lock_guard<std::mutex> lock(suggestionMutex_);
    
    if (suggestions_.empty()) return;
    
    currentSuggestionIndex_ = (currentSuggestionIndex_ - 1 + suggestions_.size()) % suggestions_.size();
    currentSuggestion_ = suggestions_[currentSuggestionIndex_];
}

void AIAutocomplete::acceptSuggestion() {
    std::lock_guard<std::mutex> lock(suggestionMutex_);
    
    if (!currentSuggestion_.text.empty()) {
        recordAcceptance(currentSuggestion_);
        currentSuggestion_ = InlineSuggestion{};
        suggestions_.clear();
        
        if (onSuggestionCleared) {
            onSuggestionCleared();
        }
    }
}

void AIAutocomplete::acceptPartialSuggestion(size_t wordCount) {
    std::lock_guard<std::mutex> lock(suggestionMutex_);
    
    if (currentSuggestion_.text.empty()) return;
    
    // Find word boundaries
    size_t words = 0;
    size_t endPos = 0;
    
    for (size_t i = 0; i < currentSuggestion_.text.length(); ++i) {
        if (std::isspace(currentSuggestion_.text[i])) {
            words++;
            if (words >= wordCount) {
                endPos = i + 1;
                break;
            }
        }
    }
    
    if (endPos == 0) {
        endPos = currentSuggestion_.text.length();
    }
    
    std::string acceptedPart = currentSuggestion_.text.substr(0, endPos);
    recordPartialAcceptance(currentSuggestion_, acceptedPart);
}

void AIAutocomplete::dismissSuggestion() {
    std::lock_guard<std::mutex> lock(suggestionMutex_);
    
    if (!currentSuggestion_.text.empty()) {
        recordRejection(currentSuggestion_);
    }
    
    currentSuggestion_ = InlineSuggestion{};
    suggestions_.clear();
    
    if (onSuggestionCleared) {
        onSuggestionCleared();
    }
}

void AIAutocomplete::recordAcceptance(const InlineSuggestion& suggestion) {
    // Log acceptance for learning
    // In production, would send to telemetry or update local model
}

void AIAutocomplete::recordRejection(const InlineSuggestion& suggestion) {
    // Log rejection for learning
}

void AIAutocomplete::recordPartialAcceptance(const InlineSuggestion& suggestion,
                                              const std::string& acceptedPart) {
    // Log partial acceptance
}

std::string AIAutocomplete::buildPrompt(const CompletionContext& context) {
    std::stringstream prompt;
    
    // Add file context
    prompt << "File: " << context.filePath << "\n";
    prompt << "Language: " << context.language << "\n\n";
    
    // Add prefix code
    prompt << "```" << context.language << "\n";
    prompt << context.prefix;
    prompt << "\n```\n\n";
    
    // Add suffix if available
    if (!context.suffix.empty()) {
        prompt << "After this code:\n";
        prompt << "```" << context.language << "\n";
        prompt << context.suffix;
        prompt << "\n```\n\n";
    }
    
    prompt << "Complete the code naturally:";
    
    return prompt.str();
}

std::vector<std::string> AIAutocomplete::gatherRelevantContext(
    const CompletionContext& context) {
    
    std::vector<std::string> relevantContext;
    
    // Add imported modules
    for (const auto& import : context.importedModules) {
        relevantContext.push_back("import: " + import);
    }
    
    // Add recent files
    for (const auto& file : context.recentFiles) {
        relevantContext.push_back("recent: " + file);
    }
    
    return relevantContext;
}

std::string AIAutocomplete::cleanupSuggestion(const std::string& suggestion,
                                               const CompletionContext& context) {
    std::string result = suggestion;
    
    // Remove leading whitespace that duplicates existing indentation
    size_t prefixEnd = context.prefix.find_last_not_of(" \t");
    size_t existingIndent = context.prefix.length() - prefixEnd - 1;
    
    size_t firstNonSpace = result.find_first_not_of(" \t");
    if (firstNonSpace != std::string::npos && firstNonSpace <= existingIndent) {
        result = result.substr(firstNonSpace);
    }
    
    return result;
}

bool AIAutocomplete::validateSuggestion(const std::string& suggestion,
                                         const CompletionContext& context) {
    // Check for common issues
    if (suggestion.empty()) return false;
    
    // Check for balanced brackets
    int bracketCount = 0;
    int parenCount = 0;
    int braceCount = 0;
    
    for (char c : suggestion) {
        if (c == '(') bracketCount++;
        else if (c == ')') bracketCount--;
        else if (c == '{') parenCount++;
        else if (c == '}') parenCount--;
        else if (c == '[') braceCount++;
        else if (c == ']') braceCount--;
    }
    
    if (bracketCount != 0 || parenCount != 0 || braceCount != 0) {
        return false;  // Unbalanced
    }
    
    return true;
}

float AIAutocomplete::calculateConfidence(const CompletionResult& result,
                                           const CompletionContext& context) {
    float confidence = result.confidence;
    
    // Adjust based on various factors
    if (result.text.length() < 10) {
        confidence *= 0.8f;  // Too short
    }
    
    if (result.text.length() > 500) {
        confidence *= 0.7f;  // Too long
    }
    
    // Boost confidence for common patterns
    if (result.text.find("if (") != std::string::npos ||
        result.text.find("for (") != std::string::npos ||
        result.text.find("while (") != std::string::npos) {
        confidence *= 1.1f;
    }
    
    return std::min(1.0f, confidence);
}

} // namespace NexusForge::AI
