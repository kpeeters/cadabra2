
#include "Exceptions.hh"

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
