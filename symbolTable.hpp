#ifndef SYMBOL_TABLE_HPP
#define SYMBOL_TABLE_HPP

#include "scope.hpp"
#include "nodes.hpp"
#include "output.hpp"
using namespace ast;
#include <memory>
#include <unordered_map>
#include <string>
#include <stack>
#include <iostream>  // For std::cout
#include <algorithm> // For std::reverse
using namespace output;
using namespace std;




class SymbolTable {
private:
    Scope* globalScope; // Global scope for functions and global variables
    std::stack<Scope*> scopeStack; // Stack to manage nested scopes

public:
    SymbolTable() {
        globalScope = new Scope(nullptr);
        scopeStack.push(globalScope);
        // globalScope->addFunctionSymbol("print", BuiltInType::VOID, {BuiltInType::STRING}, {"str"}, -1);
        // globalScope->addFunctionSymbol("printi", BuiltInType::VOID, {BuiltInType::INT}, {"num"}, -1);
    }

    ~SymbolTable() {
        while (!scopeStack.empty()) {
            delete scopeStack.top();
            scopeStack.pop();
        }
    }

    void addVariableSymbol(const std::string& name, BuiltInType dataType, int lineno) {
        getCurrentScope()->addVariableSymbol(name, dataType, lineno);
    }

    void addParameterSymbol(const std::string& name, BuiltInType dataType, int lineno) {
        getCurrentScope()->addParameterSymbol(name, dataType, lineno);
    }

    void addFunctionSymbol(const std::string& name, BuiltInType returnType,
                           const std::vector<BuiltInType>& paramTypes,
                           const std::vector<std::string>& paramNames, int lineno) {
        globalScope->addFunctionSymbol(name, returnType, paramTypes, paramNames, lineno);
        // Scope* currScope = beginScope(false, name);
        // for (size_t i = 0; i < paramNames.size(); ++i) {
        //     char paramName[2096] = {0};                     /// TODO - Not appropriate
        //     sscanf(paramName, "%s-Param%d\0", name.c_str(), i);     /// TODO - Not appropriate
        //     currScope->addParameterSymbol(paramName, paramTypes[i], lineno);
        // }
    }

    Symbol* getSymbol(const std::string &name, int lineno) {
        // printf("Get Symbol in %s\n", "get_Symbol symbolTable");
        Symbol* symbol = getCurrentScope()->getSymbolName(name);
        // if (symbol == nullptr) {
        //     printf("Undefined Variable in %s\n", "getSymbol in symbolTable");
        //     errorUndef(lineno, name);
        // }
        return symbol;
    }

    Symbol* getFuncSymbol(const std::string &name, int lineno) {
        // printf("Get Symbol in %s\n", "get_Symbol symbolTable");
        Symbol* symbol = globalScope->getSymbolName(name);
        // if (symbol == nullptr) {
        //     printf("Undefined Variable in %s\n", "getSymbol in symbolTable");
        //     errorUndef(lineno, name);
        // }
        return symbol;
    }

    Scope* beginScope(bool isLoopScope = false, std::string scopeName = "") {
        Scope* newScope = new Scope(scopeStack.top(), isLoopScope, scopeName);
        scopeStack.push(newScope);

        return newScope;
    }

    void endScope() {
        if (scopeStack.size() > 1) { 
            Scope* currentScope = scopeStack.top();
            scopeStack.pop();
            delete currentScope;
        } else {
            throw std::runtime_error("Cannot end the last scope");
        }
    }

    Scope* getCurrentScope() {
        return scopeStack.top();
    }

    void printSymbolTable() const {
        std::stack<Scope*> tempStack(scopeStack);
        std::vector<Scope*> scopes;

        // Collect scopes in order
        while (!tempStack.empty()) {
            scopes.push_back(tempStack.top());
            tempStack.pop();
        }

        // Reverse to start with the global scope
        std::reverse(scopes.begin(), scopes.end());

        std::cout << "Symbol Table:\n";
        for (size_t i = 0; i < scopes.size(); ++i) {
            std::cout << "Scope " << i << (i == 0 ? " (Global)" : "") << ":\n";
            const auto& symbols = scopes[i]->getSymbolTable();
            for (const auto& entry : symbols) {
                const Symbol& symbol = entry.second;
                std::cout << "  Name: " << symbol.getName()
                          << ", Type: " << (symbol.getSymbolType() == SymbolType::VARIABLE ? "Variable" : "Function")
                          << ", Data Type: " << static_cast<int>(symbol.getDataType())
                          << ", Offset: " << symbol.getOffset();
                if (symbol.getSymbolType() == SymbolType::FUNCTION) {
                    std::cout << ", Parameters: ";
                    const auto& paramTypes = symbol.getParameterTypes();
                    const auto& paramNames = symbol.getParameterNames();
                    for (size_t j = 0; j < paramNames.size(); ++j) {
                        std::cout << "(" << paramNames[j] << ": " << static_cast<int>(paramTypes[j]) << ")";
                        if (j < paramNames.size() - 1) std::cout << ", ";
                    }
                }
                std::cout << '\n';
            }
        }
    }
};

#endif // SYMBOL_TABLE_HPP
