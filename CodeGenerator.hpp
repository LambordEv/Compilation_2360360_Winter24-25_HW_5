#ifndef CODE_GENERATOR_HPP
#define CODE_GENERATOR_HPP

#include "visitor.hpp"
#include "symbolTable.hpp"
#include "output.hpp"
#include <string>
#include <stdexcept>
#include <vector>
#include <iostream>



const string PRINTI_Function = "@.int_specifier = constant [4 x i8] c\"%d\\0A\\00\" \n\
define void @printi(i32) { \n\
    %spec_ptr = getelementptr [4 x i8], [4 x i8]* @.int_specifier, i32 0, i32 0 \n\
    call i32 (i8*, ...) @printf(i8* %spec_ptr, i32 %0) \n\
    ret void \n\
}\n";

const string PRINT_Function = "@.str_specifier = constant [4 x i8] c\"%s\\0A\\00\" \n\
define void @print(i8*) { \n\
    %spec_ptr = getelementptr [4 x i8], [4 x i8]* @.str_specifier, i32 0, i32 0 \n\
    call i32 (i8*, ...) @printf(i8* %spec_ptr, i8* %0) \n\
    ret void \n\
}\n";

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
    string tabs = "";

public:
    // SemanticAnalyzer(SymbolTable* symbolTable)
    //     : symbolTable(symbolTable) {}
    CodeGenerator() : codeBuffer(), symbolTable() {}

    void CodeGenerator_beginScope(string scopeName = "", bool isLoopScope = false, string condition_Label = "", string done_Label = "") {
        if (!isLoopScope && symbolTable.getCurrentScope()->isInLoopScope()) {
            string conditionLabelPrev = symbolTable.getCurrentScope()->getConditionLabel();
            string doneLabelPrev = symbolTable.getCurrentScope()->getDoneLabel();
            symbolTable.beginScope(true, scopeName);
            symbolTable.getCurrentScope()->setConditionLabel(conditionLabelPrev);
            symbolTable.getCurrentScope()->setDoneLabel(doneLabelPrev);
        }
        else if (isLoopScope) {
            symbolTable.beginScope(isLoopScope, scopeName);
            symbolTable.getCurrentScope()->setConditionLabel(condition_Label);
            symbolTable.getCurrentScope()->setDoneLabel(done_Label);
        }
        else {
            symbolTable.beginScope(false, scopeName);
        }
        tabs += "\t";
    }

    void CodeGenerator_endScope() {
        symbolTable.endScope();
        tabs = tabs.substr(0, tabs.size() - 1);
    }

    // Implementations of visit methods
    void visit(Num& node) override { 
        RegisterStruct currVar{this->codeBuffer.freshVar(), 0 == node.getValueInt()};
        currVar.setRegisterValue(true, node.getValueInt());

        this->codeBuffer << tabs << currVar.name << " = add i32 " << node.getValueInt() << ", 0" << endl;
        node.setRegister(currVar);
    }

    void visit(NumB& node) override {
        RegisterStruct currVar{this->codeBuffer.freshVar(), 0 == node.getValueInt()};
        currVar.setRegisterValue(true, node.getValueInt());

        this->codeBuffer << tabs << currVar.name << " = add i32 " << node.getValueInt() << ", 0" << endl;
        RegisterStruct tmpVar = currVar;
        currVar = {this->codeBuffer.freshVar(), tmpVar.isZero};
        this->codeBuffer << tabs << currVar.name << " = and i32 " << tmpVar.name << ", 255" << endl;
        node.setRegister(currVar);
    }

    void visit(String& node) override {
        const int strSize = node.getValueStr().size() + 1;
        const string strIdentifier = this->codeBuffer.emitString(node.getValueStr());
        RegisterStruct currVar{this->codeBuffer.freshVar(), false};
        this->codeBuffer << tabs << currVar.name << " = getelementptr [" << strSize << " x i8], [" << strSize << " x i8]* " << strIdentifier << ", i32 0, i32 0" << endl;
        node.setRegister(currVar);
        node.setType(NODE_String);
    }

    void visit(Bool& node) override {
        RegisterStruct currVar = {this->codeBuffer.freshVar(), true};
        int initBoolValue = node.getValueBool() ? 1 : 0;
        currVar.isZero = !node.getValueBool();
        this->codeBuffer << tabs << currVar.name << " = add i32 " << initBoolValue << ", 0" << endl;
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

        RegisterStruct leftReg{"left", false};
        RegisterStruct rightReg{"right", false};
        RegisterStruct leftValue;
        RegisterStruct rightValue;

        if(node.getLeft()->getType() == NODE_ID){
            leftReg = this->symbolTable.getRegFromSymTable(node.getLeft()->getValueStr());
            leftValue = {this->codeBuffer.freshVar(), true};
            this->codeBuffer << tabs << leftValue.name << " = load i32, i32* " << leftReg.name << endl;
            leftValue.setRegisterValue(leftReg.isRegisterValueKnown, leftReg.getRegisterValue());
        } else {
            leftValue = node.getLeft()->getRegister(); // Maybe a result of add leftReg, 0
        }
        if(node.getRight()->getType() == NODE_ID){
            rightReg = this->symbolTable.getRegFromSymTable(node.getRight()->getValueStr());
            rightValue = {this->codeBuffer.freshVar(), true};
            this->codeBuffer << tabs << rightValue.name << " = load i32, i32* " << rightReg.name << endl;
            rightValue.setRegisterValue(rightReg.isRegisterValueKnown, rightReg.getRegisterValue());
        } else {
            rightValue = node.getRight()->getRegister(); // Maybe a result of add rightReg, 0
        }        

        RegisterStruct currVar = {this->codeBuffer.freshVar(), true};
        string resultText = currVar.name;
        switch(node.getOp()) {
            case BinOpType::ADD:
                // if left in numb and right is numb then truncate, else already written 
                currVar.setRegisterValue((leftValue.isRegisterValueKnown && rightValue.isRegisterValueKnown), leftValue.getRegisterValue() + rightValue.getRegisterValue());
                resultText += " = add i32 " + leftValue.name + ", " + rightValue.name;
                if((leftValue.isZero && rightValue.isZero)){
                    currVar.setRegisterValue(true, 0);
                }
                break;
            case BinOpType::SUB:
                currVar.setRegisterValue((leftValue.isRegisterValueKnown && rightValue.isRegisterValueKnown), leftValue.getRegisterValue() - rightValue.getRegisterValue());
                resultText += " = sub i32 " + leftValue.name + ", " + rightValue.name;
                if(leftValue.name == rightValue.name){
                    currVar.setRegisterValue(true, 0);
                }
                break;
            case BinOpType::MUL:
                currVar.setRegisterValue((leftValue.isRegisterValueKnown && rightValue.isRegisterValueKnown), leftValue.getRegisterValue() * rightValue.getRegisterValue());
                resultText += " = mul i32 " + leftValue.name + ", " + rightValue.name;
                if((leftValue.isZero || rightValue.isZero)){
                    currVar.setRegisterValue(true, 0);
                }
                break;
            case BinOpType::DIV:
                if (rightValue.isZero) {
                    const string divisionByZero = "Error division by zero";
                    const string divZeroIdentifier = this->codeBuffer.emitString(divisionByZero);
                    const int strSize = divisionByZero.size() + 1;
                    RegisterStruct currVar{this->codeBuffer.freshVar(), false};
                    this->codeBuffer << tabs << currVar.name << " = getelementptr [" << strSize << " x i8], [" << strSize << " x i8]* " << divZeroIdentifier << ", i32 0, i32 0" << endl;

                    this->codeBuffer << tabs << "call void @print(i8* " + currVar.name + ")" << endl;
                    this->codeBuffer << tabs << "call void @exit(i32 0)" << endl;
                    //FXIME - Check why the fucking exit is still executing commands after it!
                    resultText += " = sdiv i32 1, 1 ; This is done since exit is not called in some fucking unexplained reason";

                } else {
                    resultText += " = sdiv i32 " + leftValue.name + ", " + rightValue.name;
                    currVar.setRegisterValue((leftValue.isRegisterValueKnown && rightValue.isRegisterValueKnown), leftValue.getRegisterValue() / rightValue.getRegisterValue());
                }
                if((leftValue.isZero && !rightValue.isZero)){
                    currVar.setRegisterValue(true, 0);
                }
                break;
        }
        this->codeBuffer << tabs << resultText << endl;
        if (!currVar.isZero) {
            if(binOp_ResultType(*node.getLeft(), *node.getRight(), this->symbolTable) == BYTE) {
                RegisterStruct tmpVar = currVar;
                currVar = {this->codeBuffer.freshVar(), tmpVar.isZero};
                this->codeBuffer << tabs << currVar.name + " = and i32 " + tmpVar.name + ", 255" << endl;
                currVar.setRegisterValue(tmpVar.isRegisterValueKnown, tmpVar.getRegisterValue() & 255);
            }
        }

        node.setRegister(currVar);
    }

    void visit(RelOp& node) override {
        node.getLeft()->accept(*this);
        node.getRight()->accept(*this);

        RegisterStruct leftReg{"Undef", true};
        RegisterStruct rightReg{"Undef", true};
        RegisterStruct leftValue;
        RegisterStruct rightValue;

        if(node.getLeft()->getType() == NODE_ID) {
            leftReg = this->symbolTable.getRegFromSymTable(node.getLeft()->getValueStr());
            leftValue = {this->codeBuffer.freshVar(), true};
            this->codeBuffer << tabs << leftValue.name << " = load i32, i32* " << leftReg.name << endl;
        } else {
            leftValue = node.getLeft()->getRegister(); // Maybe a result of add leftReg, 0
        }
        if(node.getRight()->getType() == NODE_ID) {
            rightReg = this->symbolTable.getRegFromSymTable(node.getRight()->getValueStr());
            rightValue = {this->codeBuffer.freshVar(), true};
            this->codeBuffer << tabs << rightValue.name << " = load i32, i32* " << rightReg.name << endl;
        } else {
            rightValue = node.getRight()->getRegister(); // Maybe a result of add rightReg, 0
        }

        RegisterStruct currVar{this->codeBuffer.freshVar(), true};
        string resultText = currVar.name;
        switch(node.getOp()) { 
            // TODO - Maybe we will have to jump to labels from here, where this code should be written? here or in the if/else and while
            case RelOpType::EQ:
                resultText += " = icmp eq i32 " + leftValue.name + ", " + rightValue.name;
                break;
            case RelOpType::NE:
                resultText += " = icmp ne i32 " + leftValue.name + ", " + rightValue.name;
                break;
            case RelOpType::LT:
                resultText += " = icmp slt i32 " + leftValue.name + ", " + rightValue.name;
                break;
            case RelOpType::GT:
                resultText += " = icmp sgt i32 " + leftValue.name + ", " + rightValue.name;
                break;
            case RelOpType::LE:
                resultText += " = icmp sle i32 " + leftValue.name + ", " + rightValue.name;
                break;
            case RelOpType::GE:
                resultText += " = icmp sge i32 " + leftValue.name + ", " + rightValue.name;
                break;
        }
        this->codeBuffer << tabs << resultText << endl;
        RegisterStruct tmpVar = currVar;
        currVar = {this->codeBuffer.freshVar(), tmpVar.isZero};
        this->codeBuffer << tabs << currVar.name << " = zext i1 " << tmpVar.name << " to i32" << endl;
        node.setRegister(currVar);
        node.setType(NODE_Bool);
    }

    void visit(Not& node) override {
        node.getExpr()->accept(*this);

        RegisterStruct expReg{"Undef", true};
        RegisterStruct expBoolValue;

        if(node.getExpr()->getType() == NODE_ID) {
            expReg = this->symbolTable.getRegFromSymTable(node.getExpr()->getValueStr());
            expBoolValue = {this->codeBuffer.freshVar(), true};
            this->codeBuffer << tabs << expBoolValue.name << " = load i32, i32* " << expReg.name << endl;
        } else {
            expBoolValue = node.getExpr()->getRegister(); // Maybe a result of add leftReg, 0
        }

        RegisterStruct currVar{this->codeBuffer.freshVar(), true};
        this->codeBuffer << tabs << currVar.name << " = xor i32 " << expBoolValue.name << ", 1" << endl;
        node.setRegister(currVar);
    }

    void visit(And& node) override { 
        // Lazy Evaluation - If Left is False, then Right is not evaluated
        const string andLabel = this->codeBuffer.freshLabel();
        // Flow Control Labels
        const string rightEvaluateLabel = andLabel + ".rightEvaluationSection";
        const string resultLabel = andLabel + ".resultSection";
        // Memory Allocation Labels
        const string leftOperand_ptr = "%allocation_" + andLabel.substr(1) + ".leftOperand";
        const string rightOperand_ptr = "%allocation_" + andLabel.substr(1) + ".rightOperand";

        RegisterStruct leftReg = {"Undef", true};
        RegisterStruct rightReg = {"Undef", true};
        RegisterStruct leftBoolValue;
        RegisterStruct rightBoolValue;

        // Prepare left and right values
        this->codeBuffer << tabs << leftOperand_ptr << " = alloca i1" << endl;
        this->codeBuffer << tabs << "store i1 0, i1* " << leftOperand_ptr << endl;
        this->codeBuffer << tabs << rightOperand_ptr << " = alloca i1" << endl;
        this->codeBuffer << tabs << "store i1 0, i1* " << rightOperand_ptr << endl;

        // Evaluate Left
        node.getLeft()->accept(*this);
        if(node.getLeft()->getType() == NODE_ID) {
            leftReg = this->symbolTable.getRegFromSymTable(node.getLeft()->getValueStr());
            leftBoolValue = {this->codeBuffer.freshVar(), true};
            this->codeBuffer << tabs << leftBoolValue.name << " = load i32, i32* " << leftReg.name << endl;
        } else {
            leftBoolValue = node.getLeft()->getRegister();
        }
        
        // Convert to i1 - The branch instruction requires i1 type
        RegisterStruct tmpVar = leftBoolValue;
        leftBoolValue = {this->codeBuffer.freshVar(), true};
        this->codeBuffer << tabs << leftBoolValue.name << " = trunc i32 " << tmpVar.name << " to i1" << endl;
        this->codeBuffer << tabs << "store i1 " << leftBoolValue.name << ", i1* " << leftOperand_ptr << endl;
        
        // If Left is False, then Right is not evaluated
        this->codeBuffer << tabs << "br i1 " << leftBoolValue.name << ", label " << rightEvaluateLabel << ", label " << resultLabel << endl;

        // Evaluate Right
        this->codeBuffer << "\n" << rightEvaluateLabel.substr(1) << ":" << endl;
        node.getRight()->accept(*this);
        if(node.getRight()->getType() == NODE_ID) {
            rightReg = this->symbolTable.getRegFromSymTable(node.getRight()->getValueStr());
            rightBoolValue = {this->codeBuffer.freshVar(), true};
            this->codeBuffer << tabs << rightBoolValue.name << " = load i32, i32* " << rightReg.name << endl;
        } else {
            rightBoolValue = node.getRight()->getRegister();
        }

        // Convert to i1 - The branch instruction requires i1 type
        tmpVar = rightBoolValue;
        rightBoolValue = {this->codeBuffer.freshVar(), true};
        this->codeBuffer << tabs << rightBoolValue.name << " = trunc i32 " << tmpVar.name << " to i1" << endl;
        this->codeBuffer << tabs << "store i1 " << rightBoolValue.name << ", i1* " << rightOperand_ptr << endl;

        this->codeBuffer << tabs << "br label " << resultLabel << endl;
        this->codeBuffer << "\n" << resultLabel.substr(1) << ":" << endl;

        // Evaluate And
        RegisterStruct currVar{this->codeBuffer.freshVar(), true};
        RegisterStruct leftOperandResult{this->codeBuffer.freshVar(), true};
        RegisterStruct rightOperandResult{this->codeBuffer.freshVar(), true};
        this->codeBuffer << tabs << leftOperandResult.name << " = load i1, i1* " << leftOperand_ptr << endl;
        this->codeBuffer << tabs << rightOperandResult.name << " = load i1, i1* " << rightOperand_ptr << endl;
        this->codeBuffer << tabs << currVar.name << " = and i1 " << leftOperandResult.name << ", " << rightOperandResult.name << endl;

        // Return to i32 type
        tmpVar = currVar;
        currVar = {this->codeBuffer.freshVar(), true};
        this->codeBuffer << tabs << currVar.name << " = zext i1 " << tmpVar.name << " to i32" << endl;
        node.setRegister(currVar);
    }

    void visit(Or& node) override {
        // Lazy Evaluation - If Left is True, then Right is not evaluated
        const string orLabel = this->codeBuffer.freshLabel();
        // Flow Control Labels
        const string rightEvaluateLabel = orLabel + ".rightEvaluationSection";
        const string resultLabel = orLabel + ".resultSection";
        // Memory Allocation Labels
        const string leftOperand_ptr = "%allocation_" + orLabel.substr(1) + ".leftOperand";
        const string rightOperand_ptr = "%allocation_" + orLabel.substr(1) + ".rightOperand";

        RegisterStruct leftReg{"Undef", true};
        RegisterStruct rightReg{"Undef", true};
        RegisterStruct leftBoolValue;
        RegisterStruct rightBoolValue;

        // Prepare left and right values
        this->codeBuffer << tabs << leftOperand_ptr << " = alloca i1" << endl;
        this->codeBuffer << tabs << "store i1 0, i1* " << leftOperand_ptr << endl;
        this->codeBuffer << tabs << rightOperand_ptr << " = alloca i1" << endl;
        this->codeBuffer << tabs << "store i1 0, i1* " << rightOperand_ptr << endl;

        // Evaluate Left
        node.getLeft()->accept(*this);
        if(node.getLeft()->getType() == NODE_ID) {
            leftReg = this->symbolTable.getRegFromSymTable(node.getLeft()->getValueStr());
            leftBoolValue = {this->codeBuffer.freshVar(), true};
            this->codeBuffer << tabs << leftBoolValue.name << " = load i32, i32* " << leftReg.name << endl;
        } else {
            leftBoolValue = node.getLeft()->getRegister();
        }
        
        // Convert to i1 - The branch instruction requires i1 type
        RegisterStruct tmpVar = leftBoolValue;
        leftBoolValue = {this->codeBuffer.freshVar(), true};
        this->codeBuffer << tabs << leftBoolValue.name << " = trunc i32 " << tmpVar.name << " to i1" << endl;
        this->codeBuffer << tabs << "store i1 " << leftBoolValue.name << ", i1* " << leftOperand_ptr << endl;
        
        // If Left is True, then Right is not evaluated
        this->codeBuffer << tabs << "br i1 " << leftBoolValue.name << ", label " << resultLabel << ", label " << rightEvaluateLabel << endl;

        // Evaluate Right
        this->codeBuffer << "\n" << rightEvaluateLabel.substr(1) << ":" << endl;
        node.getRight()->accept(*this);
        if(node.getRight()->getType() == NODE_ID) {
            rightReg = this->symbolTable.getRegFromSymTable(node.getRight()->getValueStr());
            rightBoolValue = {this->codeBuffer.freshVar(), true};
            this->codeBuffer << tabs << rightBoolValue.name << " = load i32, i32* " << rightReg.name << endl;
        } else {
            rightBoolValue = node.getRight()->getRegister();
        }

        // Convert to i1 - The branch instruction requires i1 type
        tmpVar = rightBoolValue;
        rightBoolValue = {this->codeBuffer.freshVar(), true};
        this->codeBuffer << tabs << rightBoolValue.name << " = trunc i32 " << tmpVar.name << " to i1" << endl;
        this->codeBuffer << tabs << "store i1 " << rightBoolValue.name << ", i1* " << rightOperand_ptr << endl;

        this->codeBuffer << tabs << "br label " << resultLabel << endl;
        this->codeBuffer << "\n" << resultLabel.substr(1) << ":" << endl;

        // Evaluate Or
        RegisterStruct currVar{this->codeBuffer.freshVar(), true};
        RegisterStruct leftOperandResult{this->codeBuffer.freshVar(), true};
        RegisterStruct rightOperandResult{this->codeBuffer.freshVar(), true};
        this->codeBuffer << tabs << leftOperandResult.name << " = load i1, i1* " << leftOperand_ptr << endl;
        this->codeBuffer << tabs << rightOperandResult.name << " = load i1, i1* " << rightOperand_ptr << endl;
        this->codeBuffer << tabs << currVar.name << " = or i1 " << leftOperandResult.name << ", " << rightOperandResult.name << endl;

        // Return to i32 type
        tmpVar = currVar;
        currVar = {this->codeBuffer.freshVar(), true};
        this->codeBuffer << tabs << currVar.name << " = zext i1 " << tmpVar.name << " to i32" << endl;
        node.setRegister(currVar);
    }

    void visit(Type& node) override {
        // Not Needed ?
    }

    void visit(Cast& node) override {
        node.getExpr()->accept(*this);
        RegisterStruct currVar = {this->codeBuffer.freshVar(), true};
        currVar.setRegisterValue(false, 0);
        RegisterStruct expReg;
        RegisterStruct tmpVar{this->codeBuffer.freshVar(), true};

        if(NODE_ID == node.getExpr()->getType()) {
            expReg = this->symbolTable.getRegFromSymTable(node.getExpr()->getValueStr());
            this->codeBuffer << tabs << tmpVar.name << " = load i32, i32* " << expReg.name << endl;
            tmpVar.setRegisterValue(expReg.isRegisterValueKnown, expReg.getRegisterValue());
        } else {
            expReg = node.getExpr()->getRegister();
            this->codeBuffer << tabs << tmpVar.name << " = add i32 " << expReg.name << ", 0" << endl;
            tmpVar.setRegisterValue(expReg.isRegisterValueKnown, expReg.getRegisterValue());
        }

        if(BYTE == node.getTargetType()) {
            this->codeBuffer << tabs << currVar.name << " = and i32 " << tmpVar.name << ", 255" << endl;
            currVar.setRegisterValue(tmpVar.isRegisterValueKnown, tmpVar.getRegisterValue() & 255);
        } else {
            this->codeBuffer << tabs << currVar.name << " = add i32 " << tmpVar.name << ", 0" << endl;
            currVar.setRegisterValue(tmpVar.isRegisterValueKnown, tmpVar.getRegisterValue());
        }
        node.setRegister(currVar);
    }

    void visit(ExpList& node) override {
        // TO DO - Check if need to updated implementation
        for (auto& expr : node.getExpressions()) {
            expr->accept(*this);
        }
    }

    void visit(Call& node) override {
        node.getArgsExp()->accept(*this);

        RegisterStruct regNew = {this->codeBuffer.freshVar(), true};
        string callBuffer = "";
        const string funcID = node.getFuncId();

        BuiltInType returnType = symbolTable.getFuncSymbol(funcID)->getDataType();
        if (returnType == VOID) {
            callBuffer = "call void ";
        } else {
            callBuffer = regNew.name + " = call i32 ";
        }
        node.setRegister(regNew);
        callBuffer += "@" + funcID + "(";

        vector<shared_ptr<Exp>> params = node.getArgs();
        for(auto param : params) {
            RegisterStruct regParam;
            if (param->getType() == NODE_ID) {
                regParam = symbolTable.getRegFromSymTable(param->getValueStr());
                string tempReg = this->codeBuffer.freshVar();
                this->codeBuffer << tabs << tempReg << " = load i32, i32* " << regParam.name << endl;
                if (funcID == "print" || funcID == "printf") {
                    callBuffer += "i8* " + tempReg + ", ";
                } else {
                    callBuffer += "i32 " + tempReg + ", ";
                }
            } else {
                regParam = param->getRegister();
                if (funcID == "print" || funcID == "printf") {
                    callBuffer += "i8* " + regParam.name + ", ";
                } else {
                    callBuffer += "i32 " + regParam.name + ", ";
                }
            }

        }
        callBuffer = ((params.size() != 0) ? callBuffer.substr(0, callBuffer.size() - 2) : callBuffer) + ")";
        
        this->codeBuffer << tabs << callBuffer << endl;
    }

    void visit(Statements& node) override {
        for (auto& statement : node.getStatements()) {
            if (statement->getType() == NODE_Statements) {
                CodeGenerator_beginScope();
            }
            statement->accept(*this);
            if (statement->getType() == NODE_Statements) {
                CodeGenerator_endScope();
            }
        }
    }

    void visit(Break& node) override {
        if ((symbolTable.getCurrentScope()->isInLoopScope())) {
            const string done_label = this->symbolTable.getCurrentScope()->getDoneLabel();
            //cout << "Break Label: " << done_label << endl;
            this->codeBuffer << tabs << "br label " << done_label << endl;
        }
    }

    void visit(Continue& node) override {
        if ((symbolTable.getCurrentScope()->isInLoopScope())) {
            const string condition_Label = this->symbolTable.getCurrentScope()->getConditionLabel();
            //cout << "Continue Label: " << condition_Label << endl;
            this->codeBuffer << tabs << "br label " << condition_Label << endl;
        }
    }

    void visit(Return& node) override {
        if (!node.getExpr()) {
            this->codeBuffer << tabs << "ret void" << endl;
        } else {
            node.getExpr()->accept(*this);
            if (node.getExpr()->getType() == NODE_ID) {
                RegisterStruct retReg = this->symbolTable.getRegFromSymTable(node.getExpr()->getValueStr());
                string tempReg = this->codeBuffer.freshVar();
                codeBuffer << tabs << tempReg << " = load i32, i32* " << retReg.name << endl;
                this->codeBuffer << tabs << "ret i32 " << tempReg << endl;
            } else {
                RegisterStruct retReg = node.getExpr()->getRegister();
                this->codeBuffer << tabs << "ret i32 " << retReg.name << endl;
            }
        }
      
    }

    void visit(If& node) override {
        const string if_else_Label = this->codeBuffer.freshLabel();
        // Flow Control Labels
        const string then_Label = if_else_Label + ".then";
        const string else_Label = if_else_Label + ".else";
        const string done_Label = if_else_Label + ".finale";
        CodeGenerator_beginScope();

        RegisterStruct conditionReg{this->codeBuffer.freshVar(), true};
        node.getCondition()->accept(*this);
        if(NODE_ID == node.getCondition()->getType()){
            RegisterStruct condIdReg{this->codeBuffer.freshVar(), true};
            RegisterStruct tmpReg = this->symbolTable.getRegFromSymTable(node.getCondition()->getValueStr());
            this->codeBuffer << tabs << condIdReg.name << " = load i32, i32* " << tmpReg.name << endl;
            this->codeBuffer << tabs << conditionReg.name << " = trunc i32 " << condIdReg.name << " to i1" << endl;
            conditionReg.setRegisterValue(tmpReg.isRegisterValueKnown, tmpReg.getRegisterValue());
        } else {
            RegisterStruct tmpReg = node.getCondition()->getRegister();
            this->codeBuffer << tabs << conditionReg.name << " = trunc i32 " << tmpReg.name << " to i1" << endl;
            conditionReg.setRegisterValue(tmpReg.isRegisterValueKnown, tmpReg.getRegisterValue());
        }
        this->codeBuffer << tabs << "br i1 " << conditionReg.name << ", label " << then_Label << ", label " << else_Label << endl;
        this->codeBuffer << "\n" << then_Label.substr(1) << ":" << endl;

        if (node.getThen()->getType() == NODE_Statements) {
            CodeGenerator_beginScope();
        }
        node.getThen()->accept(*this);
        this->codeBuffer << tabs << "br label " << done_Label << endl;
        if (node.getThen()->getType() == NODE_Statements) {
            CodeGenerator_endScope();
        }
        CodeGenerator_endScope();

        this->codeBuffer << "\n" << else_Label.substr(1) << ":" << endl;
        if (node.getElse()) {
            CodeGenerator_beginScope();
            if(node.getElse()->getType() == NODE_Statements) {
                CodeGenerator_beginScope();
            }
            node.getElse()->accept(*this);
            this->codeBuffer << tabs << "br label " << done_Label << endl;
            if(node.getElse()->getType() == NODE_Statements) {
                CodeGenerator_endScope();
            }
            CodeGenerator_endScope();
        }
        this->codeBuffer << tabs << "br label " << done_Label << endl;
        this->codeBuffer << "\n" << done_Label.substr(1) << ":" << endl;
    }

    void visit(While& node) override {
        const string while_label = this->codeBuffer.freshLabel();
        // Flow Control Labels
        const string condition_Label = while_label + ".while_condition";
        const string body_Label = while_label + ".while_body";
        const string done_Label = while_label + ".while_finale";
        CodeGenerator_beginScope();
        this->symbolTable.getCurrentScope()->setInLoopScope(true);
        this->symbolTable.getCurrentScope()->setConditionLabel(condition_Label);
        this->symbolTable.getCurrentScope()->setDoneLabel(done_Label);

        this->codeBuffer << tabs << "br label " << condition_Label << endl;
        this->codeBuffer << "\n" << condition_Label.substr(1) << ":" << endl;  
        RegisterStruct conditionReg{this->codeBuffer.freshVar(), true};
        node.getCondition()->accept(*this);
        if(NODE_ID == node.getCondition()->getType()){
            RegisterStruct condIdReg{this->codeBuffer.freshVar(), true};
            RegisterStruct tmpReg = this->symbolTable.getRegFromSymTable(node.getCondition()->getValueStr());
            this->codeBuffer << tabs << condIdReg.name << " = load i32, i32* " << tmpReg.name << endl;
            this->codeBuffer << tabs << conditionReg.name << " = trunc i32 " << condIdReg.name << " to i1" << endl;
            conditionReg.setRegisterValue(tmpReg.isRegisterValueKnown, tmpReg.getRegisterValue());
        } else {
            RegisterStruct tmpReg = node.getCondition()->getRegister();
            this->codeBuffer << tabs << conditionReg.name << " = trunc i32 " << tmpReg.name << " to i1" << endl;
            conditionReg.setRegisterValue(tmpReg.isRegisterValueKnown, tmpReg.getRegisterValue());
        }
        this->codeBuffer << tabs << "br i1 " << conditionReg.name << ", label " << body_Label << ", label " << done_Label << endl;
        this->codeBuffer << "\n" << body_Label.substr(1) << ":" << endl;

        if (node.getBody()->getType() == NODE_Statements) {
            CodeGenerator_beginScope();
        }
        node.getBody()->accept(*this);
        this->codeBuffer << tabs << "br label " << condition_Label << endl;
        this->codeBuffer << tabs << "br label " << done_Label << endl;
        if (node.getBody()->getType() == NODE_Statements) {
            CodeGenerator_endScope();
        }
        this->codeBuffer << "\n" << done_Label.substr(1) << ":" << endl;
        CodeGenerator_endScope();
    }

    void visit(VarDecl& node) override {
        // Define the variable ptr
        RegisterStruct currVar{this->codeBuffer.freshVar(), true};
        currVar.setRegisterValue(true, 0);
        const string varID = node.getVarId()->getValueStr();

        // allocate memory for the new variable on the stack
        this->codeBuffer << tabs << currVar.name << " = alloca i32" << endl;

        RegisterStruct valueReg{this->codeBuffer.freshVar(), true};
        valueReg.setRegisterValue(true, 0);
        if(node.getVarInitExp()){
            node.getVarInitExp()->accept(*this);
            RegisterStruct expReg{"Undef", true};
            if (node.getVarInitExp()->getType() == NODE_ID){
                // The InitExp is a variable that is stored in the memory and we need to load it
                expReg = this->symbolTable.getRegFromSymTable(node.getVarInitExp()->getValueStr());
                this->codeBuffer << tabs << valueReg.name << " = load i32, i32* " << expReg.name << endl;
            } else {
                // The InitExp is a constant value
                expReg = node.getVarInitExp()->getRegister();
                this->codeBuffer << tabs << valueReg.name << " = add i32 " << expReg.name << ", 0" << endl;
            }
            valueReg.isZero = expReg.isZero;
            valueReg.setRegisterValue(expReg.isRegisterValueKnown, expReg.getRegisterValue());
        } else {
            // The InitExp is not defined, so we initialize the variable with 0 which is the default value
            this->codeBuffer << tabs << valueReg.name << " = add i32 0, 0" << endl;
            valueReg.isZero = true;
            valueReg.setRegisterValue(true, 0);
        }

        // Store the value of the initialization expression in the new variable
        this->codeBuffer << tabs << "store i32 " << valueReg.name << ", i32* " << currVar.name << endl;
        currVar.isZero = valueReg.isZero;
        currVar.setRegisterValue(valueReg.isRegisterValueKnown, valueReg.getRegisterValue());
        // Add the new variable to the symbol table
        this->symbolTable.addVariableSymbol(node.getValueStr(), node.getVarType(), node.getLine());
        // Set the register name of the variable in the symbol table
        this->symbolTable.setRegInSymTable(varID, currVar);
        node.getVarId()->accept(*this);
    }

    void visit(Assign& node) override {
        RegisterStruct currVar = this->symbolTable.getRegFromSymTable(node.getValueStr());
        RegisterStruct tmpVar = {this->codeBuffer.freshVar(), true};
        RegisterStruct expReg = {"Undef", false};

        node.getAssignExp()->accept(*this);

        if(node.getAssignExp()->getType() == NODE_ID) {
            expReg = this->symbolTable.getRegFromSymTable(node.getAssignExp()->getValueStr());
            this->codeBuffer << tabs << tmpVar.name << " = load i32, i32* " << expReg.name << endl;
            this->codeBuffer << tabs << "store i32 " << tmpVar.name << ", i32* " << currVar.name << endl;
        } else {
            expReg = node.getAssignExp()->getRegister();
            this->codeBuffer << tabs << "store i32 " << expReg.name << ", i32* " << currVar.name << endl;
        }
        currVar.isZero = expReg.isZero;
        currVar.setRegisterValue(expReg.isRegisterValueKnown, expReg.getRegisterValue());
        // cout << node.getValueStr() << " = " << currVar.getRegisterValue() << endl;
        // cout << node.getValueStr() << " = " << currVar.isZero << endl;
        // cout << node.getValueStr() << " = " << expReg.isRegisterValueKnown << endl;
        this->symbolTable.setRegInSymTable(node.getValueStr(), currVar);
    }

    void visit(Formal& node) override {
        symbolTable.addParameterSymbol(node.getFormalId(), node.getFormalType(), node.getLine());
        Symbol* symbol = symbolTable.getSymbol(node.getFormalId());
        
        RegisterStruct newParam = {this->codeBuffer.freshVar(), false};
        newParam.setRegisterValue(false);

        // Get the parameters offset 
        this->codeBuffer << tabs << newParam.name << " = alloca i32" << endl;
        this->codeBuffer << tabs << "store i32 %" << (0 - symbol->getOffset() - 1) << ", i32* " << newParam.name << endl;
        symbolTable.setRegInSymTable(node.getFormalId(), newParam);
    }

    void visit(Formals& node) override {
        for (auto& formal : node.getFormals()) {
            formal->accept(*this);
        }
    }

    void visit(FuncDecl& node) override {
        vector<BuiltInType> paramsTypes = node.getFuncParams()->getFormalsType();
        
        string funcPrototype = "define ";
        funcPrototype += (VOID == node.getFuncReturnType()) ? "void " : "i32 ";
        funcPrototype += "@" + node.getFuncId() + " (";
        for(auto param : paramsTypes) funcPrototype += "i32, ";
        funcPrototype = ((paramsTypes.size() != 0) ? funcPrototype.substr(0, funcPrototype.size() - 2) : funcPrototype) + ") {";
        
        this->codeBuffer << tabs << funcPrototype << endl;
        CodeGenerator_beginScope(node.getFuncId(), false);
        // TODO - Each Parameter should be added to the scope as was done in HW_3
        node.getFuncParams()->accept(*this);
        node.getFuncBody()->accept(*this);
        if (VOID == node.getFuncReturnType()) {
            this->codeBuffer << tabs << "ret void" << endl;
        } else {
            this->codeBuffer << tabs << "ret i32 0" << endl;
        }
        CodeGenerator_endScope();
        this->codeBuffer << tabs << "}\n\n";
    }

    void visit(Funcs& node) override {
        // // The (this) is the ScopePrinter
        this->symbolTable.addFunctionSymbol("print", BuiltInType::VOID, {BuiltInType::STRING}, {"str"}, -1);
        this->symbolTable.addFunctionSymbol("printi", BuiltInType::VOID, {BuiltInType::INT}, {"num"}, -1);
        //this->symbolTable.addFunctionSymbol("printf", BuiltInType::INT, {BuiltInType::INT}, {"num"}, -1);
        this->symbolTable.addFunctionSymbol("exit", BuiltInType::VOID, {BuiltInType::INT}, {"code"}, -1);

        this->codeBuffer.emit("declare i32 @printf(i8*, ...)");
        this->codeBuffer.emit("declare void @exit(i32)");
        this->codeBuffer.emit(PRINTI_Function);
        this->codeBuffer.emit(PRINT_Function);

        for (auto& funcDecl : node.getFuncs()) {
            this->symbolTable.addFunctionSymbol(funcDecl->getFuncId(), funcDecl->getFuncReturnType(), funcDecl->getFuncParams()->getFormalsType(), 
                funcDecl->getFuncParams()->getFormalsIds(), funcDecl->getFuncIdLine());
        }
        Symbol* main = symbolTable.getFuncSymbol("main");
      
        for(auto& funcDecl : node.getFuncs()) {
            funcDecl->accept(*this);
        }
    }

    void printBuffer() {
        cout << this->codeBuffer << tabs << endl;
    }
};

#endif // CODE_GENERATOR_HPP
