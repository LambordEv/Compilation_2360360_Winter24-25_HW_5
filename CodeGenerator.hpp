#ifndef CODE_GENERATOR_HPP
#define CODE_GENERATOR_HPP

#include "visitor.hpp"
#include "symbolTable.hpp"
#include "output.hpp"
#include <string>
#include <stdexcept>
#include <vector>
#include <iostream>

using namespace std;
using namespace ast;

static BuiltInType binOp_ResultType(Exp& left, Exp& right, SymbolTable& symbolTable) {
    BuiltInType leftBuiltInType = BuiltInType::TYPE_ERROR;
    BuiltInType rightBuiltInType = BuiltInType::TYPE_ERROR;

    if (left.getType() == NODE_ID) {
        Symbol* leftSymbol = symbolTable.getSymbol(left.getValueStr());
        leftBuiltInType = leftSymbol->getDataType();
    }
    else if (left.getType() == NODE_Num) {
        leftBuiltInType = BuiltInType::INT;
    }
    else if (left.getType() == NODE_NumB) {
        leftBuiltInType = BuiltInType::BYTE;
    }

    if (right.getType() == NODE_ID) {
        Symbol* rightSymbol = symbolTable.getSymbol(right.getValueStr());
        rightBuiltInType = rightSymbol->getDataType();
    }
    else if (right.getType() == NODE_Num) {
        rightBuiltInType = BuiltInType::INT;
    }
    else if (right.getType() == NODE_NumB) {
        rightBuiltInType = BuiltInType::BYTE;
    }

    if (TYPE_ERROR == leftBuiltInType || TYPE_ERROR == rightBuiltInType) {
        return TYPE_ERROR;
    } else if(BYTE == leftBuiltInType && BYTE == rightBuiltInType){
        return BYTE;
    } else {
        return INT;
    }
}

class CodeGenerator : public Visitor {
private:
    output::CodeBuffer codeBuffer;
    SymbolTable symbolTable;

public:
    // SemanticAnalyzer(SymbolTable* symbolTable)
    //     : symbolTable(symbolTable) {}
    CodeGenerator() : codeBuffer(), symbolTable() {}

    void CodeGenerator_beginScope(string scopeName = "", bool isLoopScope = false) {
        symbolTable.beginScope(isLoopScope, scopeName);
    }

    void CodeGenerator_endScope() {
        symbolTable.endScope();
    }

    // Implementations of visit methods
    void visit(Num& node) override { 
        string currVar = this->codeBuffer.freshVar();
        if(node.getValueInt() == 0) {
            currVar = "0";
        } else {
            this->codeBuffer << currVar << " = add i32 " << node.getValueInt() << ", 0" << endl;
        }
        node.setRegister(currVar);
    }

    void visit(NumB& node) override {
        string currVar = this->codeBuffer.freshVar();
        if(node.getValueInt() == 0) {
            currVar = "0";
        } else {
            this->codeBuffer << currVar << " = add i32 " << node.getValueInt() << ", 0" << endl;
            string tmpVar = currVar;
            currVar = this->codeBuffer.freshVar();
            this->codeBuffer << currVar << " = and i32 " << tmpVar << ", 255" << endl;
        }
        node.setRegister(currVar);
    }

    void visit(String& node) override {
        this->codeBuffer.emitString(node.getText());
    }

    void visit(Bool& node) override {
        string currVar = this->codeBuffer.freshVar();
        int initBoolValue = node.getValueBool() ? 1 : 0;
        this->codeBuffer << currVar << " = add i32 " << initBoolValue << ", 0" << endl;
        node.setRegister(currVar);
    }

    void visit(ID& node) override {
        Symbol* var = symbolTable.getSymbol(node.getValueStr());
        if (var && var->getSymbolType() == VARIABLE) {
            node.setType(NODE_ID);
        }
    }

