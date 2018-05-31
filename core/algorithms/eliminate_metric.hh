
#pragma once

#include "Algorithm.hh"
#include "IndexIterator.hh"

namespace cadabra {

class eliminate_converter : public Algorithm {
	public:
		eliminate_converter(const Kernel&, Ex&, Ex&);

		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;

	protected:
		virtual bool is_conversion_object(iterator) const=0; 

	private:
		Ex          preferred;
		index_map_t ind_dummy, ind_free;
		bool handle_one_index(index_iterator, index_iterator, iterator, sibling_iterator);
};


class eliminate_metric : public eliminate_converter {
	public:
		eliminate_metric(const Kernel&, Ex&, Ex&);
		
	protected:
		virtual bool is_conversion_object(iterator) const override; 
};

}
