
#include "properties/Depends.hh"
#include "properties/Indices.hh"
#include "properties/Coordinate.hh"
#include "properties/Derivative.hh"
#include "properties/Accent.hh"
#include "Exceptions.hh"
#include "Kernel.hh"

std::string Depends::name() const
	{
	return "Depends";
	}

bool Depends::parse(const Kernel& kernel, keyval_t& kv)
	{
	const Properties& pr=kernel.properties;

	keyval_t::const_iterator it=kv.begin();
	dependencies_.set_head(str_node("\\comma"));
	Ex::iterator comma = dependencies_.begin();
//	Ex::iterator comma = dependencies_.append_child(dependencies_.begin(), str_node("\\comma"));
	while(it!=kv.end()) {
		if(it->first=="dependants") {
			const Indices    *dum=pr.get<Indices>(it->second, true);
			const Coordinate *crd=pr.get<Coordinate>(it->second);
			const Derivative *der=pr.get<Derivative>(it->second);
			const Accent     *acc=pr.get<Accent>(it->second);
			if(dum==0 && crd==0 && der==0 && acc==0) {
				throw ArgumentException(std::string("Depends: ")+*it->second->name
												+" lacks property Coordinate, Derivative, Accent or Indices.");
				}
//			std::cout << "adding " << *it->second->name << " to deps list" << std::endl;
			dependencies_.append_child(comma, it->second);
			}
		++it;
		}

	return true;
	}

Ex Depends::dependencies(const Kernel&, Ex::iterator) const
	{
	return dependencies_;
	}
