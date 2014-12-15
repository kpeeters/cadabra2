
#pragma once

// A VisualCell contains a pointer to a DataCell, as well as
// pointers to the various cell widgets, one of which will be
// representing the DataCell.

#include "TeXView.hh"
#include "CodeInput.hh"
#include "DataCell.hh"

namespace cadabra {

	class VisualCell {
		public:
			// All cells below should derive from Gtk::VBox.
			union {
					Gtk::VBox    *document; // top-level cell; only one ever occurs in a document
					CodeInput    *inbox;
					TeXView      *outbox;
//					TeXInput        *texbox;
			};
	};
	
}
