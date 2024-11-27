
#pragma once

#include "DataCell.hh"
#include "DocumentThread.hh"
#include <gtkmm/box.h>
#include <gtkmm/textview.h>
#include <gtkmm/separator.h>

namespace cadabra {

	/// \ingroup frontend
	///
	/// A text cell editor widget with support for editing Python and LaTeX.
	/// CodeInput is essentially a TextView with some additional i/o logic.

	class CodeInput : public Gtk::VBox {
		public:
			using Prefs = cadabra::DocumentThread::Prefs;
			/// Initialise with existing TextBuffer and a pointer to the Datacell
			/// corresponding to this CodeInput widget. CodeInput is not allowed
			/// to modify this DataCell directly, but can read properties from
			/// it (e.g. in order to know when to display a 'busy' indicator).
			/// The scale parameter refers to hdpi scaling.

			CodeInput(DTree::iterator, Glib::RefPtr<Gtk::TextBuffer>, double scale, const Prefs& prefs,
						 Glib::RefPtr<Gtk::Adjustment>);

			/// Initialise with a new TextBuffer (to be created by
			/// CodeInput), filling it with the content of the given
			/// string.

			CodeInput(DTree::iterator, const std::string&, double scale, const Prefs& prefs,
						 Glib::RefPtr<Gtk::Adjustment>);


			virtual void on_size_allocate(Gtk::Allocation& allocation) override;
			
			/// The actual text widget used by CodeInput.

			class exp_input_tv : public Gtk::TextView {
				public:
					exp_input_tv(DTree::iterator, Glib::RefPtr<Gtk::TextBuffer>, double scale,
									 Glib::RefPtr<Gtk::Adjustment>);
					virtual bool on_key_press_event(GdkEventKey*) override;
					virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>&) override;
					virtual bool on_focus_in_event(GdkEventFocus *) override;
					virtual bool on_focus_out_event(GdkEventFocus *) override;
					virtual void on_show() override;
//					virtual bool on_move_cursor_event(Glib::RefPtr<Gtk::TextBuffer::Mark>, Gtk::MovementStep, bool) override;
					virtual bool on_motion_notify_event(GdkEventMotion *event) override;
					
					void         shift_enter_pressed();
					void         on_textbuf_change();

					sigc::signal1<bool, DTree::iterator>                   content_execute;
					sigc::signal1<bool, DTree::iterator>                   content_changed;
					sigc::signal3<bool, std::string, int, DTree::iterator> content_insert;
					sigc::signal3<bool, int, int, DTree::iterator>         content_erase;
					sigc::signal1<bool, DTree::iterator>                   cell_got_focus;
					sigc::signal2<bool, DTree::iterator, int>              complete_request;

					friend CodeInput;

				private:
					double                        scale_;
					DTree::iterator               datacell;
					Glib::RefPtr<Gtk::Adjustment> vadjustment;
					double                        previous_value = -99.0;
				};

			/// Set highlighting modes.

			void enable_highlighting(DataCell::CellType cell_type, const Prefs& prefs);
			void disable_highlighting();

			void relay_cursor_pos(std::function<void(int, int)> callback);

			/// Handle mouse buttons.

			bool handle_button_press(GdkEventButton *);

			/// Handle an insert event, which can consist of one or more
			/// inserted characters. This function will just massage that
			/// data and then feed it through to the notebook window class
			/// by emitting a signal on content_insert (done like this to
			/// separate DTree modification from the widget).

			void handle_insert(const Gtk::TextIter& pos, const Glib::ustring& text, int bytes);

			/// Handle an erase event. This function will just massage that
			/// data and then feed it through to the notebook window class
			/// by emitting a signal on content_erase (done like this to
			/// separate DTree modification from the widget).

			void handle_erase(const Gtk::TextIter& start, const Gtk::TextIter& end);

			/// Ensure that the visual representation matches the DTree
			/// cell.

			void update_buffer();

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
			void init(const Prefs& prefs);

			void highlight_python();
			void highlight_latex();

			sigc::connection hl_conn; // Connection holding the syntax highlighting signal
		};

	}
