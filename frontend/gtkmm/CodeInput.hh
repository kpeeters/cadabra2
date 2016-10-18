
#pragma once

#include "DataCell.hh"
#include <gtkmm/box.h>
#include <gtkmm/textview.h>
#include <gtkmm/separator.h>

namespace cadabra {

	/// \ingroup gtkmm
	///
	/// A text cell editor widget with support for editing Python and LaTeX.
   /// CodeInput is essentially a TextView with some additional i/o logic.

	class CodeInput : public Gtk::VBox {
		public:
			/// Initialise with existing TextBuffer and a pointer to the Datacell
			/// corresponding to this CodeInput widget. CodeInput is not allowed
			/// to modify this DataCell directly, but can read properties from
			/// it (e.g. in order to know when to display a 'busy' indicator).
			/// The scale parameter refers to hdpi scaling.

			CodeInput(DTree::iterator, Glib::RefPtr<Gtk::TextBuffer>, double scale);

			// Initialise with a new TextBuffer (to be created by
			// CodeInput), filling it with the content of the given
			// string.

			CodeInput(DTree::iterator, const std::string&, double scale);
			
			class exp_input_tv : public Gtk::TextView {
				public:
					exp_input_tv(DTree::iterator, Glib::RefPtr<Gtk::TextBuffer>, double scale);
					virtual bool on_key_press_event(GdkEventKey*) override;
					virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>&) override;
					virtual bool on_focus_in_event(GdkEventFocus *) override;
					virtual void on_show() override;

					void         shift_enter_pressed();
					
					sigc::signal1<bool, DTree::iterator>                   content_execute;
//					sigc::signal2<bool, std::string, DTree::iterator>      content_changed;
					sigc::signal3<bool, std::string, int, DTree::iterator> content_insert;
					sigc::signal3<bool, int, int, DTree::iterator>         content_erase;
					sigc::signal1<bool, DTree::iterator>                   cell_got_focus;

					friend CodeInput;

				private:
					double scale_;
					DTree::iterator datacell;
			};

			bool handle_button_press(GdkEventButton *);
//			void handle_changed();
			void handle_insert(const Gtk::TextIter& pos, const Glib::ustring& text, int bytes);
			void handle_erase(const Gtk::TextIter& start, const Gtk::TextIter& end);
			void update_buffer(); // update buffer from datacell

			/// Return two strings corresponding to the text before and
			/// after the current cursor position.

			void slice_cell(std::string& before, std::string& after);

			/// We cannot edit the content of the DataCell directly,
			/// because Gtk needs a Gtk::TextBuffer. However, the
			/// CodeInput widgets corresponding to a single DataCell all
			/// share their TextBuffer.

			Glib::RefPtr<Gtk::TextBuffer> buffer;

			exp_input_tv                  edit;

		private:
			void init();
	};

}
