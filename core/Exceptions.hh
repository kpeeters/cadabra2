
#pragma once

#include <stdexcept>
#include <string>

class CadabraException : public std::logic_error {
	public:
		CadabraException(std::string);
};

// Exception thrown when an inconsist expression or argument is encountered.

class ConsistencyException : public CadabraException {
	public:
		ConsistencyException(std::string);
};

// Exception thrown when the parser cannot parse an input expression.

class ParseException : public CadabraException {
	public:
		ParseException(std::string);
};

// Exception thrown when an algorithm determines that it was interrupted.

class InterruptionException : public CadabraException {
	public:
		InterruptionException(std::string="");
};

/// Exception thrown when arguments to an algorithm or property are not correct.

class ArgumentException : public CadabraException {
	public:
		ArgumentException(std::string="");

		std::string py_what() const;
};

/// Exception thrown when something requires that an expression is a pure scalar
/// (i.e. no free indices and no dummy indices), but isn't.

class NonScalarException : public CadabraException {
	public:
		NonScalarException(std::string="");

		std::string py_what() const;
};

