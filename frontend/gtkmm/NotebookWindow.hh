
#pragma once

#include <gtkmm/window.h>
#include <gtkmm/box.h>

namespace cadabra {

	// Each notebook has one main window which controls it. It has a menu bar, a
	// status pane and one or more panels that represent a view on the document.

	class NotebookWindow : public Gtk::Window {
		public:
			NotebookWindow();
			~NotebookWindow();

		private:
			Glib::RefPtr<Gtk::ActionGroup> actiongroup;

			Gtk::VBox                      topbox;
			Gtk::HBox                      supermainbox;
			Gtk::VBox                      mainbox;
			Gtk::HBox                      buttonbox;
			Gtk::HBox                      statusbarbox;

			// Name and modification data.
			void             update_title();
			std::string      name;
			bool             modified;
	};

};
