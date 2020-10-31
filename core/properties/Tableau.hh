
#pragma once

#include "Props.hh"
#include "properties/ImplicitIndex.hh"

namespace cadabra {

	/// Property representing a Young tableau with unlabelled
	/// boxes. Note the difference with TableauBase, which represents
	/// a (set of) Young tableaux and the map of the boxes to
	/// tensor indices.

	class Tableau : public ImplicitIndex, virtual public property {
		public:
			virtual ~Tableau() {};
			virtual std::string name() const override;
			virtual bool        parse(Kernel&, keyval_t& keyvals) override;

			int dimension;
		};

	}
