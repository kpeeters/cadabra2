
#include "CoreProps.hh"
#include "Storage.hh"
#include "Props.hh"

void properties::register_properties()
	{
	register_property(&create_property<IndexInherit>);
	register_property(&create_property<PropertyInherit>);
	register_property(&create_property<Symbol>);
	register_property(&create_property<Indices>);
	register_property(&create_property<Coordinate>);
	register_property(&create_property<SortOrder>);
	register_property(&create_property<ImplicitIndex>);
	register_property(&create_property<Distributable>);
	register_property(&create_property<Accent>);
	register_property(&create_property<DiracBar>);

	// commutativity
	register_property(&create_property<CommutingAsProduct>);
	register_property(&create_property<CommutingAsSum>);
	}
