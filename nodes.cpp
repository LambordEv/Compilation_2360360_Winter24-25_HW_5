#include "nodes.hpp"
#include <iostream>
#include <string>
#include <utility>

extern int yylineno;
extern char *yytext;

using namespace ast;

static bool checkTypesForRelOp(SemanticNodeType left, SemanticNodeType right) {
        // printf("left: %d, right: %d\n", left, right);
        if ((left == NODE_NumB && right == NODE_NumB) ||
            (left == NODE_Num && right == NODE_Num) ||
            (left == NODE_NumB && right == NODE_Num) ||
            (left == NODE_Num && right == NODE_NumB)) {
                return true;
        }
        return false;
}

namespace ast {

    Node::Node() : line(yylineno), text(yytext) {
        this->nodeType = NODE_Undecided;
    }

    Num::Num(const std::string str = "") : Exp(), value(std::stoi(str)) {
        this->text = str;
        this->nodeType = NODE_Num;
    }

    NumB::NumB(const std::string str = "") : Exp(), value(std::stoi(str)) {
        this->nodeType = NODE_NumB;
    }

    String::String(const std::string str = "") : Exp(), value(str) {
        // Remove the quotes
        value = value.substr(1, value.size() - 2);
        this->nodeType = NODE_String;
    }

    Bool::Bool(bool value) : Exp(), value(value) {
        this->nodeType = NODE_Bool;
    }

    ID::ID(const std::string str = "") : Exp(), value(str) {
        this->nodeType = NODE_ID;
    }

    BinOp::BinOp(BinOpType op, std::shared_ptr<Exp> left, std::shared_ptr<Exp> right)
            : Exp(), left(std::move(left)), right(std::move(right)), op(op) {
                this->nodeType = NODE_BinOP;
                
                //std::cout << "Type of left --- " << typeid(*left).name() << std::endl;
                //std::cout << "Type of right --- " << typeid(*this->right).name() << std::endl;
                // if (left && right && 
                //         (left->getType() == BuiltInType::BYTE && right->getType() == BuiltInType::BYTE)) {
                //         this->resultType = BuiltInType::BYTE;
                // }
                // else if (left && right && 
                //         ((left->getType() == BuiltInType::INT && right->getType() == BuiltInType::INT) ||
                //         (left->getType() == BuiltInType::BYTE && right->getType() == BuiltInType::INT) ||
                //         (left->getType() == BuiltInType::INT && right->getType() == BuiltInType::BYTE))) {
                //         this->resultType = BuiltInType::INT;
                // } else {
                //         printf("Type was not decided because %s is null\n", (left ? (right ? "something failed" : "right") : "left"));
                // }
        }

    RelOp::RelOp(RelOpType op, std::shared_ptr<Exp> left, std::shared_ptr<Exp> right)
            : Exp(), left(std::move(left)), right(std::move(right)), op(op) {
                this->nodeType = NODE_RelOP;
                // if (left && right && 
                //         checkTypesForRelOp(left->getType(), right->getType())) {
                //         this->resultType = BuiltInType::BOOL;
                // }
            }

    Type::Type(BuiltInType type) : Node(), type(type) {
        this->nodeType = NODE_Type;
    }

    Cast::Cast(std::shared_ptr<Exp> exp, std::shared_ptr<Type> target_type)
            : Exp(), exp(std::move(exp)), target_type(std::move(target_type)) {
                this->nodeType = NODE_Cast;
            }

    Not::Not(std::shared_ptr<Exp> exp) : Exp(), exp(std::move(exp)) {
        this->nodeType = NODE_Not;
        // if (exp && 
        //         exp->getType() == BuiltInType::BOOL) {
        //         this->resultType = BuiltInType::BOOL;
        // }
    }

    And::And(std::shared_ptr<Exp> left, std::shared_ptr<Exp> right)
            : Exp(), left(std::move(left)), right(std::move(right)) {
                this->nodeType = NODE_And;
                // if (left && right && 
                //         left->getType() == BuiltInType::BOOL && right->getType() == BuiltInType::BOOL) {
                //         this->resultType = BuiltInType::BOOL;
                // }
            }

