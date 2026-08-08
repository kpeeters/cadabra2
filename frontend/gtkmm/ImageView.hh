
#pragma once

#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/menu.h>
#include <gtkmm/gesturemultipress.h>
#ifndef USE_GTK4
#include <gtkmm/eventbox.h>
#endif
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

#ifdef USE_GTK4
	class ImageView : public Gtk::Box {
#else
	class ImageView : public Gtk::EventBox {
#endif
		public:
			ImageView(double display_scale, int logical_width);
			virtual ~ImageView();
			
			virtual bool on_motion_notify_event(GdkEventMotion *event) override;
			void on_save_as() const;

			/// Button handling goes through a gesture controller rather than the
			/// on_button_{press,release}_event vfuncs, which are removed in GTK4.
			/// Motion is still a vfunc: gtkmm-3 has no EventControllerMotion.
			void on_button_pressed(int n_press, double x, double y);
			void on_button_released(int n_press, double x, double y);
			
			void set_image_from_base64(const std::string& b64);
			void set_image_from_svg(const std::string& svg);

		private:
			ImageArea area;
			
			bool      sizing;
			double    prev_x, prev_y;
			int       height_at_press, width_at_press;

			Gtk::Menu     context_menu;
			Gtk::MenuItem item_save_as;

			Glib::RefPtr<Gtk::GestureMultiPress> click_gesture;
	};
	
};
