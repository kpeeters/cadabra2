
#include "Cadabra.hh"
#include <signal.h>
#include <fstream>
#include <gtkmm/messagedialog.h>
#include <gtkmm/entry.h>
#if GTKMM_MINOR_VERSION < 10
#include <gtkmm/main.h>
#endif
#include <gtkmm/settings.h>
#include "Snoop.hh"
#include "Config.hh"

// Signal handler for ctrl-C

cadabra::NotebookWindow *signal_window;

void signal_handler(int signal)
	{
#if GTKMM_MINOR_VERSION >= 10
	signal_window->close();
#else
	Gtk::Main::quit();
#endif
	}

Glib::RefPtr<Cadabra> Cadabra::create(int argc, char **argv)
	{
	return Glib::RefPtr<Cadabra>( new Cadabra(argc, argv) );
	}

Cadabra::Cadabra(int argc, char **argv)
	: Gtk::Application(argc, argv, "com.phi-sci.cadabra.Cadabra", 
							 Gio::APPLICATION_HANDLES_OPEN | Gio::APPLICATION_NON_UNIQUE),
	  compute_thread(&cadabra::ComputeThread::run, &compute)
	{
	// https://stackoverflow.com/questions/43886686/how-does-one-make-gtk3-look-native-on-windows-7
	//	https://github.com/shoes/shoes3/wiki/Changing-Gtk-theme-on-Windows	
#if defined(_WIN32)
	// Gtk::Settings::get_default()->property_gtk_theme_name()="win32";
#endif
	}

Cadabra::~Cadabra()
	{
   // The app is going away; stop the compute logic and join that
	// thread waiting for it to complete.
	compute.terminate();
	compute_thread.join();

//	for(auto w: windows)
//		delete w;
	}

void Cadabra::on_activate()
	{
	auto nw = new cadabra::NotebookWindow(this);
	compute.set_master(nw, nw);

	// Connect the two threads.
	nw->set_compute_thread(&compute);
	
	// Setup ctrl-C handler so we can shut down gracefully (i.e. ask
	// for confirmation, shut down server).
	signal_window = nw;
	signal(SIGINT, signal_handler);

   add_window(*nw);
	nw->show();

	std::string version=std::string(CADABRA_VERSION_MAJOR)+"."+CADABRA_VERSION_MINOR+"."+CADABRA_VERSION_PATCH;	
	snoop::log("start") << version << snoop::flush;
	
	if(!nw->prefs.is_registered && !nw->prefs.is_anonymous) {
		nw->on_help_register();
		}
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
	auto wins = get_windows();
	auto nw = static_cast<cadabra::NotebookWindow *>(wins[0]);
	nw->set_name(files[0]->get_path());
	nw->load_file(text);
	Gtk::Application::on_open(files, hint);
	}

bool Cadabra::open_help(const std::string& nm, const std::string& title) 
	{
	std::ifstream fl(nm);
	if(fl) {
		auto nw = new cadabra::NotebookWindow(this, true);
		nw->set_title_prefix("Cadabra help for ");
		nw->set_name(title);
		add_window(*nw);
		std::stringstream buffer;
		buffer << fl.rdbuf();
		nw->load_file(buffer.str());
		nw->show();
		return true;
		}
	else return false;
	}
