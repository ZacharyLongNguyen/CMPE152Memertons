#include <iostream>
#include <string>

#include "Pass2Visitor.h"
#include "wci/intermediate/SymTabStack.h"
#include "wci/intermediate/SymTabEntry.h"
#include "wci/intermediate/TypeSpec.h"
#include "wci/intermediate/symtabimpl/Predefined.h"

int labelNum = 0;
int label = 0;
using namespace wci;
using namespace wci::intermediate;
using namespace wci::intermediate::symtabimpl;

const bool DEBUG_2 = false;

Pass2Visitor::Pass2Visitor()
    : program_name(""), j_file(nullptr)
{
}

Pass2Visitor::~Pass2Visitor() {}

ostream& Pass2Visitor::get_assembly_file() { return j_file; }

antlrcpp::Any Pass2Visitor::visitProgram(Pcl1Parser::ProgramContext *ctx)
{
    auto value = visitChildren(ctx);

    j_file.close();

    if (DEBUG_2) cout << "=== Pass 2: visitProgram: closed " + j_file_name << endl;
    return value;
}

antlrcpp::Any Pass2Visitor::visitHeader(Pcl1Parser::HeaderContext *ctx)
{
    if (DEBUG_2) cout << "=== Pass 2: visitHeader: " + ctx->getText() << endl;

    program_name = ctx->IDENTIFIER()->toString();

    // Create the assembly output file.
    j_file_name = program_name + ".j";
    j_file.open(j_file_name);
    if (j_file.fail())
    {
        cout << "***** Cannot open assembly file." << endl;
        exit(-99);
    }

    if (DEBUG_2) cout << "=== Pass 2: visitHeader: created " + j_file_name << endl;

    // Emit the program header.
    j_file << ".class public " << program_name << endl;
    j_file << ".super java/lang/Object" << endl;

    return visitChildren(ctx);
}

antlrcpp::Any Pass2Visitor::visitMainBlock(Pcl1Parser::MainBlockContext *ctx)
{
    if (DEBUG_2) cout << "=== Pass 2: visitMainBlock" << endl;

    // Emit the RunTimer and PascalTextIn fields.
    j_file << endl;
    j_file << ".field private static _runTimer LRunTimer;" << endl;
    j_file << ".field private static _standardIn LPascalTextIn;" << endl;

    // Emit declarations.
    visit(ctx->block()->declarations());

    // Emit the class constructor.
    j_file << endl;
    j_file << ".method public <init>()V" << endl;
    j_file << endl;
    j_file << "\taload_0" << endl;
    j_file << "\tinvokenonvirtual java/lang/Object/<init>()V" << endl;
    j_file << "\treturn" << endl;
    j_file << endl;
    j_file << ".limit locals 1" << endl;
    j_file << ".limit stack 1" << endl;
    j_file << ".end method" << endl;

    // Emit the main program header and prologue.
    j_file << endl;
    j_file << ".method public static main([Ljava/lang/String;)V" << endl;
    j_file << endl;
    j_file << "\tnew RunTimer" << endl;
    j_file << "\tdup" << endl;
    j_file << "\tinvokenonvirtual RunTimer/<init>()V" << endl;
    j_file << "\tputstatic\t" << program_name
           << "/_runTimer LRunTimer;" << endl;
    j_file << "\tnew PascalTextIn" << endl;
    j_file << "\tdup" << endl;
    j_file << "\tinvokenonvirtual PascalTextIn/<init>()V" << endl;
    j_file << "\tputstatic\t" + program_name
           << "/_standardIn LPascalTextIn;" << endl;

    // Emit code for the main program's compound statement.
    visit(ctx->block()->compoundStmt());

    // Emit the main program epilogue.
    j_file << endl;
    j_file << "\tgetstatic     " << program_name
               << "/_runTimer LRunTimer;" << endl;
    j_file << "\tinvokevirtual RunTimer.printElapsedTime()V" << endl;
    j_file << endl;
    j_file << "\treturn" << endl;
    j_file << endl;
    j_file << ".limit locals 16" << endl;
    j_file << ".limit stack 16" << endl;
    j_file << ".end method" << endl;

    return nullptr;
}

antlrcpp::Any Pass2Visitor::visitDeclarations(Pcl1Parser::DeclarationsContext *ctx)
{
    if (DEBUG_2) cout << "=== Pass 2: visitDeclarations: " << ctx->getText() << endl;

    return visitChildren(ctx);
}

