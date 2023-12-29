#pragma once

#include <string>

class LaTeXString {
	public:
		LaTeXString(const std::string& );
		
		std::string latex() const;
	private:
		std::string content;
};
