
#include "Client.hh"
#include "NotebookWindow.hh"
#include <gtkmm/application.h>

// Cadabra frontend with GTK+ interface (using gtkmm). 
// Makes use of the client classes in the client_server directory.

// A network client which knows how to propagate the Client callback
// functions to the NotebookWindow class so that the Gtk interface
// gets updated appropriately.
// This class runs in its own thread, calls methods in NotebookWindow
// to queue updates, and then calls on the dispatcher to make that
// NotebookWindow thread wake up.

class GtkClient : public cadabra::Client {
	public:
		GtkClient(cadabra::NotebookWindow&w) : nbw(w) {};

		virtual void on_connect() {};
		virtual void on_disconnect() {};
		virtual void on_network_error() {};
		virtual void on_progress() {};

		virtual void before_tree_change(cadabra::Client::ActionBase&) {};
		virtual void after_tree_change(cadabra::Client::ActionBase&) {};
		
	private:
		cadabra::NotebookWindow& nbw;
};

int main(int argc, char **argv)
	{
	// Create the ui material.
	Glib::RefPtr<Gtk::Application> app =
		Gtk::Application::create(argc, argv, "com.phi-sci.cadabra.cadabra-gtk");
  	cadabra::NotebookWindow nw;

	// Create and start the network thread.
	GtkClient  cdb(nw);
	std::thread client_thread(&GtkClient::run, std::ref(cdb));

	// Start the ui in the main thread.
	app->run(nw);
	client_thread.join();
	}