antlrcpp::Any Pass2Visitor::visitDecl(Pcl1Parser::DeclContext *ctx)
{
    if (DEBUG_2) cout << "=== Pass 2: visitDecl: " + ctx->getText() << endl;

    j_file << "\n; " << ctx->getText() << "\n" << endl;
    return visitChildren(ctx);
}


antlrcpp::Any Pass2Visitor::visitVarId(Pcl1Parser::VarIdContext *ctx)
{
    if (DEBUG_2) cout << "=== Pass 2: visitVarId: " + ctx->getText() << endl;

    string variable_name = ctx->IDENTIFIER()->toString();
    TypeSpec *type = ctx->type;

    // Emit a field declaration.
    string type_indicator = (type == Predefined::integer_type) ? "I"
                          : (type == Predefined::real_type)    ? "F"
                          :                                      "?";
    j_file << ".field private static "
           << variable_name << " " << type_indicator << endl;

    return visitChildren(ctx);
}

antlrcpp::Any Pass2Visitor::visitStmt(Pcl1Parser::StmtContext *ctx)
{
    if (DEBUG_2) cout << "=== Pass 2: visitStmt" << endl;

    j_file << endl << "; " + ctx->getText() << endl << endl;

    return visitChildren(ctx);
}

antlrcpp::Any Pass2Visitor::visitAssignmentStmt(Pcl1Parser::AssignmentStmtContext *ctx)
{
    if (DEBUG_2) cout << "=== Pass 2: visitAssignmentStmt" << endl;

    auto value = visit(ctx->expr());

    string type_indicator =
                  (ctx->expr()->type == Predefined::integer_type) ? "I"
                : (ctx->expr()->type == Predefined::real_type)    ? "F"
                :                                                   "?";

    // Emit a field put instruction.
    j_file << "\tputstatic\t" << program_name
           << "/" << ctx->variable()->IDENTIFIER()->toString()
           << " " << type_indicator << endl;

    return value;
}

antlrcpp::Any Pass2Visitor::visitAddSubExpr(Pcl1Parser::AddSubExprContext *ctx)
{
    if (DEBUG_2) cout << "=== Pass 2: visitAddSubExpr" << endl;

    auto value = visitChildren(ctx);

    TypeSpec *type1 = ctx->expr(0)->type;
    TypeSpec *type2 = ctx->expr(1)->type;

    bool integer_mode =    (type1 == Predefined::integer_type)
                        && (type2 == Predefined::integer_type);
    bool real_mode    =    (type1 == Predefined::real_type)
                        && (type2 == Predefined::real_type);

    string op = ctx->addSubOp()->getText();
    string opcode;

    if (op == "+")
    {
        opcode = integer_mode ? "iadd"
               : real_mode    ? "fadd"
               :                "????";
    }
    else
    {
        opcode = integer_mode ? "isub"
               : real_mode    ? "fsub"
               :                "????";
    }

    // Emit an add or subtract instruction.
    j_file << "\t" << opcode << endl;

    return value;
}

antlrcpp::Any Pass2Visitor::visitMulDivExpr(Pcl1Parser::MulDivExprContext *ctx)
{
    if (DEBUG_2) cout << "=== Pass 2: visitMulDivExpr" << endl;

    auto value = visitChildren(ctx);

    TypeSpec *type1 = ctx->expr(0)->type;
    TypeSpec *type2 = ctx->expr(1)->type;

    bool integer_mode =    (type1 == Predefined::integer_type)
                        && (type2 == Predefined::integer_type);
    bool real_mode    =    (type1 == Predefined::real_type)
                        && (type2 == Predefined::real_type);

    string op = ctx->mulDivOp()->getText();
    string opcode;

    if (op == "*")
    {
        opcode = integer_mode ? "imul"
               : real_mode    ? "fmul"
               :                "????";
    }
    else
    {
        opcode = integer_mode ? "idpv"
               : real_mode    ? "fdiv"
               :                "????";
    }

    // Emit a multiply or divide instruction.
    j_file << "\t" << opcode << endl;

    return value;
}

antlrcpp::Any Pass2Visitor::visitVariableExpr(Pcl1Parser::VariableExprContext *ctx)
{
    if (DEBUG_2) cout << "=== Pass 2: visitVariableExpr" << endl;

    string variable_name = ctx->variable()->IDENTIFIER()->toString();
    TypeSpec *type = ctx->type;

    string type_indicator = (type == Predefined::integer_type) ? "I"
                          : (type == Predefined::real_type)    ? "F"
                          :                                      "?";

    // Emit a field get instruction.
    j_file << "\tgetstatic\t" << program_name
           << "/" << variable_name << " " << type_indicator << endl;

    return visitChildren(ctx);
}

