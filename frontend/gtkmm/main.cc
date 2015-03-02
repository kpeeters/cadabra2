
#include "ComputeThread.hh"
#include "NotebookWindow.hh"

#include <gtkmm/application.h>

// Cadabra frontend with GTK+ interface (using gtkmm). 
// Makes use of the client classes in the client_server directory.

int main(int argc, char **argv)
	{
	try {
		// Create the ui material.
		Glib::RefPtr<Gtk::Application> app =
			Gtk::Application::create(argc, argv, "com.phi-sci.cadabra.cadabra-gtk");
		cadabra::NotebookWindow nw;
		cadabra::ComputeThread compute(&nw, nw);
		
		// Create and start the compute/network thread.
		std::thread compute_thread(&cadabra::ComputeThread::run, &compute);
		
		// Connect the two threads.
		nw.set_compute_thread(&compute);
		
		// Start the ui in the main thread.
		app->run(nw);
		compute_thread.join();
		}
	catch(Glib::Error& er) {
		std::cerr << er.what() << std::endl;
		}
	}
