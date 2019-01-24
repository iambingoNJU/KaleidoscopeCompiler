
#include "pch.h"
#include <iostream>

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
