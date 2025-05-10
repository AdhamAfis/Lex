#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

// Symbol kinds
enum class SymbolKind {
    VARIABLE,
    FUNCTION,
    CLASS,
    STRUCT,
    ENUM,
    TYPEDEF,
    NAMESPACE,
    PARAMETER,
    TEMPLATE,
    MACRO,
    LABEL,
    MODULE,
    UNKNOWN
};

// Symbol scope types
enum class ScopeType {
    GLOBAL,
    FILE,
    NAMESPACE,
    CLASS,
    FUNCTION,
    BLOCK,
    CONDITIONAL,
    LOOP,
    TRY_CATCH,
    UNDEFINED
};

// Forward declarations
class Scope;
class Symbol;

// Symbol class - represents a single symbol in the code
class Symbol {
private:
    std::string name;
    SymbolKind kind;
    std::string type;
    int line;
    int column;
    std::string filename;
    bool isDefined;
    bool isUsed;
    Scope* scope; // Parent scope

    // Symbol attributes
    bool isStatic;
    bool isConst;
    bool isPublic;
    bool isProtected;
    bool isPrivate;
    bool isExported;
    bool isImported;
    
public:
    Symbol(const std::string& name, SymbolKind kind, const std::string& type, 
           int line, int column, const std::string& filename,
           bool isDefined = false, Scope* scope = nullptr);
           
    // Accessors
    const std::string& getName() const { return name; }
    SymbolKind getKind() const { return kind; }
    const std::string& getType() const { return type; }
    int getLine() const { return line; }
    int getColumn() const { return column; }
    const std::string& getFilename() const { return filename; }
    bool getIsDefined() const { return isDefined; }
    bool getIsUsed() const { return isUsed; }
    Scope* getScope() const { return scope; }
    
    // Modifiers
    void setDefined(bool defined) { isDefined = defined; }
    void setUsed(bool used) { isUsed = used; }
    void setScope(Scope* newScope) { scope = newScope; }
    void setType(const std::string& newType) { type = newType; }
    
    // Attribute accessors
    bool getIsStatic() const { return isStatic; }
    bool getIsConst() const { return isConst; }
    bool getIsPublic() const { return isPublic; }
    bool getIsProtected() const { return isProtected; }
    bool getIsPrivate() const { return isPrivate; }
    bool getIsExported() const { return isExported; }
    bool getIsImported() const { return isImported; }
    
    // Attribute modifiers
    void setIsStatic(bool value) { isStatic = value; }
    void setIsConst(bool value) { isConst = value; }
    void setIsPublic(bool value) { isPublic = value; }
    void setIsProtected(bool value) { isProtected = value; }
    void setIsPrivate(bool value) { isPrivate = value; }
    void setIsExported(bool value) { isExported = value; }
    void setIsImported(bool value) { isImported = value; }
    
    // String representation
    std::string toString() const;
};

// Scope class - represents a lexical scope in the code
class Scope {
private:
    std::string name;
    ScopeType type;
    int startLine;
    int startColumn;
    int endLine;
    int endColumn;
    std::string filename;
    
    // Parent-child relationships
    Scope* parent;
    std::vector<std::unique_ptr<Scope>> children;
    
    // Symbols in this scope
    std::unordered_map<std::string, std::unique_ptr<Symbol>> symbols;
    
public:
    Scope(const std::string& name, ScopeType type, 
          int startLine, int startColumn, 
          const std::string& filename, Scope* parent = nullptr);
    
    // Set end position (when scope is closed)
    void setEndPosition(int line, int column) {
        endLine = line;
        endColumn = column;
    }
    
    // Accessors
    const std::string& getName() const { return name; }
    ScopeType getType() const { return type; }
    int getStartLine() const { return startLine; }
    int getStartColumn() const { return startColumn; }
    int getEndLine() const { return endLine; }
    int getEndColumn() const { return endColumn; }
    const std::string& getFilename() const { return filename; }
    Scope* getParent() const { return parent; }
    
    // Scope management
    Scope* createChildScope(const std::string& name, ScopeType type, 
                            int startLine, int startColumn);
    const std::vector<std::unique_ptr<Scope>>& getChildren() const { return children; }
    std::vector<std::unique_ptr<Scope>>& getChildren() { return children; }
    
    // Symbol management
    Symbol* addSymbol(const std::string& name, SymbolKind kind, const std::string& type, 
                      int line, int column, bool isDefined = false);
    Symbol* findSymbol(const std::string& name);
    Symbol* findSymbolInScope(const std::string& name) const;
    const std::unordered_map<std::string, std::unique_ptr<Symbol>>& getSymbols() const { return symbols; }
    
    // Symbol lookup through scope hierarchy
    Symbol* lookup(const std::string& name);
    
    // String representation
    std::string toString() const;
};

// SymbolTable class - main container for all symbol information
class SymbolTable {
private:
    std::unique_ptr<Scope> globalScope;
    std::unordered_map<std::string, std::vector<Symbol*>> symbolsByName;
    
public:
    SymbolTable();
    
    // Scope management
    Scope* getGlobalScope() const { return globalScope.get(); }
    Scope* createScope(const std::string& name, ScopeType type, 
                      int startLine, int startColumn, 
                      const std::string& filename, Scope* parent = nullptr);
    
    // Symbol management
    void registerSymbol(Symbol* symbol);
    std::vector<Symbol*> findSymbols(const std::string& name) const;
    
    // Scope tracking for parsing
    Scope* currentScope;
    void enterScope(Scope* scope) { currentScope = scope; }
    void exitScope() { 
        if (currentScope && currentScope->getParent()) {
            currentScope = currentScope->getParent();
        }
    }
    
    // Convenience methods for adding symbols to current scope
    Symbol* addSymbol(const std::string& name, SymbolKind kind, const std::string& type, 
                      int line, int column, bool isDefined = false);
                      
    // String representation
    std::string toString() const;
};

#endif // SYMBOL_TABLE_H 