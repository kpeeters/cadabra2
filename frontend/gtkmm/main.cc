
#include "Client.hh"
#include "NotebookWindow.hh"
#include <gtkmm/main.h>

// Cadabra frontend with GTK+ interface (using gtkmm). 
// Makes use of the client classes in the client_server directory.

class CadabraGtk : public cadabra::Client {
	public:
		virtual void on_connect() {};
		virtual void on_disconnect() {};
		virtual void on_network_error() {};
		virtual void on_progress() {};

		virtual void before_tree_change(cadabra::Client::ActionBase&) {};
		virtual void after_tree_change(cadabra::Client::ActionBase&) {};
};

int main(int argc, char **argv)
	{
	CadabraGtk cdb;
	Gtk::Main  kit(&argc, &argv);

	std::thread client_thread(&CadabraGtk::run, std::ref(cdb));
	cadabra::NotebookWindow nw;
	
	Gtk::Main::run();
	client_thread.join();
	}
