
#include "Cadabra.hh"
#include <signal.h>
#include <fstream>
#include <gtkmm/messagedialog.h>
#include <gtkmm/entry.h>
#if GTKMM_MINOR_VERSION < 10
#include <gtkmm/main.h>
#endif
#include <gtkmm/settings.h>
#include <giomm.h>
#include "Snoop.hh"
#include "Config.hh"

// Signal handler for ctrl-C

cadabra::NotebookWindow *signal_window;

void signal_handler(int)
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
	                   Gio::APPLICATION_HANDLES_OPEN |
	                   Gio::APPLICATION_NON_UNIQUE),
	  compute(0), compute_thread(0),
	  server_port(0)
	{
	// https://stackoverflow.com/questions/43886686/how-does-one-make-gtk3-look-native-on-windows-7
	//	https://github.com/shoes/shoes3/wiki/Changing-Gtk-theme-on-Windows
#if defined(_WIN32)
	// Gtk::Settings::get_default()->property_gtk_theme_name()="win32";
#endif

	//https://github.com/GNOME/gtkmm-documentation/blob/master/examples/book/application/command_line_handling/exampleapplication.cc

	signal_handle_local_options().connect(
	   sigc::mem_fun(*this, &Cadabra::on_handle_local_options), false);

	add_main_option_entry(Gio::Application::OptionType::OPTION_TYPE_INT,
	                      "server-port",
	                      's',
	                      "Connect to running server on given port.",
	                      "number");
	add_main_option_entry(Gio::Application::OptionType::OPTION_TYPE_FILENAME,
	                      "token",
	                      't',
	                      "Use the given authentication token to connect to the server.",
	                      "string");
	}

template <typename T_ArgType>
static bool get_arg_value(const Glib::RefPtr<Glib::VariantDict>& options, const Glib::ustring& arg_name, T_ArgType& arg_value)
	{
	arg_value = T_ArgType();
	if(options->lookup_value(arg_name, arg_value)) return true;
	else return false;
	}

Cadabra::~Cadabra()
	{
	// The app is going away; stop the compute logic and join that
	// thread waiting for it to complete.

	if(compute)
		compute->terminate();
	if(compute_thread)
		compute_thread->join();
	if(compute)
		delete compute;
	if(compute_thread)
		delete compute_thread;

	//	for(auto w: windows)
	//		delete w;
	}


int Cadabra::on_handle_local_options(const Glib::RefPtr<Glib::VariantDict>& options)
	{
	if(!options)
		return -1;

	get_arg_value(options, "server-port", server_port);
	if(get_arg_value(options, "token",       server_token)==false)
		std::cerr << "no token" << std::endl;
	std::cerr << server_port << ", " << server_token << std::endl;
	return -1;
	}

void Cadabra::on_activate()
	{
	compute = new cadabra::ComputeThread(server_port, server_token);
	compute_thread = new std::thread(&cadabra::ComputeThread::run, compute);

	auto nw = new cadabra::NotebookWindow(this);
	compute->set_master(nw, nw);

	// Connect the two threads.
	nw->set_compute_thread(compute);

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
#ifdef DEBUG
	std::cerr << "Opening help file " << nm << std::endl;
#endif
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
