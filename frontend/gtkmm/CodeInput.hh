
#pragma once

#include "DataCell.hh"
#include "DocumentThread.hh"
#include <gtkmm/box.h>
#include <gtkmm/textview.h>
#include <gtkmm/separator.h>
#ifdef USE_GTK4
#include <gtkmm/snapshot.h>
#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/eventcontrollerfocus.h>
#include <gtkmm/eventcontrollermotion.h>
#endif

namespace cadabra {

	/// \ingroup frontend
	///
	/// A text cell editor widget with support for editing Python and LaTeX.
	/// CodeInput is essentially a TextView with some additional i/o logic.

	class CodeInput : public Gtk::Box {
		public:
			using Prefs = cadabra::DocumentThread::Prefs;
			/// Initialise with existing TextBuffer and a pointer to the Datacell
			/// corresponding to this CodeInput widget. CodeInput is not allowed
			/// to modify this DataCell directly, but can read properties from
			/// it (e.g. in order to know when to display a 'busy' indicator).
			/// The scale parameter refers to hdpi scaling.

			CodeInput(DTree::iterator, Glib::RefPtr<Gtk::TextBuffer>, double scale, double display_scale, const Prefs& prefs,
						 Glib::RefPtr<Gtk::Adjustment>);

			/// Initialise with a new TextBuffer (to be created by
			/// CodeInput), filling it with the content of the given
			/// string.

			CodeInput(DTree::iterator, const std::string&, double scale, double display_scale, const Prefs& prefs,
						 Glib::RefPtr<Gtk::Adjustment>);


#ifndef USE_GTK4
			// Chains to the base class only; not needed under GTK4.
			virtual void on_size_allocate(Gtk::Allocation& allocation) override;
#endif

			/// The actual text widget used by CodeInput.

			class exp_input_tv : public Gtk::TextView {
				public:
					exp_input_tv(DTree::iterator, Glib::RefPtr<Gtk::TextBuffer>,
									 double scale,
									 double display_scale,
									 Glib::RefPtr<Gtk::Adjustment>);
					virtual void on_show() override;

					/// Key handling, shared between the toolkits. Returns true
					/// when the key was consumed; false lets the TextView's own
					/// default handling run. gtkmm-3 reaches it through the
					/// on_key_press_event vfunc, GTK4 through an
					/// Gtk::EventControllerKey, which has the same contract.
					bool handle_key_press(guint keyval, Gdk::ModifierType state);

					/// Side effects of focus and pointer motion, likewise shared.
					void on_focus_entered();
					void on_focus_left();
					void on_pointer_motion();

#ifndef USE_GTK4
					virtual bool on_key_press_event(GdkEventKey*) override;
					virtual bool on_focus_in_event(GdkEventFocus *) override;
					virtual bool on_focus_out_event(GdkEventFocus *) override;
					virtual bool on_motion_notify_event(GdkEventMotion *event) override;
#endif

					virtual Gtk::SizeRequestMode get_request_mode_vfunc() const override;

					/// Paint the coloured bar down the left-hand side of the
					/// cell, on top of the TextView's own rendering.
					void draw_gutter(const Cairo::RefPtr<Cairo::Context>& cr, int height);

					/// Width logic for LaTeX cells, shared between the toolkits;
					/// returns false when the base class should decide instead.
					bool measure_width_for_height(int& minimum_width, int& natural_width) const;

#ifdef USE_GTK4
					void snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot>& snapshot) override;
					void measure_vfunc(Gtk::Orientation orientation, int for_size,
											 int& minimum, int& natural,
											 int& minimum_baseline, int& natural_baseline) const override;
#else
					virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>&) override;
					virtual void get_preferred_width_for_height_vfunc(int height, int& minimum_width, int& natural_width) const  override;
					virtual void on_size_allocate(Gtk::Allocation& allocation) override;
#endif

					void         shift_enter_pressed();
					void         on_textbuf_change();

					sigc::signal1<bool, DTree::iterator>                   content_execute;
					sigc::signal1<bool, DTree::iterator>                   content_changed;
					sigc::signal3<bool, std::string, int, DTree::iterator> content_insert;
					sigc::signal3<bool, int, int, DTree::iterator>         content_erase;
					sigc::signal1<bool, DTree::iterator>                   cell_got_focus;
					sigc::signal2<bool, DTree::iterator, int>              complete_request;

					friend CodeInput;

					int                           window_width = 100;
				private:
					double                        scale_, display_scale_;
					DTree::iterator               datacell;
					Glib::RefPtr<Gtk::Adjustment> vadjustment;
					double                        previous_value = -99.0;
#ifdef USE_GTK4
					Glib::RefPtr<Gtk::EventControllerKey>    key_controller;
					Glib::RefPtr<Gtk::EventControllerFocus>  focus_controller;
					Glib::RefPtr<Gtk::EventControllerMotion> motion_controller;
#endif
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
			void init(const Prefs& prefs, double display_scale);

			void highlight_python();
			void highlight_latex();

			sigc::connection hl_conn; // Connection holding the syntax highlighting signal
		};

	}
