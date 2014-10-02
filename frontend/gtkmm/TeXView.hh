
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
			
			void update_image();
			
		protected:
			virtual void on_show();
	};

}

