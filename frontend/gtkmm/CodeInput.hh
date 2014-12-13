
#pragma once

// CodeInput is essentially a TextView with some
// additional i/o logic.

#include <gtkmm/box.h>
#include <gtkmm/textview.h>
#include <gtkmm/separator.h>

namespace cadabra {

	class CodeInput : public Gtk::VBox {
		public:
			CodeInput();
			CodeInput(Glib::RefPtr<Gtk::TextBuffer>);
			
			class exp_input_tv : public Gtk::TextView {
				public:
					exp_input_tv(Glib::RefPtr<Gtk::TextBuffer>);
					virtual bool on_key_press_event(GdkEventKey*) override;
					virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>&) override;
					
					sigc::signal0<bool>              content_changed;
					sigc::signal1<bool, std::string> content_execute;
			};
			
			bool handle_button_press(GdkEventButton *);
	
			// We cannot edit the content of the DataCell directly,
			// because Gtk needs a Gtk::TextBuffer. The CodeInput widgets
			// corresponding to a single DataCell all share their 
			// TextBuffer, however.

			Glib::RefPtr<Gtk::TextBuffer> buffer;

			exp_input_tv                  edit;
			Gtk::HBox                     hbox;
			Gtk::VSeparator               vsep;

		private:
			void init();
	};

}
