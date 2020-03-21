
#pragma once

#include "Props.hh"
#include "properties/Symmetric.hh"
#include "properties/Distributable.hh"
#include "properties/IndexInherit.hh"
#include "properties/TableauInherit.hh"
#include "properties/NumericalFlat.hh"

namespace cadabra {

	class Trace :
			public Distributable,
			public IndexInherit,
			public TableauInherit,
			public NumericalFlat,
			virtual public property {
		public:
			Trace();
			virtual std::string name() const override;
			virtual std::string unnamed_argument() const override;
			virtual bool        parse(Kernel&, keyval_t&) override;
			virtual void        validate(const Kernel&, const Ex&) const override;
			virtual void        latex(std::ostream&) const override;

			Ex obj;
			std::string index_set_name; // refers to Indices::set_name
		};

	}
