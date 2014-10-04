
#pragma once

#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/label.h>
#include <gtkmm/stock.h>
#include <gtkmm/button.h>
#include <glibmm/dispatcher.h>

#include <thread>
#include <mutex>

#include "DocumentThread.hh"
#include "ComputeThread.hh"
#include "GUIBase.hh"
#include "NotebookCanvas.hh"
#include "../common/TeXEngine.hh"

namespace cadabra {

   // Each notebook has one main window which controls it. It has a menu bar, a
   // status pane and one or more panels that represent a view on the document.

   class NotebookWindow : public Gtk::Window, public DocumentThread, public GUIBase {
      public:
         NotebookWindow();
         ~NotebookWindow();
        
         // Virtual functions from GUIBase.

         virtual void add_cell(DTree::iterator) override;
         virtual void remove_cell(DTree::iterator) override;
         virtual void update_cell(DTree::iterator) override;

         virtual void on_connect() override;
         virtual void on_disconnect() override;
         virtual void on_network_error() override;

			virtual void process_data() override;

      private:

			// Main handler which fires whenever the Client object signals 
			// that the document is changing or the network status is modified.
			// Runs on the GUI thread.

			Glib::Dispatcher dispatcher;

			// GUI elements.
			
			Glib::RefPtr<Gtk::ActionGroup> actiongroup;

			Gtk::VBox                      topbox;
			Gtk::HBox                      supermainbox;
			Gtk::VBox                      mainbox;
			Gtk::HBox                      buttonbox;
			Gtk::HBox                      statusbarbox;

			// All canvasses which are stored in the ...
			// These pointers are managed by gtkmm.
			std::vector<NotebookCanvas *>  canvasses;

         // Buttons
         Gtk::Button                    b_kill, b_run, b_run_to, b_run_from, b_help, b_stop, b_undo, b_redo;

			// Status bar
			Gtk::ProgressBar               progressbar;
			Gtk::Label                     status_label, kernel_label;

			// GUI data which is the autoritative source for things displayed in
			// the status bars declared above. These strings are filled on the
			// compute thread and then updated into the gui on the gui thread.

			std::mutex                     status_mutex;
			std::string                    status_string, kernel_string;

			// Name and modification data.
			void             update_title();
			std::string      name;
			bool             modified;

			// Todo deque processing logic.
			void process_todo_queue();

			// TeX stuff
			TeXEngine        engine;
	};

};
