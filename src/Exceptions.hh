
#pragma once

#include <stdexcept>
#include <string>

class CadabraException : public std::logic_error {
	public:
		CadabraException(std::string);
};

class ConsistencyException : public CadabraException {
	public:
		ConsistencyException(std::string);
};

class ParseException : public CadabraException {
	public:
		ParseException(std::string);
};

class InterruptionException : public CadabraException {
	public:
		InterruptionException(std::string);
};