    Or::Or(std::shared_ptr<Exp> left, std::shared_ptr<Exp> right)
            : Exp(), left(std::move(left)), right(std::move(right)) {
                this->nodeType = NODE_Or;
                // if (left && right && 
                //         left->getType() == BuiltInType::BOOL && right->getType() == BuiltInType::BOOL) {
                //         this->resultType = BuiltInType::BOOL;
                // }
            }

    ExpList::ExpList(std::shared_ptr<Exp> exp) : Node(), exps({std::move(exp)}) {
        this->nodeType = NODE_ExpList;
    }

    void ExpList::push_front(const std::shared_ptr<Exp> &exp) {
        exps.insert(exps.begin(), exp);
    }

    void ExpList::push_back(const std::shared_ptr<Exp> &exp) {
        exps.push_back(exp);
    }

    Call::Call(std::shared_ptr<ID> func_id, std::shared_ptr<ExpList> args)
            : Exp(), func_id(std::move(func_id)), args(std::move(args)) {
                this->nodeType = NODE_Call;
            }

    Call::Call(std::shared_ptr<ID> func_id)
            : Exp(), func_id(std::move(func_id)), args(std::make_shared<ExpList>()) {
                this->nodeType = NODE_Call;
            }

    Statements::Statements(std::shared_ptr<Statement> statement) : Statement(), statements({std::move(statement)}) {
        this->nodeType = NODE_Statements;
    }

    void Statements::push_front(const std::shared_ptr<Statement> &statement) {
        statements.insert(statements.begin(), statement);
    }

    void Statements::push_back(const std::shared_ptr<Statement> &statement) {
        statements.push_back(statement);
    }

    Return::Return(std::shared_ptr<Exp> exp) : Statement(), exp(std::move(exp)) {
        this->nodeType = NODE_Return;
    }

    If::If(std::shared_ptr<Exp> condition, std::shared_ptr<Statement> then, std::shared_ptr<Statement> otherwise)
            : Statement(), condition(std::move(condition)), then(std::move(then)), otherwise(std::move(otherwise)) {
                this->nodeType = NODE_If;
            }

    While::While(std::shared_ptr<Exp> condition, std::shared_ptr<Statement> body)
            : Statement(), condition(std::move(condition)),
              body(std::move(body)) {
                this->nodeType = NODE_While;
              }

    VarDecl::VarDecl(std::shared_ptr<ID> id, std::shared_ptr<Type> type, std::shared_ptr<Exp> init_exp)
            : Statement(), id(std::move(std::move(id))), type(std::move(type)), init_exp(std::move(init_exp)) {
                this->nodeType = NODE_VarDecl;
            }

    Assign::Assign(std::shared_ptr<ID> id, std::shared_ptr<Exp> exp)
            : Statement(), id(std::move(id)), exp(std::move(exp)) {
                this->nodeType = NODE_Assign;
             }

    Formal::Formal(std::shared_ptr<ID> id, std::shared_ptr<Type> type)
            : Node(), id(std::move(id)), type(std::move(type)) {
                this->nodeType = NODE_Formal;
            }

    Formals::Formals(std::shared_ptr<Formal> formal) : Node(), formals({std::move(formal)}) {
        this->nodeType = NODE_Formals;
    }

    void Formals::push_front(const std::shared_ptr<Formal> &formal) {
        formals.insert(formals.begin(), formal);
    }

    void Formals::push_back(const std::shared_ptr<Formal> &formal) {
        formals.push_back(formal);
    }

    FuncDecl::FuncDecl(std::shared_ptr<ID> id, std::shared_ptr<Type> return_type, std::shared_ptr<Formals> formals,
                       std::shared_ptr<Statements> body)
            : Node(), id(std::move(id)), return_type(std::move(return_type)), formals(std::move(formals)),
              body(std::move(body)) { 
                this->nodeType = NODE_FuncDecl;
              }

    Funcs::Funcs(std::shared_ptr<FuncDecl> func) : Node(), funcs({std::move(func)}) {
        this->nodeType = NODE_Funcs;
    }

    void Funcs::push_front(const std::shared_ptr<FuncDecl> &func) {
        funcs.insert(funcs.begin(), func);
    }

    void Funcs::push_back(const std::shared_ptr<FuncDecl> &func) {
        funcs.push_back(func);
    }

}