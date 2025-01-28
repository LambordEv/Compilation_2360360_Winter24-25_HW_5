#include "output.hpp"
#include "nodes.hpp"
#include "symbol.hpp"
#include "scope.hpp"
#include "symbolTable.hpp"
#include "semanticAnalyzer.hpp"
using namespace output;

// Extern from the bison-generated parser
extern int yyparse();

extern std::shared_ptr<ast::Node> program;

int main() {
    // Parse the input. The result is stored in the global variable `program`
    yyparse();

    // Print the AST using the PrintVisitor
    SemanticAnalyzer analyzer;
    program->accept(analyzer);

    analyzer.printResults();
}