antlrcpp::Any Pass2Visitor::visitSignedNumber(Pcl1Parser::SignedNumberContext *ctx)
{
    if (DEBUG_2) cout << "=== Pass 2: visitSignedNumber" << endl;

    auto value = visitChildren(ctx);
    TypeSpec *type = ctx->number()->type;

    if (ctx->sign()->children[0] == ctx->sign()->SUB_OP())
    {
        string opcode = (type == Predefined::integer_type) ? "ineg"
                      : (type == Predefined::real_type)    ? "fneg"
                      :                                      "?neg";

        // Emit a negate instruction.
        j_file << "\t" << opcode << endl;
    }

    return value;
}

antlrcpp::Any Pass2Visitor::visitIntegerConst(Pcl1Parser::IntegerConstContext *ctx)
{
    if (DEBUG_2) cout << "=== Pass 2: visitIntegerConst" << endl;

    // Emit a load constant instruction.
    j_file << "\tldc\t" << ctx->getText() << endl;

    return visitChildren(ctx);
}

antlrcpp::Any Pass2Visitor::visitFloatConst(Pcl1Parser::FloatConstContext *ctx)
{
    if (DEBUG_2) cout << "=== Pass 2: visitFloatConst" << endl;

    // Emit a load constant instruction.
    j_file << "\tldc\t" << ctx->getText() << endl;

    return visitChildren(ctx);
}

/*antlrcpp::Any Pass2Visitor::visitLoop_num_stmt(Pcl1Parser::Loop_num_stmtContext *ctx)
{
	if (DEBUG_2) cout << "=== Pass 2: visitLoop_num_stmt" << endl;


}*/
antlrcpp::Any Pass2Visitor::visitLoop_until_stmt(Pcl1Parser::Loop_until_stmtContext *ctx)
{
	if (DEBUG_2) cout << "=== Pass 2: visitLoop_until_stmt" << endl;

	int loop_start = labelNum++;
	j_file << "Label_" << loop_start << ":" << endl;
	visit(ctx->stmtList());
	label=loop_start;
	visit(ctx->expr());
	return NULL;

}

antlrcpp::Any Pass2Visitor::visitIf_stmt(Pcl1Parser::If_stmtContext *ctx)
{
	if (DEBUG_2) cout << "=== Pass 2: visitIf_stmt" << endl;

	int original_label = labelNum++;
	int true_label = labelNum++;
	int last_label = labelNum++;
	int statement_size = ctx->stmtList().size();
	bool has_else = false;

	if (statement_size > 1)
		has_else=true;

	j_file << "Label_" << original_label << ":" << endl;
	label = true_label;
	visit(ctx->expr());

	if(has_else)
	{
		visitChildren(ctx->stmtList(statement_size - 1));
		j_file << "\tgoto " << "Label_" << last_label << endl;
	}
	else
	{
		j_file << "Label_" << true_label << ":" << endl;
		visitChildren(ctx->stmtList(statement_size - 1));
		return NULL;
	}

	j_file << "Label_" << true_label << ":" << endl;
	visitChildren(ctx->stmtList(statement_size - 2));
	j_file << "Label_" << last_label << ":" << endl;
	return NULL;
}

antlrcpp::Any Pass2Visitor::visitRelOpExpr(Pcl1Parser::RelOpExprContext *ctx)
{
	if (DEBUG_2) cout << "=== Pass 2: visitRelOpExpr" << endl;
	auto value = visitChildren(ctx);

	TypeSpec *type1 = ctx->expr(0)->type;
	TypeSpec *type2 = ctx->expr(1)->type;

	string op = ctx->relOp()->getText();
	string jas_op;

	//Character are loaded as integers
	bool integer_mode =    ((type1 == Predefined::integer_type)
						&& (type2 == Predefined::integer_type)) ||
						   ((type1 == Predefined::real_type)
						&& (type2 == Predefined::real_type));
	if (op == ">")
	   jas_op = integer_mode ? "if_icmpgt":"????";
	else if (op == ">=")
	   jas_op = integer_mode ? "if_icmpge":"????";
	else if (op == "<")
		jas_op = integer_mode ? "if_icmplt":"????";
	else if (op == "<=")
		jas_op = integer_mode ? "if_icmple":"????";
	else if (op == "!=")
		jas_op = integer_mode ? "if_icmpne":"????";
	else
		jas_op = integer_mode ? "if_icmpeq" : "????";

	j_file << "\t" << jas_op << " Label_" << label << endl;
	return NULL;
}