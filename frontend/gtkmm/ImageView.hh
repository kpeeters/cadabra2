
#pragma once

#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/drawingarea.h>

namespace cadabra {

	class ImageView;
	
	/// Widget which behaves as Gtk::Image, but instead uses the
	/// full device resolution (so images come out sharp).
	
	class ImageArea : public Gtk::DrawingArea {
		public:
			ImageArea(int logical_width, double display_scale);
			ImageArea(int logical_width, double display_scale,
						 const std::string& filename, bool raster);

			void set_image_from_base64(const std::string& b64);
			void set_image_from_svg(const std::string& svg);

			// Re-render the image from the raw bytes at the indicated
			// width (in logical pixels, which may be multiple display pixels).
			void rerender(int width);
			
			friend ImageView;
		protected:
			virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;

		private:
			Glib::RefPtr<Gdk::Pixbuf> pixbuf;
			
			std::string decoded; // raw byte content of image
			bool        is_raster;
			double      display_scale;
			int         logical_width;
	};
			

	/// \ingroup frontend
	///
	/// An image viewing widget.
	
	class ImageView : public Gtk::EventBox {
		public:
			ImageView(double display_scale, int logical_width);
			virtual ~ImageView();
			
			virtual bool on_motion_notify_event(GdkEventMotion *event) override;
			virtual bool on_button_press_event(GdkEventButton *event) override;
			virtual bool on_button_release_event(GdkEventButton *event) override;
			
			void set_image_from_base64(const std::string& b64);
			void set_image_from_svg(const std::string& svg);

		private:
			ImageArea area;
			
			bool      sizing;
			double    prev_x, prev_y;
			int       height_at_press, width_at_press;
			
	};
	
};
