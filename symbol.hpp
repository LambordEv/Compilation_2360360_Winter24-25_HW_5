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

// typedef struct RegisterStruct {
//     string name;
//     BuiltInType type;
// } RegisterStruct;

class Symbol {

private:
string name;                   // Name of the symbol
SymbolType symbolType;              // VARIABLE or FUNCTION
BuiltInType dataType;                  // Data type (e.g., INT, BYTE, BOOL, etc.)
int offset;                         // Memory offset (for variables or parameters)
vector<BuiltInType> parameterTypes; // Function parameter types
vector<string> parameterNames; // Function parameter names
string regName;

public:
    // Default constructor
    Symbol()
        : name(""), symbolType(SymbolType::VARIABLE), dataType(BuiltInType::TYPE_ERROR), offset(0), regName("") {}

    // Constructor for variables
    Symbol(const string& name, SymbolType symbolType, BuiltInType dataType, int offset)
        : name(name), symbolType(symbolType), dataType(dataType), offset(offset), regName("") {}

    // Constructor for functions
    Symbol(const string& name, SymbolType symbolType, BuiltInType dataType,
           const vector<BuiltInType>& paramTypes, const vector<string>& paramNames)
        : name(name), symbolType(symbolType), dataType(dataType),
          offset(0), parameterTypes(paramTypes), parameterNames(paramNames), regName("") {}

    // Getters
    const string& getName() const { return name; }
    SymbolType getSymbolType() const { return symbolType; }
    BuiltInType getDataType() const { return dataType; }
    void setDataType(BuiltInType newSymbolType) { dataType = newSymbolType; }
    int getOffset() const { return offset; }
    const vector<BuiltInType>& getParameterTypes() const { return parameterTypes; }
    const vector<string>& getParameterNames() const { return parameterNames; }
    void setRegName(const string& reg) { regName = reg; }
    string getRegName() const { return regName; }

};

#endif // SYMBOL_HPP