    void visit(BinOp& node) override {
        node.getLeft()->accept(*this);
        node.getRight()->accept(*this);

        string leftReg = (node.getLeft()->getType() == NODE_ID) ? this->symbolTable.getRegNameSymTable(node.getLeft()->getValueStr()) : node.getLeft()->getRegister();
        string rightReg = (node.getRight()->getType() == NODE_ID) ? this->symbolTable.getRegNameSymTable(node.getRight()->getValueStr()) : node.getRight()->getRegister();

        string currVar = this->codeBuffer.freshVar();
        string resultText = currVar;
        switch(node.getOp()) {
            case BinOpType::ADD:
                // if left in numb and right is numb then truncate, else already written 
                resultText += " = add i32 " + leftReg + ", " + rightReg;
                break;
            case BinOpType::SUB:
                if(leftReg == rightReg) {
                    currVar = "0";
                } else {
                    resultText += " = sub i32 " + leftReg + ", " + rightReg;
                }
                break;
            case BinOpType::MUL:
                if (leftReg == "0" || rightReg == "0") {
                    currVar = "0";
                } else {
                    resultText += " = mul i32 " + leftReg + ", " + rightReg;
                }
                break;
            case BinOpType::DIV:
                if(rightReg == "0") {
                    // TODO - call errorDivByZero
                    // TODO - call exit
                    cout << "Division by zero" << endl;
                    exit(0);
                } else {
                    resultText += " = sdiv i32 " + leftReg + ", " + rightReg;
                }
                break;
        }
        
        if (currVar != "0") {
            if(binOp_ResultType(*node.getLeft(), *node.getRight(), this->symbolTable) == BYTE) {
                string tmpVar = currVar;
                currVar = this->codeBuffer.freshVar();
                resultText += "\n" + currVar + " = and i32 " + tmpVar + ", 255";
            }
            this->codeBuffer << resultText << endl;
        }

        node.setRegister(currVar);
    }

    void visit(RelOp& node) override {
        node.getLeft()->accept(*this);
        node.getRight()->accept(*this);

        string leftReg = (node.getLeft()->getType() == NODE_ID) ? this->symbolTable.getRegNameSymTable(node.getLeft()->getValueStr()) : node.getLeft()->getRegister();
        string rightReg = (node.getRight()->getType() == NODE_ID) ? this->symbolTable.getRegNameSymTable(node.getRight()->getValueStr()) : node.getRight()->getRegister();

        string currVar = this->codeBuffer.freshVar();
        string resultText = currVar;
        switch(node.getOp()) { 
            // TODO - Maybe we will have to jump to labels from here, where this code should be written? here or in the if/else and while
            case RelOpType::EQ:
                resultText += " = icmp eq i32 " + leftReg + ", " + rightReg;
                break;
            case RelOpType::NE:
                resultText += " = icmp ne i32 " + leftReg + ", " + rightReg;
                break;
            case RelOpType::LT:
                resultText += " = icmp slt i32 " + leftReg + ", " + rightReg;
                break;
            case RelOpType::GT:
                resultText += " = icmp sgt i32 " + leftReg + ", " + rightReg;
                break;
            case RelOpType::LE:
                resultText += " = icmp sle i32 " + leftReg + ", " + rightReg;
                break;
            case RelOpType::GE:
                resultText += " = icmp sge i32 " + leftReg + ", " + rightReg;
                break;
        }
        this->codeBuffer << resultText << endl;
        string tmpVar = currVar;
        currVar = this->codeBuffer.freshVar();
        this->codeBuffer << currVar << " = zext i1 " << tmpVar << " to i32" << endl;
        node.setRegister(currVar);
        node.setType(NODE_Bool);
    }

    void visit(Not& node) override {
        node.getExpr()->accept(*this);
        string currVar = this->codeBuffer.freshVar();
        
        string expReg = (node.getExpr()->getType() == NODE_ID) ? this->symbolTable.getRegNameSymTable(node.getExpr()->getValueStr()) : node.getExpr()->getRegister();
        this->codeBuffer << currVar << " = xor i32 " << expReg << ", 1" << endl;
        node.setRegister(currVar);
    }

