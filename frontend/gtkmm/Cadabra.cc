
#include "Cadabra.hh"
#include <signal.h>


// Signal handler for ctrl-C

cadabra::NotebookWindow *signal_window;

void signal_handler(int signal)
	{
	signal_window->close();
	}

Glib::RefPtr<Cadabra> Cadabra::create()
	{
	return Glib::RefPtr<Cadabra>( new Cadabra() );
	}

Cadabra::Cadabra()
	: compute(&nw, nw), compute_thread(&cadabra::ComputeThread::run, &compute)
	{
	// Connect the two threads.
	nw.set_compute_thread(&compute);
	
	// Setup ctrl-C handler so we can shut down gracefully (i.e. ask
	// for confirmation, shut down server).
	signal_window = &nw;
	signal(SIGINT, signal_handler);
	}

void Cadabra::on_activate()
	{
	add_window(nw);
	}

void Cadabra::on_open(const Gio::Application::type_vec_files& files, const Glib::ustring& hint)
	{
	nw.load_file(files[0]);
	Gtk::Application::on_open(files, hint);
	}
