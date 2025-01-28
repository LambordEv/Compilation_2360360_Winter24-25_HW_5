%{

#include <iostream>
#include "nodes.hpp"
#include "output.hpp"
#include "visitor.hpp"

// bison declarations
extern int yylineno;
extern char* yytext;
extern int yylex();

using namespace std;
using namespace ast;

void yyerror(const char*);
static RelOpType whatRelOpRecieved(string);
static BinOpType whatBinOpRecieved(string);
static BuiltInType whatTypeReceived(string);

// root of the AST, set by the parser and used by other parts of the compiler
shared_ptr<Node> program;

// TODO: Place any additional declarations here
%}



// TODO: Define tokens here
%nonassoc   T_ID
%nonassoc   T_STRING
%nonassoc   T_NUM
%nonassoc   T_NUM_B
%nonassoc   T_COMMA
%nonassoc   T_SC
%nonassoc   T_CONTINUE
%nonassoc   T_BREAK
%nonassoc   T_WHILE
%nonassoc   T_IF
%nonassoc   T_RETURN
%nonassoc   T_FALSE
%nonassoc   T_TRUE
%nonassoc   T_BOOL
%nonassoc   T_BYTE
%nonassoc   T_INT
%nonassoc   T_VOID

// TODO: Define precedence and associativity here
%right      T_ASSIGN
%left       T_OR
%left       T_AND
%left       T_RELOP
%left       T_ADD_SUB
%left       T_MUL_DIV
%right      T_NOT
%left       T_LPAREN
%left       T_RPAREN
%left       T_LBRACE
%left       T_RBRACE
%right      T_ELSE

%%

// While reducing the start variable, set the root of the AST
Program: Funcs { program = $1; };

// TODO: Define grammar here
Funcs: { $$ = make_shared<Funcs>(); }
            | FuncDecl Funcs 
            { 
                shared_ptr<Funcs> funcs_ptr = dynamic_pointer_cast<Funcs>($2);
                shared_ptr<FuncDecl> singleFunc_ptr = dynamic_pointer_cast<FuncDecl>($1);
                funcs_ptr->push_front(singleFunc_ptr);
                $$ = funcs_ptr;
            }


FuncDecl: RetType T_ID T_LPAREN Formals T_RPAREN T_LBRACE Statements T_RBRACE 
            { 
                shared_ptr<ID> id_ptr = dynamic_pointer_cast<ID>($2);
                shared_ptr<Type> retType_ptr = dynamic_pointer_cast<Type>($1);
                shared_ptr<Formals> formals_ptr = dynamic_pointer_cast<Formals>($4);
                shared_ptr<Statements> statements_ptr = dynamic_pointer_cast<Statements>($7);
                $$ = make_shared<FuncDecl>(id_ptr, retType_ptr, formals_ptr, statements_ptr); 
            }

RetType: Type { $$ = make_shared<Type>(whatTypeReceived($1->text)); }
        | T_VOID { $$ = make_shared<Type>(whatTypeReceived($1->text)); }

Formals: { $$ = make_shared<Formals>(); }
        | FormalsList { $$ = $1; }

FormalsList: FormalDecl { $$ = make_shared<Formals>(dynamic_pointer_cast<Formal>($1)); }
        | FormalDecl T_COMMA FormalsList 
        { 
            shared_ptr<Formals> formals_ptr = dynamic_pointer_cast<Formals>($3);
            shared_ptr<Formal> singleFormal_ptr = dynamic_pointer_cast<Formal>($1);
            formals_ptr->push_front(singleFormal_ptr);
            $$ = formals_ptr;
        }

FormalDecl: Type T_ID 
            { 
                shared_ptr<ID> id_ptr = make_shared<ID>($2->text);
                shared_ptr<Type> retType_ptr = dynamic_pointer_cast<Type>($1);
                $$ = make_shared<Formal>(id_ptr, retType_ptr); 
            }

Statements: Statement { $$ = make_shared<Statements>(dynamic_pointer_cast<Statement>($1)); }
            | Statements Statement 
            { 
                shared_ptr<Statements> statements_ptr = dynamic_pointer_cast<Statements>($1);
                shared_ptr<Statement> singleStatement_ptr = dynamic_pointer_cast<Statement>($2);
                statements_ptr->push_back(singleStatement_ptr);
                $$ = statements_ptr;
            }