    void visit(And& node) override { 
        // Lazy Evaluation - If Left is False, then Right is not evaluated
        const string andLabel = this->codeBuffer.freshLabel();
        // Flow Control Labels
        const string rightEvaluateLabel = andLabel + ".rightEvaluationSection";
        const string resultLabel = andLabel + ".resultSection";
        // Memory Allocation Labels
        const string leftOperand = "%allocation_" + andLabel.substr(1) + ".leftOperand";
        const string rightOperand = "%allocation_" + andLabel.substr(1) + ".rightOperand";

        string leftReg = "0";
        string rightReg = "0";
        string currVar = this->codeBuffer.freshVar();

        // Prepare left and right values
        this->codeBuffer << leftOperand << " = alloca i1" << endl;
        this->codeBuffer << "store i1 0, i1* " << leftOperand << endl;
        this->codeBuffer << rightOperand << " = alloca i1" << endl;
        this->codeBuffer << "store i1 0, i1* " << rightOperand << endl;

        // Evaluate Left
        node.getLeft()->accept(*this);
        leftReg = (node.getLeft()->getType() == NODE_ID) ? this->symbolTable.getRegNameSymTable(node.getLeft()->getValueStr()) : node.getLeft()->getRegister();
        
        // Convert to i1 - The branch instruction requires i1 type
        string tmpVar = leftReg;
        leftReg = this->codeBuffer.freshVar();
        this->codeBuffer << leftReg << " = trunc i32 " << tmpVar << " to i1" << endl;
        this->codeBuffer << "store i1 " << leftReg << ", i1* " << leftOperand << endl;
        
        // If Left is False, then Right is not evaluated
        this->codeBuffer << "br i1 " << leftReg << ", label " << rightEvaluateLabel << ", label " << resultLabel << endl;

        // Evaluate Right
        this->codeBuffer << "\n" << rightEvaluateLabel.substr(1) << ":" << endl;
        node.getRight()->accept(*this);
        rightReg = (node.getRight()->getType() == NODE_ID) ? this->symbolTable.getRegNameSymTable(node.getRight()->getValueStr()) : node.getRight()->getRegister();

        // Convert to i1 - The branch instruction requires i1 type
        tmpVar = rightReg;
        rightReg = this->codeBuffer.freshVar();
        this->codeBuffer << rightReg << " = trunc i32 " << tmpVar << " to i1" << endl;
        this->codeBuffer << "store i1 " << rightReg << ", i1* " << rightOperand << endl;

        this->codeBuffer << "br label " << resultLabel << endl;
        this->codeBuffer << "\n" << resultLabel.substr(1) << ":" << endl;

        // Evaluate And
        string leftOperandResult = this->codeBuffer.freshVar();
        string rightOperandResult = this->codeBuffer.freshVar();
        this->codeBuffer << leftOperandResult << " = load i1, i1* " << leftOperand << endl;
        this->codeBuffer << rightOperandResult << " = load i1, i1* " << rightOperand << endl;
        this->codeBuffer << currVar << " = and i1 " << leftOperandResult << ", " << rightOperandResult << endl;

        // Return to i32 type
        tmpVar = currVar;
        currVar = this->codeBuffer.freshVar();
        this->codeBuffer << currVar << " = zext i1 " << tmpVar << " to i32" << endl;
        node.setRegister(currVar);
    }

