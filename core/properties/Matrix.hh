
#pragma once

#include "properties/ImplicitIndex.hh"

class Matrix : public ImplicitIndex, virtual public property {
	public: 
		virtual ~Matrix() {};
		virtual std::string name() const;
};