Statement: T_LBRACE Statements T_RBRACE { $$ = $2; }
            | Type T_ID T_SC 
            { 
                shared_ptr<ID> id_ptr = make_shared<ID>($2->text);
                shared_ptr<Type> type_ptr = dynamic_pointer_cast<Type>($1);
                $$ = make_shared<VarDecl>(id_ptr, type_ptr); 
            }
            | Type T_ID T_ASSIGN Exp T_SC 
            { 
                shared_ptr<ID> id_ptr = make_shared<ID>($2->text);
                shared_ptr<Type> type_ptr = dynamic_pointer_cast<Type>($1);
                shared_ptr<Exp> exp_ptr = dynamic_pointer_cast<Exp>($4);
                $$ = make_shared<VarDecl>(id_ptr, type_ptr, exp_ptr); 
            }
            | T_ID T_ASSIGN Exp T_SC 
            { 
                shared_ptr<ID> id_ptr = make_shared<ID>($1->text);
                shared_ptr<Exp> exp_ptr = dynamic_pointer_cast<Exp>($3);
                $$ = make_shared<Assign>(id_ptr, exp_ptr); 
            }
            | Call T_SC { $$ = $1; }
            | T_RETURN T_SC { $$ = make_shared<Return>(); }
            | T_RETURN Exp T_SC { $$ = make_shared<Return>(dynamic_pointer_cast<Exp>($2)); }
            | T_IF T_LPAREN Exp T_RPAREN Statement 
            { 
                shared_ptr<Exp> exp_ptr = dynamic_pointer_cast<Exp>($3);
                shared_ptr<Statement> statement_ptr = dynamic_pointer_cast<Statement>($5);
                $$ = make_shared<If>(exp_ptr, statement_ptr); 
            }
            | T_IF T_LPAREN Exp T_RPAREN Statement T_ELSE Statement 
            { 
                shared_ptr<Exp> exp_ptr = dynamic_pointer_cast<Exp>($3);
                shared_ptr<Statement> statement_ptr1 = dynamic_pointer_cast<Statement>($5);
                shared_ptr<Statement> statement_ptr2 = dynamic_pointer_cast<Statement>($7);
                $$ = make_shared<If>(exp_ptr, statement_ptr1, statement_ptr2); 
            }
            | T_WHILE T_LPAREN Exp T_RPAREN Statement 
            { 
                shared_ptr<Exp> exp_ptr = dynamic_pointer_cast<Exp>($3);
                shared_ptr<Statement> statement_ptr = dynamic_pointer_cast<Statement>($5);
                $$ = make_shared<While>(exp_ptr, statement_ptr); 
            }
            | T_BREAK T_SC { $$ = make_shared<Break>(); }
            | T_CONTINUE T_SC { $$ = make_shared<Continue>(); }
            | Exp { output::errorSyn(yylineno); }

Call:   T_ID T_LPAREN ExpList T_RPAREN 
            {
                shared_ptr<ID> id_ptr = make_shared<ID>($1->text);
                shared_ptr<ExpList> expList_ptr = dynamic_pointer_cast<ExpList>($3);
                $$ = make_shared<Call>(id_ptr, expList_ptr); 
            }
        | T_ID T_LPAREN T_RPAREN { $$ = make_shared<Call>(make_shared<ID>($1->text)); }

ExpList: Exp { $$ = make_shared<ExpList>(dynamic_pointer_cast<Exp>($1)); }
            | Exp T_COMMA ExpList 
            { 
                shared_ptr<ExpList> exp_list_ptr = dynamic_pointer_cast<ExpList>($3);
                shared_ptr<Exp> exp_ptr = dynamic_pointer_cast<Exp>($1);
                exp_list_ptr->push_front(exp_ptr);
                $$ = exp_list_ptr;
            }

Type:   T_INT { $$ = make_shared<Type>(BuiltInType::INT); }
            | T_BYTE { $$ = make_shared<Type>(BuiltInType::BYTE); }
            | T_BOOL { $$ = make_shared<Type>(BuiltInType::BOOL); }

