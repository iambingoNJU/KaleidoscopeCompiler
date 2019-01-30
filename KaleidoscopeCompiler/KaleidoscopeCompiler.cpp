
#include "pch.h"
#include <iostream>
#include <vector>
#include <map>

enum Token
{
	tok_eof = -1,

	tok_def = -2,
	tok_extern = -3,

	tok_identifier = -4,
	tok_number = -5,
};

static std::string IdentifierStr;
static double NumVal;

static int gettok() {
	static int LastChar = ' ';

	while (isspace(LastChar))
		LastChar = getchar();

	if (isalpha(LastChar)) { // identifier: [a-zA-Z][a-zA-Z0-9]*
		IdentifierStr = LastChar;
		while (isalnum(LastChar = getchar()))
			IdentifierStr += LastChar;

		if (IdentifierStr == "def")
			return tok_def;
		if (IdentifierStr == "extern")
			return tok_extern;
		return tok_identifier;
	}

	if (isdigit(LastChar) || LastChar == '.') { // Number: [0-9.]+
		std::string NumStr;
		do {
			NumStr += LastChar;
			LastChar = getchar();
		} while (isdigit(LastChar) || LastChar == '.');

		NumVal = strtod(NumStr.c_str(), 0);
		return tok_number;
	}

	if (LastChar == '#') { // comment until end of line
		do {
			LastChar = getchar();
		} while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

		if (LastChar != EOF)
			return gettok();
	}

	// check for end of file
	if (LastChar == EOF)
		return tok_eof;

	// Otherwise, just return the character as its ascii value.
	int ThisChar = LastChar;
	LastChar = getchar();
	return ThisChar;
}

/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
	virtual ~ExprAST() {}
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
	double Val;

public:
	NumberExprAST(double val) : Val(val) {}
};

/// VariableExprAST - Expression class for referrencing a variable, like "a".
class VariableExprAST : public ExprAST {
	std::string Name;

public:
	VariableExprAST(std::string &name) : Name(name) {}
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
	char Op;
	std::unique_ptr<ExprAST> LHS, RHS;

public:
	BinaryExprAST(char op, std::unique_ptr<ExprAST> lhs,
		std::unique_ptr<ExprAST> rhs)
		: Op(op), LHS(std::move(lhs)), RHS(std::move(rhs)) {}
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
	std::string Callee;
	std::vector<std::unique_ptr<ExprAST>> Args;

public:
	CallExprAST(const std::string &callee, std::vector<std::unique_ptr<ExprAST>> args)
		: Callee(callee), Args(std::move(args)) {}
};


/// PrototypeAST - This class represents the "prototype" for a function,
/// which capture its name, and its argument names (thus implictly the number
/// of arguments the function takes).
class PrototypeAST : public ExprAST {
	std::string Name;
	std::vector<std::string> Args;

public:
	PrototypeAST(const std::string &name, std::vector<std::string> args)
		: Name(name), Args(std::move(args)) {}
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST : public ExprAST {
	std::unique_ptr<PrototypeAST> Proto;
	std::unique_ptr<ExprAST> Body;

public:
	FunctionAST(std::unique_ptr<PrototypeAST> proto, std::unique_ptr<ExprAST> body)
		: Proto(std::move(proto)), Body(std::move(body)) {}
};

static int CurTok;
static int getNextToken() {
	return CurTok = gettok();
}

std::unique_ptr<ExprAST> LogError(const char *Str) {
	fprintf(stderr, "LogError: %s\n", Str);
	return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
	LogError(Str);
	return nullptr;
}

static std::unique_ptr<ExprAST> ParseExpression();
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS);

/// numberexpr ::= number
static std::unique_ptr<ExprAST> ParseNumberExpr() {
	auto Result = std::make_unique<NumberExprAST>(NumVal);
	getNextToken();
	return std::move(Result);
}

