
#include "Log.hh"
#include "ComputeThread.hh"
#include "NotebookWindow.hh"

#include <gtkmm/application.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/label.h>

#include <signal.h>

// Signal handler for ctrl-C

cadabra::NotebookWindow *signal_window;

void signal_handler(int signal)
	{
	signal_window->close();
	}

// Cadabra frontend with GTK+ interface (using gtkmm). 
// Makes use of the client classes in the client_server directory.

int main(int argc, char **argv)
	{
	try {
		// Create the ui material.
		Glib::RefPtr<Gtk::Application> app =
			Gtk::Application::create(argc, argv, "com.phi-sci.cadabra.cadabra-gtk");

		// Setup windows.
		cadabra::NotebookWindow nw;
		cadabra::ComputeThread compute(&nw, nw);
 
		// Create and start the compute/network thread.
		std::thread compute_thread(&cadabra::ComputeThread::run, &compute);
		
		// Connect the two threads.
		nw.set_compute_thread(&compute);

		// Setup ctrl-C handler so we can shut down gracefully (i.e. ask
		// for confirmation, shut down server).
		signal_window = &nw;
		signal(SIGINT, signal_handler);

		// Start the ui in the main thread.
		app->run(nw);

		// The window has been closed; stop the compute logic and join
		// that thread waiting for it to complete.
		compute.terminate();
		compute_thread.join();
		}
	catch(Glib::Error& er) {
		std::cerr << er.what() << std::endl;
		}
	}
