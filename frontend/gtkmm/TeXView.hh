
#pragma once

#include "GtkCompat.hh"

#ifndef USE_GTK4
#include <gtkmm/eventbox.h>
#endif
#include <gtkmm/box.h>
#include <gtkmm/drawingarea.h>
#if GTKMM_MAJOR_VERSION>3 || (GTKMM_MAJOR_VERSION==3 && GTKMM_MINOR_VERSION>=10)
#include <gtkmm/revealer.h>
#endif

#include "Config.hh"
#include "DataCell.hh"
#include "../common/TeXEngine.hh"

// MicroTeX
#include "microtex.h"
#include "utils/utf.h"

namespace cadabra {

	/// \ingroup frontend
	/// TeXView is a widget which knows how to turn a string into
	/// a LaTeX-rendered image and display that.
	//
	/// See gtkmm custom widget documentation at:
	/// https://developer-old.gnome.org/gtkmm-tutorial/3.24/sec-custom-widgets.html.en

	/// Gtk::EventBox is removed in GTK4 (every widget can receive events
	/// there); a plain Gtk::Box takes its place.
#ifdef USE_GTK4
	using TeXViewBase = Gtk::Box;
#else
	using TeXViewBase = Gtk::EventBox;
#endif

	class TeXView : public TeXViewBase {
		public:
			TeXView(TeXEngine&, DTree::iterator, int hmargin=25);
			virtual ~TeXView();

			std::shared_ptr<TeXEngine::TeXRequest> content;

			sigc::signal<bool(DTree::iterator)>   show_hide_requested;

			/// Emitted on a button press which is not the context-menu button, so
			/// that the owner can select this cell. Previously this was done by
			/// connecting to signal_button_press_event() and relying on the
			/// default handler having already swallowed right-clicks.
			sigc::signal<void()>                    select_requested;

			void set_is_error(bool);
					
			DTree::iterator           datacell;
			Gtk::Revealer             rbox;
			Gtk::Box                  vbox;
			Gtk::Box                  hbox;

			int                       window_width=0;

			class TeXArea : public Gtk::DrawingArea {
				public:
					TeXArea(TeXView *owner_);
					~TeXArea();

					/// The actual painting, shared between the two toolkits.
					/// gtkmm-3 reaches it through the on_draw vfunc, GTK4
					/// through the callback registered with set_draw_func().
					bool draw_impl(const Cairo::RefPtr<Cairo::Context>& cr);
#ifdef USE_GTK4
					void draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height);
#else
					virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;
#endif

					/// Propagate a change in the TeXRequest (i.e. a new LaTeX
					/// string) through to the visible widget.

					void update_image(std::shared_ptr<TeXEngine::TeXRequest>);

					float                     _text_size;

					/// MicroTeX: set the LaTeX string and
					/// replace/substitute so that MicroTeX can render it
					/// (but do not actually render).
					void set_latex(const std::string& latex);

					/// MicroTeX: just run the LaTeX layout algorithms, do
					/// not draw.
					void layout_latex() const;

					int need_height(int width) const;
				protected:
					Gtk::SizeRequestMode get_request_mode_vfunc() const override;

					/// The MicroTeX size logic, shared between the two toolkits.
					void measure_height_for_width(int width, int& minimum_height,
														   int& natural_height) const;
					void relayout_for_allocation(int width, int height);

#ifdef USE_GTK4
					void measure_vfunc(Gtk::Orientation orientation, int for_size,
											 int& minimum, int& natural,
											 int& minimum_baseline, int& natural_baseline) const override;
					void size_allocate_vfunc(int width, int height, int baseline) override;
#else
					void get_preferred_height_for_width_vfunc(int width, int& minimum_height,
																			int& natural_height) const  override;
					void get_preferred_width_for_height_vfunc(int height, int& minimum_width,
																			int& natural_width) const  override;
					void on_size_allocate(Gtk::Allocation& allocation) override;
#endif

				private:
					TeXView        *owner;
					mutable int     rendering_width;
					int             padding_x, padding_y;

					// MicroTeX
					mutable microtex::Render* _render;
					std::string               unfixed, fixed;

					// Return a string in which backslashes, curly braces,
					// underscores and tildes are properly backslashed, so they
					// typeset as if we are in a verbatim environment.
					std::string verbatimise(const std::string&) const;
				};

			TeXArea                   image;

			/// Update the TeX image.
			void update_image();

			/// Dim the output to indicate that the result is no longer guaranteed to
			/// be correlated with the input cell from which it was derived.

			void dim(bool);

			sigc::signal<bool(std::string)> tex_error;
			sigc::signal<bool()>              signal_on_copy_as_latex;
			
		protected:
#ifdef USE_GTK4
			// See the comment on the definition: GTK4 never emits ::show for
			// these cells, so the initial conversion hangs off ::map instead.
			void on_map() override;
#else
			virtual void on_show() override;
#endif

			/// Button handling goes through a gesture controller rather than the
			/// on_button_{press,release}_event vfuncs, which are removed in GTK4.
			void on_button_pressed(int n_press, double x, double y);
			void on_button_released(int n_press, double x, double y);

			void convert();

			void on_copy_as_latex() const;

		private:
			TeXEngine& engine;
			bool is_error;
			
			cdb::ContextMenu context_menu;

			Glib::RefPtr<cdb::ClickGesture>     click_gesture;

			float text_size() const;
		};

	}