Exp:    T_LPAREN Exp T_RPAREN { $$ = $2; }
            | Exp T_MUL_DIV Exp
            { 
                BinOpType binOp = whatBinOpRecieved($2->text);
                shared_ptr<Exp> left_exp = dynamic_pointer_cast<Exp>($1);
                shared_ptr<Exp> right_exp = dynamic_pointer_cast<Exp>($3);
                
                $$ = make_shared<BinOp>(binOp, left_exp, right_exp);
            }
            | Exp T_ADD_SUB Exp
            { 
                BinOpType binOp = whatBinOpRecieved($2->text);
                shared_ptr<Exp> left_exp = dynamic_pointer_cast<Exp>($1);
                shared_ptr<Exp> right_exp = dynamic_pointer_cast<Exp>($3);

                $$ = make_shared<BinOp>(binOp, left_exp, right_exp);
            }
            | T_ID { $$ = make_shared<ID>($1->text); }
            | Call { $$ = $1; }
            | T_NUM { $$ = make_shared<Num>($1->text); }
            | T_NUM_B { $$ = make_shared<NumB>($1->text); }
            | T_STRING { $$ = dynamic_pointer_cast<String>($1); }
            | T_TRUE { $$ = make_shared<Bool>(true); }
            | T_FALSE { $$ = make_shared<Bool>(false); }
            | T_NOT Exp { $$ = make_shared<Not>(dynamic_pointer_cast<Exp>($2)); }
            | Exp T_AND Exp 
            { 
                shared_ptr<Exp> exp_ptr1 = dynamic_pointer_cast<Exp>($1);
                shared_ptr<Exp> exp_ptr2 = dynamic_pointer_cast<Exp>($3);
                $$ = make_shared<And>(exp_ptr1, exp_ptr2); 
            }
            | Exp T_OR Exp 
            { 
                shared_ptr<Exp> exp_ptr1 = dynamic_pointer_cast<Exp>($1);
                shared_ptr<Exp> exp_ptr2 = dynamic_pointer_cast<Exp>($3);
                $$ = make_shared<Or>(exp_ptr1, exp_ptr2); 
            }
            | Exp T_RELOP Exp
            { 
                auto left_exp = dynamic_pointer_cast<Exp>($1);
                auto right_exp = dynamic_pointer_cast<Exp>($3);
                
                RelOpType relop = whatRelOpRecieved($2->text);
                $$ = make_shared<RelOp>(relop, left_exp, right_exp);
            }
            | T_LPAREN Type T_RPAREN Exp 
            { 
                shared_ptr<Type> type_ptr = dynamic_pointer_cast<Type>($2);
                shared_ptr<Exp> exp_ptr = dynamic_pointer_cast<Exp>($4);
                $$ = make_shared<Cast>(exp_ptr, type_ptr); 
            }

%%

// TODO: Place any additional code here
void yyerror(const char* msg) {
    output::errorSyn(yylineno);
}

static BinOpType whatBinOpRecieved(string received)
{
    //cout << received << endl;
    BinOpType retval = BIN_ERROR;
    if("+" == received){
        retval = ADD;
    } else if("-" == received){
        retval = SUB;
    } else if("*" == received){
        retval = MUL;
    } else if("/" == received){
        retval = DIV;
    } else {
        yyerror("Unknown binop operator");
    }

    return retval;
}

static RelOpType whatRelOpRecieved(string received)
{
    //cout << received << endl;
    RelOpType retval = REL_ERROR;
    if("==" == received){
        retval = EQ;
    } else if("!=" == received){
        retval = NE;
    } else if("<" == received){
        retval = LT;
    } else if(">" == received){
        retval = GT;
    } else if("<=" == received){
        retval = LE;
    } else if(">=" == received){
        retval = GE;
    } else {
        yyerror("Unknown relop operator");
    }

    return retval;
}

static BuiltInType whatTypeReceived(string received)
{
    BuiltInType retval = TYPE_ERROR;
    if("int" == received){
        retval = INT;
    } else if("bool" == received){
        retval = BOOL;
    } else if("byte" == received){
        retval = BYTE;
    } else if("string" == received){
        retval = STRING;
    } else if("void" == received){
        retval = VOID;
    } else {
        yyerror("Unknown Type operator");
    }

    return retval;
}