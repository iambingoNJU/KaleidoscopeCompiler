
#include "pch.h"
#include <iostream>
#include <vector>

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

int main()
{
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
}
