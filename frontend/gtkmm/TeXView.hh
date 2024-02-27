
#pragma once

#include <gtkmm/eventbox.h>
#include <gtkmm/box.h>
#include <gtkmm/drawingarea.h>
#if GTKMM_MINOR_VERSION>=10
#include <gtkmm/revealer.h>
#endif

#include "DataCell.hh"
#include "../common/TeXEngine.hh"

#ifdef USE_MICROTEX
#include "latex.h"
#include "utils/utf.h"
#endif

namespace cadabra {

	/// \ingroup frontend
	/// TeXView is a widget which knows how to turn a string into
	/// a LaTeX-rendered image and display that.

	class TeXView : public Gtk::EventBox {
		public:
			TeXView(TeXEngine&, DTree::iterator, int hmargin=25);
			virtual ~TeXView();

			std::shared_ptr<TeXEngine::TeXRequest> content;

			sigc::signal1<bool, DTree::iterator>   show_hide_requested;

			DTree::iterator           datacell;
#if GTKMM_MINOR_VERSION>=10
			Gtk::Revealer             rbox;
#endif
			Gtk::VBox                 vbox;
			Gtk::HBox                 hbox;

			class TeXArea : public Gtk::DrawingArea {
				public:
					TeXArea();
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

#ifdef USE_MICROTEX
					void set_latex(const std::string& latex);
#endif

				protected:
					virtual bool on_configure_event(GdkEventConfigure *) override;
					
				private:
					int             rendering_width;
					
#ifdef USE_MICROTEX
					void check_invalidate();
					
					tex::TeXRender* _render;
					float           _text_size;
					int             _padding;
#endif
				};

			TeXArea                   image;

			/// Update the TeX image.
			void update_image();

			/// Dim the output to indicate that the result is no longer guaranteed to
			/// be correlated with the input cell from which it was derived.

			void dim(bool);

			sigc::signal1<bool, std::string> tex_error;

		protected:
			virtual bool on_button_release_event(GdkEventButton *) override;
			virtual void on_show() override;

			void convert();

		private:
			TeXEngine& engine;
		};

	}

