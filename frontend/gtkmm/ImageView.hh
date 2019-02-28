
#pragma once

#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/eventbox.h>

namespace cadabra {

   /// \ingroup frontend
   ///
   /// An image viewing widget.

	class ImageView : public Gtk::EventBox {
		public:
			ImageView();
			virtual ~ImageView();

			void set_image_from_base64(const std::string& b64);

			virtual bool on_motion_notify_event(GdkEventMotion *event) override;
			virtual bool on_button_press_event(GdkEventButton *event) override;
			virtual bool on_button_release_event(GdkEventButton *event) override;
			
		private:
			Gtk::VBox   vbox;
			Gtk::Image  image;
			Glib::RefPtr<Gdk::Pixbuf> pixbuf;			

			bool   sizing;
			double prev_x, prev_y;
			int    height_at_press, width_at_press;
	};

};
