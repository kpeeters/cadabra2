
#pragma once

// A VisualCell contains a pointer to a DataCell, as well as
// pointers to the various cell widgets, one of which will be
// representing the DataCell.

#include "TeXView.hh"
#include "TeXEdit.hh"
#include "CodeInput.hh"
#include "DataCell.hh"

namespace cadabra {

	/// \ingroup gtkmm
	///
	/// Structure holding a pointer to one of the possible GUI widgets that can 
	/// appear in a document.

	class VisualCell {
		public:
			/// Union of pointers to one of the possible GUI realisations
			/// of the cell types declared in DataCell::CellType. All
			/// of these cells below should derive from Gtk::VBox.

			union {
					Gtk::VBox    *document; // top-level cell; only one ever occurs in a document
					CodeInput    *inbox;
					TeXView      *outbox;
					TeXEdit      *latexbox;
			};
	};
	
}
