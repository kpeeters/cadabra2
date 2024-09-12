
#pragma once


#include "properties/TableauBase.hh"
#include "properties/Traceless.hh"

namespace cadabra {

	class AntiSymmetric : public TableauBase, public Traceless, virtual public property {
		public:
			virtual ~AntiSymmetric();
			virtual std::string name() const override;

			virtual unsigned int size(const Properties&, Ex&, Ex::iterator) const override;
			virtual tab_t        get_tab(const Properties&, Ex&, Ex::iterator, unsigned int) const override;
		};

	}
