
#pragma once

#include "CoreProps.hh"

class Matrix : public ImplicitIndex, virtual public property {
	public: 
		virtual ~Matrix() {};
		virtual std::string name() const;
};
