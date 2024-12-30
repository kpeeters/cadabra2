
#include "Kernel.hh"
#include "properties/Indices.hh"
#include "properties/Integer.hh"
#include "properties/Coordinate.hh"
#include "Exceptions.hh"
#include "Functional.hh"

using namespace cadabra;

#define MAX_INTEGER_RANGE 100

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
			if(*ki->second.begin()->multiplier!=1) 
				throw std::logic_error("Indices: use quotes to label names when they start with a number.");
			if(!ki->second.is_string())
				throw std::logic_error("Indices: label has to be a normal string, not a mathematical expression.");
			
			set_name=*ki->second.begin()->name;
			if(set_name.size()>0) {
				if(set_name[0]=='\"' && set_name[set_name.size()-1]=='\"')
					set_name=set_name.substr(1,set_name.size()-2);
				}
			}
		else if(ki->first=="parent") {
			if(*ki->second.begin()->multiplier!=1) 
				throw std::logic_error("Indices: use quotes to label names when they start with a number.");
			if(!ki->second.is_string())
				throw std::logic_error("Indices: 'parent' has to be a normal string, not a mathematical expression.");

			parent_name=*ki->second.begin()->name;
			if(parent_name.size()>0) {
				if(parent_name[0]=='\"' && parent_name[set_name.size()-1]=='\"')
					parent_name=parent_name.substr(1,parent_name.size()-2);
				}
			}
		else if(ki->first=="position") {
			if(ki->second.equals("free"))
				position_type=free;
			else if(ki->second.equals("fixed"))
				position_type=fixed;
			else if(ki->second.equals("independent"))
				position_type=independent;
			else throw ConsistencyException("Position type should be fixed, free or independent.");
			}
		else if(ki->first=="values") {
			//std::cerr << "got values keyword " << *(ki->second->name) << std::endl;
			if(*ki->second.begin()->name=="\\sequence") {
				// Only accept a sequence if both start and end are explicit integers.
				Ex::sibling_iterator sqit1 = Ex::child(ki->second.begin(), 0);
				Ex::sibling_iterator sqit2 = Ex::child(ki->second.begin(), 1);
				if(!sqit1->is_integer() || !sqit2->is_integer()) 
					throw ConsistencyException("Value sequence for Indices property should contain explicit integers.");

				auto args = std::make_shared<cadabra::Ex>(ki->second.begin());
				auto prop = new Integer();
				kernel.inject_property(prop, ex, args);

				if(prop->difference.to_integer() > MAX_INTEGER_RANGE)
					throw ConsistencyException("Value sequence for Indices property spans too many elements.");

				for (int i=prop->from.to_integer(); i<=prop->to.to_integer(); ++i) {
					values.push_back(Ex(i));
					}
				
				++ki;
				continue;
				}

			collect_index_values(ki->second.begin());
			// If all values are integers, add an `Integer' property for the object,
			// listing these integers.
			bool is_number=true;
			bool is_continuous=false;
			for(auto& val: values)
				if(!val.begin()->is_integer()) {
					is_number=false;
					break;
					}
			if(is_number) {
				std::sort(values.begin(), values.end(), [](Ex a, Ex b) {
					return a.to_integer() < b.to_integer();
					});
				is_continuous = (int)values.size() == (values[values.size() - 1].to_integer() - values[0].to_integer() + 1);
				}
			// Do not apply Integer to a list of integers with gaps as
			// the former can only deal with continuous ranges.
			if(is_continuous) {
//				std::cerr << "Injecting Integer property" << std::endl;
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
			str << "(position=free";
			break;
		case fixed:
			str << "(position=fixed";
			break;
		case independent:
			str << "(position=independent";
			break;
		}
	if(values.size()>0) {
		str << ", values=\\{";
		bool first=true;
		for(const auto& v: values) {
			if(first)
				first=false;
			else
				str << ", ";
			str << v;
			}
		str << "\\})";
		}
	else {
		str << ")";
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

