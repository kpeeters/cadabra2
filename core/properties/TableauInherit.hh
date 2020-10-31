
#pragma once

#include "Props.hh"
#include "properties/TableauBase.hh"

namespace cadabra {

	/// Property which makes a node inherit the TableauBase properties of
	/// child nodes. The `size` and `get_tab` functions translate the
	/// index numbers of the child nodes to index numbers of the
	/// wrapping node.
	
	class TableauInherit : virtual public TableauBase, virtual public property {
		public:
			virtual std::string name() const
				{
				return std::string("TableauInherit");
				};

			virtual unsigned int size(const Properties&, Ex&, Ex::iterator) const;
			virtual tab_t        get_tab(const Properties&, Ex&, Ex::iterator, unsigned int) const;
			
		};

	}
