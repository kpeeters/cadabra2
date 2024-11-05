
#pragma once

#include <gtkmm/box.h>
#include <gtkmm/glarea.h>
#include <gtkmm/eventbox.h>

namespace cadabra {

	/// \ingroup frontend
	///
	/// A widget to display OpenGL (or perhaps one day Metal) content.

	class GraphicsView : public Gtk::EventBox {
		public:
			GraphicsView();
			virtual ~GraphicsView();

			virtual bool on_motion_notify_event(GdkEventMotion *event) override;
			virtual bool on_button_press_event(GdkEventButton *event) override;
			virtual bool on_button_release_event(GdkEventButton *event) override;

			class GLView : public Gtk::GLArea {
				public:
					virtual bool on_render (const Glib::RefPtr< Gdk::GLContext > &context);
			};
			
		private:
			Gtk::VBox   vbox;
			GLView      glview;

			bool   sizing;
			double prev_x, prev_y;
			int    height_at_press, width_at_press;
		};

	};
