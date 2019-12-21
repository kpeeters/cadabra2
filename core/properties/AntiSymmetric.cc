
#include "Algorithm.hh"
#include "properties/AntiSymmetric.hh"

using namespace cadabra;

std::string AntiSymmetric::name() const
	{
	return "AntiSymmetric";
	}

bool AntiSymmetric::parse(Kernel&, keyval_t& keyvals)
	{
	keyval_t::const_iterator kv=keyvals.find("indices");
	if(kv!=keyvals.end())
		index_set_name=*(kv->second->name);
	return true;
	}

unsigned int AntiSymmetric::size(const Properties&, Ex&, Ex::iterator) const
	{
	return 1;
	}

TableauBase::tab_t AntiSymmetric::get_tab(const Properties& pr, Ex& tr, Ex::iterator it, unsigned int num) const
	{
	assert(num==0);

	const AntiSymmetric *pd;
	for(;;) {
		pd=pr.get<AntiSymmetric>(it);
		if(!pd)
			it=tr.begin(it);
		else break;
		}

	tab_t tab;
	for(unsigned int i=0; i<Ex::number_of_children(it); ++i)
		tab.add_box(i,i);
	return tab;
	}

