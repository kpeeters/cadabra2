
#include "ComputeThread.hh"
#include "NotebookWindow.hh"

#include <gtkmm/application.h>

// Cadabra frontend with GTK+ interface (using gtkmm). 
// Makes use of the client classes in the client_server directory.


int main(int argc, char **argv)
	{
	// Create the ui material.
	Glib::RefPtr<Gtk::Application> app =
		Gtk::Application::create(argc, argv, "com.phi-sci.cadabra.cadabra-gtk");
  	cadabra::NotebookWindow nw;

	// Create and start the compute/network thread.
	cadabra::ComputeThread compute(&nw);
	std::thread compute_thread(&cadabra::ComputeThread::run, std::ref(compute));

	nw.set_compute_thread(&compute);
	
	// Start the ui in the main thread.
	app->run(nw);
	compute_thread.join();
	}
