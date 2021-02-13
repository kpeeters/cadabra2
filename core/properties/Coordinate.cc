
#include "properties/Coordinate.hh"
#include "properties/Indices.hh"
#include "Exceptions.hh"
#include "Kernel.hh"
#include "Functional.hh"

using namespace cadabra;

std::string Coordinate::name() const
	{
	return "Coordinate";
	}

void Coordinate::validate(const Kernel& k, const Ex& ex) const
	{
	do_list(ex, ex.begin(), [&k](Ex::iterator i) {
									Ex tmp(i);
									tmp.begin()->fl.parent_rel=str_node::parent_rel_t::p_sub;
									if(k.properties.get<Indices>(tmp.begin()))
										throw ConsistencyException("Object already has an Indices property attached to it.");
									tmp.begin()->fl.parent_rel=str_node::parent_rel_t::p_super;
									if(k.properties.get<Indices>(tmp.begin()))
										throw ConsistencyException("Object already has an Indices property attached to it.");
									return true;
									}
		);
	}
