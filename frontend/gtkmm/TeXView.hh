
#pragma once

#ifndef USE_GTK4
#include <gtkmm/eventbox.h>
#endif
#include <gtkmm/box.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/menu.h>
#include <gtkmm/gesturemultipress.h>
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

	class TeXView : public Gtk::EventBox {
		public:
			TeXView(TeXEngine&, DTree::iterator, bool use_microtex_, int hmargin=25);
			virtual ~TeXView();

			std::shared_ptr<TeXEngine::TeXRequest> content;

			sigc::signal1<bool, DTree::iterator>   show_hide_requested;

			/// Emitted on a button press which is not the context-menu button, so
			/// that the owner can select this cell. Previously this was done by
			/// connecting to signal_button_press_event() and relying on the
			/// default handler having already swallowed right-clicks.
			sigc::signal0<void>                    select_requested;

			void set_use_microtex(bool);

			void set_is_error(bool);
					
			DTree::iterator           datacell;
			Gtk::Revealer             rbox;
			Gtk::Box                  vbox;
			Gtk::Box                  hbox;

			int                       window_width=0;

			class TeXArea : public Gtk::DrawingArea {
				public:
					TeXArea(bool use_microtex, TeXView *owner_);
					~TeXArea();

					virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;

					/// Update the visible image from the pixbuf. Call this in order to propagate
					/// changes to the pixbuf (e.g. from re-running the TeXRequest) to the
					/// visible widget itself.

					void update_image(std::shared_ptr<TeXEngine::TeXRequest>, double scale);

					/// The actual image is stored in the image referenced by pixbuf.
					/// FIXME: This pointer is not yet shared among instances which show the
					/// same content.

					Glib::RefPtr<Gdk::Pixbuf> pixbuf;
					double                    scale_;
					float                     _text_size;

					/// MicroTeX: set the LaTeX string and
					/// replace/substitute so that MicroTeX can render it
					/// (but do not actually render).
					void set_latex(const std::string& latex);

					/// MicroTeX: just run the LaTeX layout algorithms, do
					/// not draw.
					void layout_latex() const;

					bool                    use_microtex;

					int need_height(int width) const;
				protected:
					Gtk::SizeRequestMode get_request_mode_vfunc() const override;
					void get_preferred_height_for_width_vfunc(int width, int& minimum_height,
																			int& natural_height) const  override;
					void get_preferred_width_for_height_vfunc(int height, int& minimum_width,
																			int& natural_width) const  override;
					void on_size_allocate(Gtk::Allocation& allocation) override;
					
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

			sigc::signal1<bool, std::string> tex_error;
			sigc::signal0<bool>              signal_on_copy_as_latex;
			
		protected:
			virtual void on_show() override;

			/// Button handling goes through a gesture controller rather than the
			/// on_button_{press,release}_event vfuncs, which are removed in GTK4.
			void on_button_pressed(int n_press, double x, double y);
			void on_button_released(int n_press, double x, double y);

			void convert();

			void on_copy_as_latex() const;

		private:
			TeXEngine& engine;
			bool use_microtex;
			bool is_error;
			
			Gtk::Menu     context_menu;
			Gtk::MenuItem item_copy_as_latex;

			Glib::RefPtr<Gtk::GestureMultiPress> click_gesture;

			float text_size() const;
		};

	}

