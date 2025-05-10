#include "SymbolTable.h"
#include <sstream>

// Symbol implementation
Symbol::Symbol(const std::string& name, SymbolKind kind, const std::string& type, 
               int line, int column, const std::string& filename,
               bool isDefined, Scope* scope)
    : name(name), kind(kind), type(type), line(line), column(column), 
      filename(filename), isDefined(isDefined), isUsed(false), scope(scope),
      isStatic(false), isConst(false), isPublic(false), isProtected(false), 
      isPrivate(false), isExported(false), isImported(false) {}

std::string Symbol::toString() const {
    std::stringstream ss;
    
    // Basic symbol info
    ss << "Symbol: " << name;
    
    // Kind info
    ss << " (";
    switch (kind) {
        case SymbolKind::VARIABLE: ss << "Variable"; break;
        case SymbolKind::FUNCTION: ss << "Function"; break;
        case SymbolKind::CLASS: ss << "Class"; break;
        case SymbolKind::STRUCT: ss << "Struct"; break;
        case SymbolKind::ENUM: ss << "Enum"; break;
        case SymbolKind::TYPEDEF: ss << "Typedef"; break;
        case SymbolKind::NAMESPACE: ss << "Namespace"; break;
        case SymbolKind::PARAMETER: ss << "Parameter"; break;
        case SymbolKind::TEMPLATE: ss << "Template"; break;
        case SymbolKind::MACRO: ss << "Macro"; break;
        case SymbolKind::LABEL: ss << "Label"; break;
        case SymbolKind::MODULE: ss << "Module"; break;
        case SymbolKind::UNKNOWN: ss << "Unknown"; break;
    }
    ss << ")";
    
    // Type info if available
    if (!type.empty()) {
        ss << " Type: " << type;
    }
    
    // Location
    ss << " at " << filename << ":" << line << ":" << column;
    
    // Definition/use status
    ss << " [" << (isDefined ? "Defined" : "Declared");
    ss << ", " << (isUsed ? "Used" : "Unused") << "]";
    
    // Attributes
    bool hasAttributes = false;
    ss << " Attributes: [";
    
    if (isStatic) {
        ss << "static";
        hasAttributes = true;
    }
    
    if (isConst) {
        if (hasAttributes) ss << ", ";
        ss << "const";
        hasAttributes = true;
    }
    
    if (isPublic) {
        if (hasAttributes) ss << ", ";
        ss << "public";
        hasAttributes = true;
    }
    
    if (isProtected) {
        if (hasAttributes) ss << ", ";
        ss << "protected";
        hasAttributes = true;
    }
    
    if (isPrivate) {
        if (hasAttributes) ss << ", ";
        ss << "private";
        hasAttributes = true;
    }
    
    if (isExported) {
        if (hasAttributes) ss << ", ";
        ss << "exported";
        hasAttributes = true;
    }
    
    if (isImported) {
        if (hasAttributes) ss << ", ";
        ss << "imported";
        hasAttributes = true;
    }
    
    if (!hasAttributes) {
        ss << "none";
    }
    
    ss << "]";
    
    return ss.str();
}

// Scope implementation
Scope::Scope(const std::string& name, ScopeType type, 
             int startLine, int startColumn, 
             const std::string& filename, Scope* parent)
    : name(name), type(type), startLine(startLine), startColumn(startColumn),
      endLine(-1), endColumn(-1), filename(filename), parent(parent) {}

Scope* Scope::createChildScope(const std::string& name, ScopeType type, 
                              int startLine, int startColumn) {
    auto childScope = std::make_unique<Scope>(name, type, startLine, startColumn, filename, this);
    Scope* childPtr = childScope.get();
    children.push_back(std::move(childScope));
    return childPtr;
}

Symbol* Scope::addSymbol(const std::string& name, SymbolKind kind, const std::string& type, 
                        int line, int column, bool isDefined) {
    auto symbol = std::make_unique<Symbol>(name, kind, type, line, column, filename, isDefined, this);
    Symbol* symbolPtr = symbol.get();
    symbols[name] = std::move(symbol);
    return symbolPtr;
}

