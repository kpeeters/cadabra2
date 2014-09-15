
#pragma once

#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/label.h>
#include <gtkmm/stock.h>
#include <gtkmm/button.h>

namespace cadabra {

// Each notebook has one main window which controls it. It has a menu bar, a
// status pane and one or more panels that represent a view on the document.

   class NotebookWindow : public Gtk::Window {
      public:
         NotebookWindow();
         ~NotebookWindow();

         // Methods used to communicate information from Netbits to the GUI.
         void on_connect();

      private:
			Glib::RefPtr<Gtk::ActionGroup> actiongroup;

			Gtk::VBox                      topbox;
			Gtk::HBox                      supermainbox;
			Gtk::VBox                      mainbox;
			Gtk::HBox                      buttonbox;
			Gtk::HBox                      statusbarbox;

         // Buttons
         Gtk::Button                    b_kill, b_run, b_run_to, b_run_from, b_help, b_stop, b_undo, b_redo;


			// Status bar
			Gtk::ProgressBar               progressbar;
			Gtk::Label                     cdbstatus, kernelversion;

			// Name and modification data.
			void             update_title();
			std::string      name;
			bool             modified;
	};

};
