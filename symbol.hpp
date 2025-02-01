#ifndef SYMBOL_HPP
#define SYMBOL_HPP

#include <string>
#include <vector>
#include "nodes.hpp"
using namespace ast;
using namespace std;

enum SymbolType {
    VARIABLE,
    FUNCTION
};

/*from ast namespace:
    Built-in types 
enum BuiltInType {
    TYPE_ERROR = -1,
    VOID,
    BOOL,
    BYTE,
    INT,
    STRING
}; */

class Symbol {

private:
string name;                   // Name of the symbol
SymbolType symbolType;              // VARIABLE or FUNCTION
BuiltInType dataType;                  // Data type (e.g., INT, BYTE, BOOL, etc.)
int offset;                         // Memory offset (for variables or parameters)
vector<BuiltInType> parameterTypes; // Function parameter types
vector<string> parameterNames; // Function parameter names
RegisterStruct symbolRegister;

public:
    // Default constructor
    Symbol()
        : name(""), symbolType(SymbolType::VARIABLE), dataType(BuiltInType::TYPE_ERROR), offset(0), symbolRegister{"0", false} {}

    // Constructor for variables
    Symbol(const string& name, SymbolType symbolType, BuiltInType dataType, int offset)
        : name(name), symbolType(symbolType), dataType(dataType), offset(offset), symbolRegister{"0", false} {}

    // Constructor for functions
    Symbol(const string& name, SymbolType symbolType, BuiltInType dataType,
           const vector<BuiltInType>& paramTypes, const vector<string>& paramNames)
        : name(name), symbolType(symbolType), dataType(dataType),
          offset(0), parameterTypes(paramTypes), parameterNames(paramNames), symbolRegister{"0", false} {}

    // Getters
    const string& getName() const { return name; }
    SymbolType getSymbolType() const { return symbolType; }
    BuiltInType getDataType() const { return dataType; }
    void setDataType(BuiltInType newSymbolType) { dataType = newSymbolType; }
    int getOffset() const { return offset; }
    const vector<BuiltInType>& getParameterTypes() const { return parameterTypes; }
    const vector<string>& getParameterNames() const { return parameterNames; }

    RegisterStruct getRegister(void) { return symbolRegister; }
    void setRegister(const RegisterStruct& regToSet) { symbolRegister = regToSet; }

    void setRegName(const string& reg) { symbolRegister.name = reg; }
    string getRegName() const { return symbolRegister.name; }
    void setRegZeroState(bool isZero) { symbolRegister.isZero = isZero; }
    bool isRegZero(void) { return symbolRegister.isZero; }
    

};

#endif // SYMBOL_HPP