Symbol* Scope::findSymbolInScope(const std::string& name) const {
    auto it = symbols.find(name);
    if (it != symbols.end()) {
        return it->second.get();
    }
    return nullptr;
}

Symbol* Scope::findSymbol(const std::string& name) {
    Symbol* symbol = findSymbolInScope(name);
    if (symbol) {
        return symbol;
    }
    
    // Look in child scopes
    for (auto& child : children) {
        symbol = child->findSymbol(name);
        if (symbol) {
            return symbol;
        }
    }
    
    return nullptr;
}

Symbol* Scope::lookup(const std::string& name) {
    // First check this scope
    Symbol* symbol = findSymbolInScope(name);
    if (symbol) {
        return symbol;
    }
    
    // Then check parent scopes
    if (parent) {
        return parent->lookup(name);
    }
    
    return nullptr;
}

std::string Scope::toString() const {
    std::stringstream ss;
    
    // Basic scope info
    ss << "Scope: " << name << " (";
    
    // Type info
    switch (type) {
        case ScopeType::GLOBAL: ss << "Global"; break;
        case ScopeType::FILE: ss << "File"; break;
        case ScopeType::NAMESPACE: ss << "Namespace"; break;
        case ScopeType::CLASS: ss << "Class"; break;
        case ScopeType::FUNCTION: ss << "Function"; break;
        case ScopeType::BLOCK: ss << "Block"; break;
        case ScopeType::CONDITIONAL: ss << "Conditional"; break;
        case ScopeType::LOOP: ss << "Loop"; break;
        case ScopeType::TRY_CATCH: ss << "Try/Catch"; break;
        case ScopeType::UNDEFINED: ss << "Undefined"; break;
    }
    ss << ")";
    
    // Location
    ss << " at " << filename << ":" << startLine << ":" << startColumn;
    if (endLine != -1) {
        ss << " to " << endLine << ":" << endColumn;
    }
    
    // Symbol count
    ss << " [" << symbols.size() << " symbols]";
    
    return ss.str();
}

// SymbolTable implementation
SymbolTable::SymbolTable() {
    // Create the global scope
    globalScope = std::make_unique<Scope>("global", ScopeType::GLOBAL, 0, 0, "", nullptr);
    currentScope = globalScope.get();
}

Scope* SymbolTable::createScope(const std::string& name, ScopeType type, 
                               int startLine, int startColumn, 
                               const std::string& filename, Scope* parent) {
    if (!parent) {
        // Create a new file scope
        auto scope = std::make_unique<Scope>(name, type, startLine, startColumn, filename, globalScope.get());
        Scope* scopePtr = scope.get();
        globalScope->getChildren().push_back(std::move(scope));
        return scopePtr;
    } else {
        // Create a child scope under the specified parent
        return parent->createChildScope(name, type, startLine, startColumn);
    }
}

void SymbolTable::registerSymbol(Symbol* symbol) {
    if (symbol) {
        symbolsByName[symbol->getName()].push_back(symbol);
    }
}

std::vector<Symbol*> SymbolTable::findSymbols(const std::string& name) const {
    auto it = symbolsByName.find(name);
    if (it != symbolsByName.end()) {
        return it->second;
    }
    return {};
}

Symbol* SymbolTable::addSymbol(const std::string& name, SymbolKind kind, const std::string& type, 
                              int line, int column, bool isDefined) {
    if (currentScope) {
        Symbol* symbol = currentScope->addSymbol(name, kind, type, line, column, isDefined);
        registerSymbol(symbol);
        return symbol;
    }
    return nullptr;
}

std::string SymbolTable::toString() const {
    std::stringstream ss;
    
    ss << "Symbol Table: " << symbolsByName.size() << " unique symbols\n";
    ss << globalScope->toString() << "\n";
    
    // We could recursively print all scopes and symbols here
    // but that would be very verbose for large programs
    
    return ss.str();
} 