
#pragma once

#include <gtkmm/box.h>
#include <gtkmm/image.h>

namespace cadabra {

   /// \ingroup gtkmm
   ///
   /// An image viewing widget.

	class ImageView : public Gtk::VBox {
		public:
			ImageView();
			virtual ~ImageView();

			void set_image_from_base64(const std::string& b64);

		private:
			Gtk::Image image;			
	};

};