    void visit(Or& node) override {
        // Lazy Evaluation - If Left is True, then Right is not evaluated
        const string orLabel = this->codeBuffer.freshLabel();
        // Flow Control Labels
        const string rightEvaluateLabel = orLabel + ".rightEvaluationSection";
        const string resultLabel = orLabel + ".resultSection";
        // Memory Allocation Labels
        const string leftOperand = "%allocation_" + orLabel.substr(1) + ".leftOperand";
        const string rightOperand = "%allocation_" + orLabel.substr(1) + ".rightOperand";

        string leftReg = "0";
        string rightReg = "0";
        string currVar = this->codeBuffer.freshVar();

        // Prepare left and right values
        this->codeBuffer << leftOperand << " = alloca i1" << endl;
        this->codeBuffer << "store i1 0, i1* " << leftOperand << endl;
        this->codeBuffer << rightOperand << " = alloca i1" << endl;
        this->codeBuffer << "store i1 0, i1* " << rightOperand << endl;

        // Evaluate Left
        node.getLeft()->accept(*this);
        leftReg = (node.getLeft()->getType() == NODE_ID) ? this->symbolTable.getRegNameSymTable(node.getLeft()->getValueStr()) : node.getLeft()->getRegister();
        
        // Convert to i1 - The branch instruction requires i1 type
        string tmpVar = leftReg;
        leftReg = this->codeBuffer.freshVar();
        this->codeBuffer << leftReg << " = trunc i32 " << tmpVar << " to i1" << endl;
        this->codeBuffer << "store i1 " << leftReg << ", i1* " << leftOperand << endl;
        
        // If Left is True, then Right is not evaluated
        this->codeBuffer << "br i1 " << leftReg << ", label " << resultLabel << ", label " << rightEvaluateLabel << endl;

        // Evaluate Right
        this->codeBuffer << "\n" << rightEvaluateLabel.substr(1) << ":" << endl;
        node.getRight()->accept(*this);
        rightReg = (node.getRight()->getType() == NODE_ID) ? this->symbolTable.getRegNameSymTable(node.getRight()->getValueStr()) : node.getRight()->getRegister();

        // Convert to i1 - The branch instruction requires i1 type
        tmpVar = rightReg;
        rightReg = this->codeBuffer.freshVar();
        this->codeBuffer << rightReg << " = trunc i32 " << tmpVar << " to i1" << endl;
        this->codeBuffer << "store i1 " << rightReg << ", i1* " << rightOperand << endl;

        this->codeBuffer << "br label " << resultLabel << endl;
        this->codeBuffer << "\n" << resultLabel.substr(1) << ":" << endl;

        // Evaluate Or
        string leftOperandResult = this->codeBuffer.freshVar();
        string rightOperandResult = this->codeBuffer.freshVar();
        this->codeBuffer << leftOperandResult << " = load i1, i1* " << leftOperand << endl;
        this->codeBuffer << rightOperandResult << " = load i1, i1* " << rightOperand << endl;
        this->codeBuffer << currVar << " = or i1 " << leftOperandResult << ", " << rightOperandResult << endl;

        // Return to i32 type
        tmpVar = currVar;
        currVar = this->codeBuffer.freshVar();
        this->codeBuffer << currVar << " = zext i1 " << tmpVar << " to i32" << endl;
        node.setRegister(currVar);

        // We need to de-allocate the allocated memory (?)
    }

    void visit(Type& node) override {
        // Not Needed ?
    }

    void visit(Cast& node) override {
        node.getExpr()->accept(*this);
        string currVar = this->codeBuffer.freshVar();
        string tmpVar;

        Symbol* expSymbol = nullptr;
        BuiltInType originalType = TYPE_ERROR;
        BuiltInType targetType = TYPE_ERROR;
        if (node.getExpr()->getType() == NODE_ID || node.getExpr()->getType() == NODE_Num ||
            node.getExpr()->getType() == NODE_NumB) {
            expSymbol = symbolTable.getSymbol(node.getExpr()->getValueStr());
            tmpVar = (node.getExpr()->getType() == NODE_ID) ? this->symbolTable.getRegNameSymTable(node.getExpr()->getValueStr()) : node.getExpr()->getRegister();
            if(!expSymbol) {
                originalType = semanticToBuiltInType(node.getExpr()->getType());
            }else{
                originalType = expSymbol->getDataType();
            }
        }

        if (node.getTargetType() == INT
            && (originalType == INT || originalType == BYTE)) {
            targetType = INT;
            node.getExpr()->setType(NODE_Num);
            node.setType(NODE_Num);
            this->codeBuffer << currVar << " = add i32 " << tmpVar << ", 0" << endl;
        } else if (node.getTargetType() == BYTE
            && (originalType == BYTE || originalType == INT)) {
            targetType = BYTE;
            node.getExpr()->setType(NODE_NumB);
            node.setType(NODE_NumB);
            this->codeBuffer << currVar << " = and i32 " << tmpVar << ", 255" << endl;
        }
        node.setRegister(currVar);
    }

