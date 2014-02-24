
#include "properties/Indices.hh"
#include "Exceptions.hh"

Indices::Indices()
	: position_type(free)
	{
	}

std::string Indices::name() const
	{
	return "Indices";
	}

property_base::match_t Indices::equals(const property_base *other) const
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
	return property_base::equals(other);
	}

bool Indices::parse(exptree& tr, exptree::iterator pat, exptree::iterator prop, keyval_t& keyvals)
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
			values=*ki->second;
			if(*values.begin()->name!="\\comma") 
				throw ConsistencyException("Key 'values' of property 'Indices' needs a list as value.");
			}
		else throw ConsistencyException("Property 'Indices' does not accept key '"+ki->first+"'.");
		++ki;
		}

	return true;
	}
