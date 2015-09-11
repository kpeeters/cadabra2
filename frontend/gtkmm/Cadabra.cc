
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
	: Gtk::Application("com.phi-sci.cadabra.Cadabra", Gio::APPLICATION_HANDLES_OPEN | Gio::APPLICATION_NON_UNIQUE),
	  compute(&nw, nw), compute_thread(&cadabra::ComputeThread::run, &compute)
	{
	// Connect the two threads.
	nw.set_compute_thread(&compute);
	
	// Setup ctrl-C handler so we can shut down gracefully (i.e. ask
	// for confirmation, shut down server).
	signal_window = &nw;
	signal(SIGINT, signal_handler);
	}

Cadabra::~Cadabra()
	{
   // The app is going away; stop the compute logic and join that
	// thread waiting for it to complete.
	compute.terminate();
	compute_thread.join();
	}

void Cadabra::on_activate()
	{
	add_window(nw);
	nw.show();
	}

void Cadabra::on_open(const Gio::Application::type_vec_files& files, const Glib::ustring& hint)
	{
	on_activate();
	// Load the first file into a string.
	char* contents = nullptr;
	gsize length = 0;
	std::string text;
	try {
		if(files[0]->load_contents(contents, length)) {
			if(contents && length) {
				text=std::string(contents);
				}
			g_free(contents);
			}
		}
	catch (const Glib::Error& ex) {
		std::cerr << ex.what() << std::endl;
		return;
		}

	// Tell the window to open the notebook stored in the string.
	nw.load_file(text);
	Gtk::Application::on_open(files, hint);
	}

