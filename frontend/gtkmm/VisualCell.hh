
#pragma once

// A VisualCell contains a pointer to a DataCell, as well as
// pointers to the various cell widgets, one of which will be
// representing the DataCell.

#include "TeXView.hh"
#include "DataCell.hh"

namespace cadabra {

	class VisualCell {
		public:
			union {
//					ExpressionInput *inbox;
					TeXView         *outbox;
//					TeXInput        *texbox;
			};
	};
	
}
