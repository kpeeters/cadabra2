
#pragma once

// TeXInput is a widget which can be used to edit and display
// TeX input. Double-clicking on the graphical TeX version
// toggles visibility of the edit box.

#include "TeXView.hh"

namespace cadabra {

	class TeXEdit : public Gtk::VBox {
		public:
			TeXEdit(TeXEngine&, Glib::RefPtr<Gtk::TextBuffer>, TeXRequest *);
			
			class exp_input_tv : public Gtk::TextView {
				public:
					exp_input_tv(Glib::RefPtr<Gtk::TextBuffer>);
					virtual bool on_key_press_event(GdkEventKey*) override;

					sigc::signal0<bool>              content_execute;
					sigc::signal1<bool, std::string> content_changed;
					sigc::signal0<bool>              cell_got_focus;

					friend class TeXEdit;

				protected:
					bool is_modified;
					bool folded_away;
			};
			
			exp_input_tv  edit;
			
			bool is_folded() const;
			void set_folded(bool);
			
			TeXView  texview;
	};

};

