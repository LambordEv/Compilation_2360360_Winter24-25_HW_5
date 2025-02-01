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
#include <iostream>  // For cout
#include <algorithm> // For reverse
using namespace output;
using namespace std;




class SymbolTable {
private:
    Scope* globalScope; // Global scope for functions and global variables
    stack<Scope*> scopeStack; // Stack to manage nested scopes

public:
    SymbolTable() {
        globalScope = new Scope(nullptr);
        scopeStack.push(globalScope);
        // globalScope->addFunctionSymbol("print", BuiltInType::VOID, {BuiltInType::STRING}, {"str"}, -1);
        // globalScope->addFunctionSymbol("printi", BuiltInType::VOID, {BuiltInType::INT}, {"num"}, -1);
    }

    ~SymbolTable() {
        while (!scopeStack.empty()) {
            Scope* top = scopeStack.top();
            scopeStack.pop();
            delete top;
        }
    }

    void addVariableSymbol(const string& name, BuiltInType dataType, int lineno) {
        getCurrentScope()->addVariableSymbol(name, dataType, lineno);
    }

    void addParameterSymbol(const string& name, BuiltInType dataType, int lineno) {
        getCurrentScope()->addParameterSymbol(name, dataType, lineno);
    }

    void addFunctionSymbol(const string& name, BuiltInType returnType,
                           const vector<BuiltInType>& paramTypes,
                           const vector<string>& paramNames, int lineno) {
        globalScope->addFunctionSymbol(name, returnType, paramTypes, paramNames, lineno);
    }

    Symbol* getSymbol(const string &name) {
        Symbol* symbol = getCurrentScope()->getSymbolName(name);
        return symbol;
    }

    Symbol* getFuncSymbol(const string &name) {
        Symbol* symbol = globalScope->getSymbolName(name);
        return symbol;
    }

    Scope* beginScope(bool isLoopScope = false, string scopeName = "") {
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
            throw runtime_error("Cannot end the last scope");
        }
    }

    Scope* getCurrentScope() {
        return scopeStack.top();
    }

    void setRegInSymTable(const string& name, const RegisterStruct& reg) {
        this->getCurrentScope()->getSymbolName(name)->setRegister(reg);
    }

    RegisterStruct getRegFromSymTable(const string& name) { 
        RegisterStruct resultReg = this->getCurrentScope()->getSymbolName(name)->getRegister();
        return resultReg;
    }

    void printSymbolTable() const {
        stack<Scope*> tempStack(scopeStack);
        vector<Scope*> scopes;

        // Collect scopes in order
        while (!tempStack.empty()) {
            scopes.push_back(tempStack.top());
            tempStack.pop();
        }

        // Reverse to start with the global scope
        reverse(scopes.begin(), scopes.end());

        cout << "Symbol Table:\n";
        for (size_t i = 0; i < scopes.size(); ++i) {
            cout << "Scope " << i << (i == 0 ? " (Global)" : "") << ":\n";
            const auto& symbols = scopes[i]->getSymbolTable();
            for (const auto& entry : symbols) {
                const Symbol& symbol = entry.second;
                cout << "  Name: " << symbol.getName()
                          << ", Type: " << (symbol.getSymbolType() == SymbolType::VARIABLE ? "Variable" : "Function")
                          << ", Data Type: " << static_cast<int>(symbol.getDataType())
                          << ", Offset: " << symbol.getOffset();
                if (symbol.getSymbolType() == SymbolType::FUNCTION) {
                    cout << ", Parameters: ";
                    const auto& paramTypes = symbol.getParameterTypes();
                    const auto& paramNames = symbol.getParameterNames();
                    for (size_t j = 0; j < paramNames.size(); ++j) {
                        cout << "(" << paramNames[j] << ": " << static_cast<int>(paramTypes[j]) << ")";
                        if (j < paramNames.size() - 1) cout << ", ";
                    }
                }
                cout << '\n';
            }
        }
    }
};

#endif // SYMBOL_TABLE_HPP