/// parenexpr ::= '(' expression ')'
static std::unique_ptr<ExprAST> ParseParenExpr() {
	getNextToken(); // eat '('.
	auto V = ParseExpression();
	if (!V)
		return nullptr;

	if (CurTok != ')')
		return LogError("expected ')'");
	getNextToken(); // eat ')'.
	return V;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
	std::string IdName = IdentifierStr;

	getNextToken(); // eat identifier.

	if (CurTok != '(') // Simple variable ref.
		return std::make_unique<VariableExprAST>(IdName);

	// Call.
	getNextToken(); // eat '('
	std::vector<std::unique_ptr<ExprAST>> Args;
	if (CurTok != ')') {
		while (1) {
			if (auto Arg = ParseExpression())
				Args.push_back(std::move(Arg));
			else
				return nullptr;

			if (CurTok == ')')
				break;

			if (CurTok != ',')
				LogError("Expected ')' or ',' in argument list");
			getNextToken(); // eat ','
		}
	}

	getNextToken(); // eat ')'

	return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
static std::unique_ptr<ExprAST> ParsePrimary() {
	switch (CurTok) {
	default:
		LogError("unknown token when expecting an expression");
	case tok_identifier:
		return ParseIdentifierExpr();
	case tok_number:
		return ParseNumberExpr();
	case '(':
		return ParseParenExpr();
	}
}

/// BinopPrecedence - This holds the precedence for each binary operator that is defined
static std::map<char, int> BinopPrecedence = { 
	{ '<', 10 },
	{ '+', 20 },
	{ '-', 20 },
	{ '*', 40 },
};

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
static int GetTokPrecedence() {
	if (!isascii(CurTok))
		return -1;

	// Make sure it's a declared binop.
	int TokPrec = BinopPrecedence[CurTok];
	if (TokPrec <= 0) return -1;
	return TokPrec;
}

/// expression
///   ::= primary binoprhs
///
static std::unique_ptr<ExprAST> ParseExpression() {
	auto LHS = ParsePrimary();
	if (!LHS)
		return nullptr;

	return ParseBinOpRHS(0, std::move(LHS));
}

/// binoprhs
///   ::= ('+' primary)*
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS) {
	// If this is a binop, find its precedence.
	while (true) {
		int TokPrec = GetTokPrecedence();

		// If this is a binop that binds at least as tightly as the current binop,
		// consume it, otherwise we are done.
		if (TokPrec < ExprPrec)
			return LHS;

		// Okay, we know this is a binop.
		int BinOp = CurTok;
		getNextToken(); // eat binop

		// Parse the primary expression after the binary operator.
		auto RHS = ParsePrimary();
		if (!RHS)
			return nullptr;

		// If BinOp binds less tightly with RHS than the operator after RHS, let
		// the pending operator take RHS as its LHS.
		int NextPrec = GetTokPrecedence();
		if (TokPrec < NextPrec) {
			RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
			if (!RHS)
				return nullptr;
		}

		// Merge LHS/RHS
		LHS = std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
	}
}

/// prototype
///   ::= id '(' id* ')'
static std::unique_ptr<PrototypeAST> ParsePrototype() {
	if (CurTok != tok_identifier)
		return LogErrorP("Expected function name in prototype");

	std::string FnName = IdentifierStr;
	getNextToken();

	if (CurTok != '(')
		return LogErrorP("Expected '(' in prototype");

	// Read the list of argument names
	std::vector<std::string> ArgNames;
	while (getNextToken() == tok_identifier)
		ArgNames.push_back(IdentifierStr);
	if (CurTok != ')')
		return LogErrorP("Expected ')' in prototype");

	// success
	getNextToken();

	return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

/// definition ::= 'def' prototype expression
static std::unique_ptr<FunctionAST> ParseDefinition() {
	getNextToken(); // eat def
	auto Proto = ParsePrototype();
	if (!Proto) return nullptr;

	if (auto E = ParseExpression())
		return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
	return nullptr;
}

/// external ::= 'extern' prototype
static std::unique_ptr<PrototypeAST> ParseExtern() {
	getNextToken(); // eat extern
	return ParsePrototype();
}

/// toplevelexpr ::= expression
static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
	if (auto E = ParseExpression()) {
		// Make an anonymous proto.
		auto Proto = std::make_unique<PrototypeAST>("", std::vector<std::string>());
		return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
	}
	return nullptr;
}

static void HandleDefinition() {
	if (ParseDefinition()) {
		fprintf(stderr, "Parsed a function definition.\n");
	} else {
		// skip token for error recovery.
		getNextToken();
	}
}

static void HandleExtern() {
	if (ParseExtern()) {
		fprintf(stderr, "Parsed an extern.\n");
	} else {
		getNextToken();
	}
}

static void HandleTopLevelExpression() {
	if (ParseTopLevelExpr()) {
		fprintf(stderr, "Parsed a top-level expr.\n");
	} else {
		getNextToken();
	}
}

/// top ::= definition | external | expression | ';'
static void MainLoop() {
	while (1) {
		fprintf(stderr, "ready> ");
		switch (CurTok) {
		case tok_eof:
			return;
		case ';': // ignore top-level semicolons
			getNextToken();
			break;
		case tok_def:
			HandleDefinition();
			break;
		case tok_extern:
			HandleExtern();
			break;
		default:
			HandleTopLevelExpression();
			break;
		}
	}
}

int main()
{
	/*
	int token;
	while ((token = gettok()) != tok_eof) {
		std::cout << "Token: ";
		switch (token)
		{
			case tok_identifier:
				std::cout << IdentifierStr.c_str();
				break;
			case tok_number:
				std::cout << NumVal;
				break;
			case tok_extern:
				std::cout << "extern";
				break;
			case tok_def:
				std::cout << "def";
				break;
			default:
				std::cout << (char)token;
				break;
		}
		std::cout << std::endl;
	}
	*/

	fprintf(stderr, "ready> ");
	getNextToken();

	MainLoop();

	return 0;
}
