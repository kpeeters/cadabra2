
#include "properties/ImplicitIndex.hh"
#include "Exceptions.hh"

std::string ImplicitIndex::name() const
	{
	return "ImplicitIndex";
	}

bool ImplicitIndex::parse(const Properties&, keyval_t& keyvals)
	{
	keyval_t::const_iterator ki=keyvals.begin();
	while(ki!=keyvals.end()) {
//		std::cout << "ImplicitIndex: " << ki->first << " = " << *ki->second->name << std::endl;
		if(ki->first=="name") {
			if(*ki->second->multiplier!=1) {
				throw std::logic_error("ImplicitIndex: use quotes to label names when they start with a number.");
				}
			set_names.push_back(*ki->second->name);
			}
		else throw ConsistencyException("Property 'ImplicitIndex' does not accept key '"+ki->first+"'.");
		++ki;
		}

	return true;
	}

void ImplicitIndex::display(std::ostream& str) const
	{
	property::display(str);
	for(size_t n=0; n<set_names.size(); ++n) {
		if(n>0) str << ", ";
		str << set_names[n];
		}
	}