    void visit(ExpList& node) override {
        // for (auto& expr : node.getExpressions()) {
        //     expr->accept(*this);
        // }
    }

    void visit(Call& node) override {
        this->codeBuffer << "call i32 (i8*, ...) @printf(i8* %format_ptr, i32 %counter_val)" << endl;
        // // printf("Get Symbol in %s\n", "Visit Call");
        // Symbol* func = symbolTable.getSymbol(node.getFuncId(), node.getLine());
        // if (func && func->getSymbolType() != FUNCTION) {
        //     errorDefAsVar(node.getLine(), node.getFuncId());
        // }
        // if (!func) {
        //     errorUndefFunc(node.getLine(), node.getFuncId());
        // }

        // node.setType(builtInToNodeType(func->getDataType())); 
        // node.getArgsExp()->accept(*this);

        // vector<BuiltInType> paramsTypesInSymbolTable = func->getParameterTypes();
        // vector<shared_ptr<Exp>> paramsInFunc = node.getArgs();

        // vector<BuiltInType> types;
        // for(auto& exp : paramsInFunc) {
        //     if (exp->getType() == NODE_ID) {
        //         Symbol* expSymbol = symbolTable.getSymbol(exp->getValueStr(), exp->getLine());
        //         if (expSymbol == nullptr) {
        //             errorUndef(exp->getLine(), exp->getValueStr());
        //         }
        //         types.push_back(expSymbol->getDataType());
        //     } else if (exp->getType() == NODE_Num) {
        //         // cout << "Num type is not supported in function calls" << endl;
        //         types.push_back(BuiltInType::INT);
        //     } else if (exp->getType() == NODE_NumB) {
        //         types.push_back(BuiltInType::BYTE);
        //     } else if (exp->getType() == NODE_Bool) {
        //         types.push_back(BuiltInType::BOOL);
        //     } else if (exp->getType() == NODE_String) {
        //         // cout << "String type is not supported in function calls" << endl;
        //         types.push_back(BuiltInType::STRING);
        //     } else {
        //         types.push_back(BuiltInType::TYPE_ERROR);
        //     }
        // }

        // // in error which vector should be passed as argument????????????????????
        // vector<string> typesStr = convertVectorToStrings(types);
        // vector<string> paramsTypesInSymbolTableStr = convertVectorToStrings(paramsTypesInSymbolTable);
        // if (paramsTypesInSymbolTable.size() != types.size()) {
        //     errorPrototypeMismatch(node.getLine(), node.getFuncId(), paramsTypesInSymbolTableStr);
        // }
        // for (size_t i = 0; i < paramsTypesInSymbolTable.size(); i++) {
        //     if (paramsTypesInSymbolTable[i] != types[i]) {
        //         if((paramsTypesInSymbolTable[i] == BuiltInType::INT && types[i] == BuiltInType::BYTE)) {
        //             continue;
        //         }
        //         errorPrototypeMismatch(node.getLine(), node.getFuncId(), paramsTypesInSymbolTableStr);
        //     }
        // }

        // node.setType(builtInToNodeType(func->getDataType())); 

    }

    void visit(Statements& node) override {
        // // beginScope();
        for (auto& statement : node.getStatements()) {
            // if (statement->getType() == NODE_Statements) {
            //     beginScope();
            // }
            statement->accept(*this);
            // cout << "Current Statement Type: " << statement->getType() << endl;
            // if (statement->getType() == NODE_Statements) {
            //     endScope();
            // }
        }
        // // endScope();
    }

    void visit(Break& node) override {
        // if (!(symbolTable.getCurrentScope()->isInLoopScope())) {
        //     errorUnexpectedBreak(node.getLine());
        // }
    }

    void visit(Continue& node) override {
        // if (!(symbolTable.getCurrentScope()->isInLoopScope())) {
        //     errorUnexpectedContinue(node.getLine());
        // }
    }

