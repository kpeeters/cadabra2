
#pragma once

#include <gtkmm/box.h>
#include <gtkmm/image.h>
#ifndef USE_GTK4
#include <gtkmm/eventbox.h>
#endif

namespace cadabra {

	/// \ingroup frontend
	///
	/// An image viewing widget.

#ifdef USE_GTK4
	class ImageView : public Gtk::Box {
#else
	class ImageView : public Gtk::EventBox {
#endif
		public:
			ImageView(double scale);
			virtual ~ImageView();

			void set_image_from_base64(const std::string& b64);
			void set_image_from_svg(const std::string& svg);

			virtual bool on_motion_notify_event(GdkEventMotion *event) override;
			virtual bool on_button_press_event(GdkEventButton *event) override;
			virtual bool on_button_release_event(GdkEventButton *event) override;

		private:
			std::string decoded; // raw byte content of image
			bool        is_raster;
			
			Gtk::Box    vbox;
			Gtk::Image  image;
			Glib::RefPtr<Gdk::Pixbuf> pixbuf;

			bool   sizing;
			double prev_x, prev_y;
			int    height_at_press, width_at_press;
			double scale;

			// Re-render the image from the raw bytes at the current
			// resolution of the image widget (or 400 if not set).
			void rerender(int override_width=0);
		};

	};
