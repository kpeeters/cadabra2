
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

#include "Client.hh"
#include "GUIBase.hh"

namespace cadabra {

// Each notebook has one main window which controls it. It has a menu bar, a
// status pane and one or more panels that represent a view on the document.

   class NotebookWindow : public Gtk::Window, public GUIBase {
      public:
         NotebookWindow();
         ~NotebookWindow();
        
         // Let the notebook know about the client so that it can access
         // the document and its functions. Notebook does not own this pointer.

         void set_client(Client *);

         // Methods used to communicate information from Client to the GUI.
			// These get called on the Client thread, which is _not_ the GUI
			// thread, so they store the information (FIXME: where) and then
			// signal the GUI thread through the use of the dispatcher (below).
			// All actual GUI updates take place in the on_client_notification
			// member, on the GUI thread.

         virtual void on_connect() override;
         virtual void on_disconnect() override;
         virtual void on_network_error() override;
			virtual void new_todo_notification() override;

      private:
         Client *client;

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

         // Buttons
         Gtk::Button                    b_kill, b_run, b_run_to, b_run_from, b_help, b_stop, b_undo, b_redo;


			// Status bar
			Gtk::ProgressBar               progressbar;
			Gtk::Label                     cdbstatus, kernelversion;

			// Name and modification data.
			void             update_title();
			std::string      name;
			bool             modified;

			// Todo deque processing logic.
			void process_todo_queue();
	};

};
