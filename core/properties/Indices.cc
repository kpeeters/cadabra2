
#include "properties/Indices.hh"
#include "Exceptions.hh"
#include "Functional.hh"

using namespace cadabra;

Indices::Indices()
	: position_type(free)
	{
	}

std::string Indices::name() const
	{
	return "Indices";
	}

property::match_t Indices::equals(const property *other) const
	{
	const Indices *cast_other = dynamic_cast<const Indices *>(other);
	if(cast_other) {
		 if(set_name == cast_other->set_name) {
			  if(parent_name == cast_other->parent_name && position_type == cast_other->position_type)
					return exact_match;
			  else
					return id_match;
			  }
		 return no_match;
		 }
	return property::equals(other);
	}

bool Indices::parse(const Kernel&, keyval_t& keyvals)
	{
	keyval_t::const_iterator ki=keyvals.begin();
	while(ki!=keyvals.end()) {
		if(ki->first=="name") {
			if(*ki->second->multiplier!=1) {
				throw std::logic_error("Indices: use quotes to label names when they start with a number.");
				}
			set_name=*ki->second->name;
			}
		else if(ki->first=="parent") {
			parent_name=*ki->second->name;
			}
		else if(ki->first=="position") {
			if(*ki->second->name=="free")
				position_type=free;
			else if(*ki->second->name=="fixed")
				position_type=fixed;
			else if(*ki->second->name=="independent")
				position_type=independent;
			else throw ConsistencyException("Position type should be fixed, free or independent.");
			}
		else if(ki->first=="values") { 
			//std::cerr << "got values keyword " << *(ki->second->name) << std::endl;
			collect_index_values(ki->second);
			}
		else throw ConsistencyException("Property 'Indices' does not accept key '"+ki->first+"'.");
		++ki;
		}

	return true;
	}

void Indices::latex(std::ostream& str) const
	{
	str << "Indices";
	switch(position_type) {
		case free:
			str << "(position=free)";
			break;
		case fixed:
			str << "(position=fixed)";
			break;
		case independent:
			str << "(position=independent)";
			break;
		}
	}

void Indices::collect_index_values(Ex::iterator ind_values)
	{
	Ex tmp;
	cadabra::do_list(tmp, ind_values, [&](Ex::iterator ind) {
			values.push_back(Ex(ind));
//			auto name=ind_values.begin(ind);
//			sibling_iterator vals=name;
//			++vals;
//			index_values[indprop]=cadabra::make_list(Ex(vals));
			return true;
			});
	}

