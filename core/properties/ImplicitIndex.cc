
#include "properties/ImplicitIndex.hh"
#include "Exceptions.hh"

using namespace cadabra;

std::string ImplicitIndex::name() const
	{
	return "ImplicitIndex";
	}

bool ImplicitIndex::parse(Kernel&, keyval_t& keyvals)
	{
	keyval_t::const_iterator ki=keyvals.begin();
	while(ki!=keyvals.end()) {
		//		std::cout << "ImplicitIndex: " << ki->first << " = " << *ki->second->name << std::endl;
		if(ki->first=="name") {
			throw std::logic_error("ImplicitIndex: argument 'name' no longer supported");
			}
		else if(ki->first=="explicit") {
			explicit_form=ki->second;
			}
		else throw ConsistencyException("Property 'ImplicitIndex' does not accept key '"+ki->first+"'.");
		++ki;
		}

	return true;
	}

void ImplicitIndex::latex(std::ostream& str) const
	{
	property::latex(str);
	//	for(size_t n=0; n<set_names.size(); ++n) {
	//		if(n>0) str << ", ";
	//		str << set_names[n];
	//		}
	}