    void visit(Return& node) override {
        // if(nullptr != node.getExpr()) {
        //     node.getExpr()->accept(*this);
        // }

        // string funcName = symbolTable.getCurrentScope()->getScopeName();
        // Symbol* func = symbolTable.getSymbol(funcName, node.getLine());
        // if(func == nullptr) {
        //     errorUndefFunc(node.getLine(), funcName);
        // }
        // BuiltInType funcRetType = func->getDataType();
        
        // BuiltInType expType = BuiltInType::TYPE_ERROR;
        // if(node.getExpr() == nullptr) {
        //     // cout << "Expresion is null" << endl;
        //     expType = BuiltInType::VOID;
        // } 
        // else {
        //     // cout << "Expresion is not null --- Its type is " << node.getExpr()->getType() << endl;
        //     if (node.getExpr()->getType() == NODE_ID) {
        //         Symbol* expSymbol = symbolTable.getSymbol(node.getExpr()->getValueStr(), node.getExpr()->getLine());
        //         if (expSymbol == nullptr) {
        //             errorUndef(node.getLine(), node.getExpr()->getValueStr());
        //         }
        //         expType = expSymbol->getDataType();
        //     } else if (node.getExpr()->getType() == NODE_Num) {
        //         expType = BuiltInType::INT;
        //     } else if (node.getExpr()->getType() == NODE_NumB) {
        //         expType = BuiltInType::BYTE;
        //     } else if (node.getExpr()->getType() == NODE_Bool) {
        //         expType = BuiltInType::BOOL;
        //     }
        // }

        // if (funcRetType != expType && !(funcRetType == BuiltInType::INT && expType == BuiltInType::BYTE)) {
        //     // cout << "funcRetType: " << funcRetType << " expType: " << expType << endl;
           
        //     errorMismatch(node.getLine());
        // }
    }

    void visit(If& node) override {
        // beginScope();
        // // beginScope();
        // node.getCondition()->accept(*this);
        // if (node.getCondition()->getType() != NODE_Bool) {
        //     errorMismatch(node.getCondition()->getLine());
        // }
        // // node.getThen()->accept(*this);
        // // cout << "In Visit If Condition Exists " << node.getThen()->getType() << endl;
        // if (node.getThen()->getType() == NODE_Statements) {
        //     beginScope();
        // }
        // node.getThen()->accept(*this);
        // if (node.getThen()->getType() == NODE_Statements) {
        //     endScope();
        // }
        
        // // endScope();
        // endScope();

        // if (node.getElse()) {
        //     beginScope();
        //     if(node.getElse()->getType() == NODE_Statements) {
        //         beginScope();
        //         node.getElse()->accept(*this);
        //         endScope();
        //     }
        //     endScope();
        // }
    }

    void visit(While& node) override {
        // // Condition scope (?)
        // beginScope("", true);
        // node.getCondition()->accept(*this);
        // if (node.getCondition()->getType() != NODE_Bool) {
        //     errorMismatch(node.getCondition()->getLine());
        // }
        // // Body scope (?)
        // if (node.getBody()->getType() == NODE_Statements) {
        //     beginScope("", true);
        // }
        // node.getBody()->accept(*this);
        // if (node.getBody()->getType() == NODE_Statements) {
        //     endScope();
        // }
        
        // // cout << "In Visit While Condition Exists " << node.getCondition()->getType() << endl;
        // // cout << "In Visit While Body Exists " << node.getBody()->getType() << endl;
        // endScope();
    }

    void visit(VarDecl& node) override {
        string currVar = this->codeBuffer.freshVar();
        string varID = node.getVarId()->getValueStr();

        // TODO - different types of variables!!
        if (node.getVarInitExp()) {
            node.getVarInitExp()->accept(*this);
            if(node.getVarInitExp()->getRegister() == "0") {
                currVar = "0";
            } else {
                this->codeBuffer << currVar << " = add i32 " << node.getVarInitExp()->getRegister() << ", 0" << endl;
                if (node.getVarType() == BuiltInType::BYTE) {
                    string tmpVar = currVar;
                    currVar = this->codeBuffer.freshVar();
                    this->codeBuffer << currVar << " = and i32 " << tmpVar << ", 255" << endl; // In case of initialization with a negative
                }
            }
        } else {
            // TODO - Check type and the default value for each type is defined in the PDF
            if (node.getVarType() == BuiltInType::INT) {
                currVar = "0";
            } else if (node.getVarType() == BuiltInType::BYTE) {
                currVar = "0";
            } else if (node.getVarType() == BuiltInType::BOOL) {
                this->codeBuffer << currVar << " = add i32 0, 0" << endl;
            }
        }
        
        this->symbolTable.addVariableSymbol(node.getValueStr(), node.getVarType(), node.getLine());
        this->symbolTable.setRegNameSymTable(varID, currVar);
        node.getVarId()->accept(*this);
    }

