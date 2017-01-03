
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

RuntimeException::RuntimeException(std::string s)
	: CadabraException(s)
	{
	}

InternalError::InternalError(std::string s)
	: CadabraException(s)
	{
	}

std::string InternalError::py_what() const
	{
	std::cerr << "Internal error: " << what() << "Please report a bug to info@cadabra.science." << std::endl;
	return what();
	}

NotYetImplemented::NotYetImplemented(std::string s)
	: CadabraException(s)
	{
	}

std::string NotYetImplemented::py_what() const
	{
	std::cerr << "Not yet implemented: " << what() << std::endl;
	return what();
	}

