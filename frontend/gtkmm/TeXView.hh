
#pragma once

#include <gtkmm/eventbox.h>
#include <gtkmm/box.h>
#include <gtkmm/image.h>

#include "../common/TeXEngine.hh"

namespace cadabra {

	/// TeXView is a widget which knows how to turn a string into
	/// a LaTeX-rendered image and display that. 

	class TeXView : public Gtk::EventBox {
		public:
         TeXView(TeXEngine&, const std::string&, int hmargin=25);
			
			std::shared_ptr<TeXEngine::TeXRequest> content;
			
			Gtk::VBox                 vbox;
			Gtk::HBox                 hbox;
			Gtk::Image                image;
			
			// The actual image is stored in the pixbuf below. 
			// FIXME: This pointer is not yet shared among instances which show the
			// same content.

			Glib::RefPtr<Gdk::Pixbuf> pixbuf;
			
			// Update the visible image from the pixbuf. Call this in order to propagate
			// changes to the pixbuf (e.g. from re-running the TeXRequest) to the 
			// visible widget itself.

			void update_image();
			
			sigc::signal1<bool, std::string> tex_error;

		protected:
			virtual void on_show();

		private:
			TeXEngine& engine;
	};

}