    void visit(Assign& node) override {
        string currVar = this->codeBuffer.freshVar();
        node.getAssignExp()->accept(*this);
        string expReg = (node.getAssignExp()->getType() == NODE_ID) ? this->symbolTable.getRegNameSymTable(node.getAssignExp()->getValueStr()) : node.getAssignExp()->getRegister();

        this->codeBuffer << currVar << " = add i32 " << expReg << ", 0" << endl;
        this->symbolTable.setRegNameSymTable(node.getValueStr(), currVar);
    }

    void visit(Formal& node) override {
        // if (symbolTable.getSymbol(node.getFormalId(), node.getLine())) {
        //     errorDef(node.getLine(), node.getFormalId());
        // }

        // symbolTable.addParameterSymbol(node.getFormalId(), node.getFormalType(), node.getLine());
        // Symbol* symbol = symbolTable.getSymbol(node.getFormalId(), node.getLine());
        // printer.emitVar(node.getFormalId(), node.getFormalType(), symbol->getOffset());
    }

    void visit(Formals& node) override {
        // for (auto& formal : node.getFormals()) {
        //     formal->accept(*this);
        // }
    }

    void visit(FuncDecl& node) override {
        // vector<BuiltInType> paramsTypes = node.getFuncParams()->getFormalsType();
        // vector<string> paramsIds = node.getFuncParams()->getFormalsIds();

        
        // printer.emitFunc(node.getFuncId(), node.getFuncReturnType(), paramsTypes);

        CodeGenerator_beginScope(node.getFuncId(), false);
        // TODO - Each Parameter should be added to the scope as was done in HW_3
        // node.getFuncParams()->accept(*this);
        node.getFuncBody()->accept(*this);
        CodeGenerator_endScope();
    }

    void visit(Funcs& node) override {
        // // The (this) is the ScopePrinter
        this->codeBuffer.emit("declare i32 @printf(i8*, ...)");
        this->codeBuffer.emit("define i32 @main() {");
        for(auto& funcDecl : node.getFuncs()) {
            funcDecl->accept(*this);
        }
        this->codeBuffer.emit("}");
        // this->symbolTable.addFunctionSymbol("print", BuiltInType::VOID, {BuiltInType::STRING}, {"str"}, -1);
        // this->symbolTable.addFunctionSymbol("printi", BuiltInType::VOID, {BuiltInType::INT}, {"num"}, -1);
        // printer.emitFunc("print", BuiltInType::VOID, {BuiltInType::STRING});
        // printer.emitFunc("printi", BuiltInType::VOID, {BuiltInType::INT});

        // for (auto& funcDecl : node.getFuncs()) {
        //     this->symbolTable.addFunctionSymbol(funcDecl->getFuncId(), funcDecl->getFuncReturnType(), funcDecl->getFuncParams()->getFormalsType(), 
        //         funcDecl->getFuncParams()->getFormalsIds(), funcDecl->getFuncIdLine());
        // }
        // Symbol* main = symbolTable.getFuncSymbol("main", -1);
      
        // if(main == nullptr || main->getDataType() != BuiltInType::VOID || main->getParameterTypes().size() != 0) {
        //     errorMainMissing();
        // }

        // for (auto& funcDecl : node.getFuncs()) {
        //     funcDecl->accept(*this);
        // }
    }

    void printBuffer() {
        cout << this->codeBuffer << endl;
    }
};

#endif // CODE_GENERATOR_HPP
