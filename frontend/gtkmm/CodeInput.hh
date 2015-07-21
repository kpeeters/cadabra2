
#pragma once

// CodeInput is essentially a TextView with some
// additional i/o logic.

#include "DataCell.hh"
#include <gtkmm/box.h>
#include <gtkmm/textview.h>
#include <gtkmm/separator.h>

namespace cadabra {

	class CodeInput : public Gtk::VBox {
		public:
			// Initialise with a new empty TextBuffer.
//			CodeInput();

			// Initialise with existing TextBuffer and a pointer to the Datacell
			// corresponding to this CodeInput widget.
			CodeInput(DTree::iterator, Glib::RefPtr<Gtk::TextBuffer>);

			// Initialise with a new TextBuffer, filling it with the content of the
			// given string.
			CodeInput(DTree::iterator, const std::string&);
			
			class exp_input_tv : public Gtk::TextView {
				public:
					exp_input_tv(DTree::iterator, Glib::RefPtr<Gtk::TextBuffer>);
					virtual bool on_key_press_event(GdkEventKey*) override;
					virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>&) override;
					virtual bool on_focus_in_event(GdkEventFocus *) override;
					
					sigc::signal1<bool, DTree::iterator>              content_execute;
					sigc::signal2<bool, std::string, DTree::iterator> content_changed;
					sigc::signal1<bool, DTree::iterator>              cell_got_focus;

				private:
					DTree::iterator datacell;
			};

			bool handle_button_press(GdkEventButton *);

			// We cannot edit the content of the DataCell directly,
			// because Gtk needs a Gtk::TextBuffer. The CodeInput widgets
			// corresponding to a single DataCell all share their 
			// TextBuffer, however.

			Glib::RefPtr<Gtk::TextBuffer> buffer;

			exp_input_tv                  edit;

		private:
			void init();
	};

}
