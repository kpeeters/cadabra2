
#include "Kernel.hh"
#include "properties/Indices.hh"
#include "properties/Integer.hh"
#include "properties/Coordinate.hh"
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

bool Indices::parse(Kernel& kernel, std::shared_ptr<Ex> ex, keyval_t& keyvals)
	{
	keyval_t::const_iterator ki=keyvals.begin();
	while(ki!=keyvals.end()) {
		if(ki->first=="name") {
			if(*ki->second->multiplier!=1) {
				throw std::logic_error("Indices: use quotes to label names when they start with a number.");
				}
			set_name=*ki->second->name;
			if(set_name.size()>0) {
				if(set_name[0]=='\"' && set_name[set_name.size()-1]=='\"')
					set_name=set_name.substr(1,set_name.size()-2);
				}
			}
		else if(ki->first=="parent") {
			parent_name=*ki->second->name;
			if(parent_name.size()>0) {
				if(parent_name[0]=='\"' && parent_name[set_name.size()-1]=='\"')
					parent_name=parent_name.substr(1,parent_name.size()-2);
				}
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

			// If all values are indices, add an `Integer' property for the object,
			// listing these integers.
			bool is_number=true;
			for(auto& val: values)
				if(!val.begin()->is_integer()) {
					is_number=false;
					break;
					}
			if(is_number) {
				Ex from(values[0]), to(values[values.size()-1]);
//				std::cerr << "Injecting Integer property" << std::endl;
				// FIXME: this assumes that the values are in a continuous range (as Integer cannot
				// represent anything else at this stage).
				kernel.inject_property(new Integer(), ex,
											  kernel.ex_from_string(std::to_string(values[0].to_integer())+".."
																			+std::to_string(values[values.size()-1].to_integer())
																			));
				}
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

void Indices::validate(const Kernel& k, const Ex& ex) const
	{
	do_list(ex, ex.begin(), [&k](Ex::iterator i) {
									if(k.properties.get<Coordinate>(i))
										throw ConsistencyException("Object already has a Coordinate property attached to it.");
									return true;
									}
		);
	}


void Indices::collect_index_values(Ex::iterator ind_values)
	{
	if(*ind_values->name=="\\sequence") {
		// We have been given a sequence, not a list.
		// FIXME: guard against errors or large ranges.
		auto ch = Ex::begin(ind_values);
		size_t start = to_long(*ch->multiplier);
		++ch;
		size_t end   = to_long(*ch->multiplier);
		if(end<start)
			throw ArgumentException("Index range must be increasing.");
		if(end-start > 100)
			throw ArgumentException("Number of index values larger than 100, probably a typo.");
			
		for(size_t i=start; i<=end; ++i) {
			values.push_back(Ex(i));
			}
		return;
		}
	
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

