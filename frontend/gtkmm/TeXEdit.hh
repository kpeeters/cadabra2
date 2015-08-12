
#pragma once

#include "DataCell.hh"
#include "TeXView.hh"
#include <gtkmm/box.h>
#include <gtkmm/textview.h>

namespace cadabra {

   /// \ingroup gtkmm
   ///
   /// A widget which can be used to edit and display TeX
   /// input. Double-clicking on the graphical TeX version toggles
   /// visibility of the edit box.

	class TeXEdit : public Gtk::VBox {
		public:
			/// Initialise with existing TextBuffer and a pointer to the Datacell
			/// corresponding to this CodeInput widget.
			TeXEdit(DTree::iterator, Glib::RefPtr<Gtk::TextBuffer>, TeXEngine&);

			/// Initialise with a new TextBuffer (to be created by
			/// CodeInput), filling it with the content of the given
			/// string.
			TeXEdit(DTree::iterator, const std::string&, TeXEngine&);

			/// Shared init member.
			void init();

			/// The edit box part of the widget. This closely follows CodeInput::exp_input_tv.

			class exp_input_tv : public Gtk::TextView {
				public:
					exp_input_tv(DTree::iterator, Glib::RefPtr<Gtk::TextBuffer>);
					virtual bool on_key_press_event(GdkEventKey*) override;
					virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>&) override;
					virtual bool on_focus_in_event(GdkEventFocus *) override;
					
					sigc::signal1<bool, DTree::iterator>              content_execute;
					sigc::signal2<bool, std::string, DTree::iterator> content_changed;
					sigc::signal1<bool, DTree::iterator>              cell_got_focus;

					friend class TeXEdit;

				private:
					DTree::iterator datacell;
					bool is_modified;
					bool folded_away;
			};
			
			/// We cannot edit the content of the DataCell directly,
			/// because Gtk needs a Gtk::TextBuffer. However, the
			/// CodeInput widgets corresponding to a single DataCell all
			/// share their TextBuffer.

			Glib::RefPtr<Gtk::TextBuffer> buffer;

			exp_input_tv  edit;
			
			bool is_folded() const;
			void set_folded(bool);
			
			/// The other part of the widget is a TeXView, which holds all the LaTeX
			/// functionality.

			TeXView  texview;
	};

};

