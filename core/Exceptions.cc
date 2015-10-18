
#include "Exceptions.hh"
#include <iostream>

CadabraException::CadabraException(std::string s)
	: std::logic_error(s)
	{
	}

ParseException::ParseException(std::string s) 
	: CadabraException(s) 
	{
	}

ConsistencyException::ConsistencyException(std::string s) 
	: CadabraException(s) 
	{
	}

InterruptionException::InterruptionException(std::string s)
	: CadabraException(s)
	{
	}

ArgumentException::ArgumentException(std::string s)
	: CadabraException(s)
	{
	}

std::string ArgumentException::py_what() const
	{
	std::cerr << what() << std::endl;
	return what();
	}

NonScalarException::NonScalarException(std::string s)
	: CadabraException(s)
	{
	}

std::string NonScalarException::py_what() const
	{
	std::cerr << what() << std::endl;
	return what();
	}
