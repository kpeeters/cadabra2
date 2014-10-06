
#pragma once

// CodeInput is essentially a TextView with some
// additional i/o logic.

#include <gtkmm/box.h>
#include <gtkmm/textview.h>
#include <gtkmm/separator.h>

namespace cadabra {

	class CodeInput : public Gtk::VBox {
		public:
			CodeInput(Glib::RefPtr<Gtk::TextBuffer>, const std::string& fontname, int hmargin=25);
			
			class exp_input_tv : public Gtk::TextView {
				public:
					exp_input_tv(Glib::RefPtr<Gtk::TextBuffer>);
					virtual bool on_key_press_event(GdkEventKey*);
					virtual bool on_expose_event(GdkEventExpose* event);
					
					sigc::signal1<bool, std::string> emitter;
					sigc::signal0<bool>              content_changed;
			};
			
			bool handle_button_press(GdkEventButton *);
			
			
			exp_input_tv               edit;
			Gtk::HBox                  hbox;
			Gtk::VSeparator            vsep;
	};

}
