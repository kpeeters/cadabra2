
#pragma once

#include <gtkmm/eventbox.h>
#include <gtkmm/box.h>
#include <gtkmm/image.h>

#include "../common/TeXEngine.hh"

namespace cadabra {

	// TeXView is a widget which knows how to turn a string into
	// a LaTeX-rendered image and display that. 

	class TeXView : public Gtk::EventBox {
		public:
         TeXView(TeXEngine&, const std::string&, int hmargin=25);
			
			TeXEngine::TeXRequest    *content;
			
			Gtk::VBox                 vbox;
			Gtk::HBox                 hbox;
			Gtk::Image                image;
			
			// The actual image is stored in the pixbuf below. This pointer is shared
			// among
			Glib::RefPtr<Gdk::Pixbuf> pixbuf;
			
			void update_image();
			
			sigc::signal1<bool, std::string> tex_error;

		protected:
			virtual void on_show();

		private:
			TeXEngine& engine;
	};

}

