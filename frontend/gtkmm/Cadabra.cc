
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
#if defined(_WIN32)
	Gtk::Settings::get_default()->property_gtk_theme_name()="win32";
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
	
	if(!nw->is_registered()) {
		Gtk::Dialog md("Welcome to Cadabra!", *nw, Gtk::MESSAGE_WARNING);
		md.set_transient_for(*nw);
		md.set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
		Gtk::Box *box = md.get_content_area();
		Gtk::Label txt;
		txt.set_markup("<span font_size=\"large\" font_weight=\"bold\">Welcome to Cadabra!</span>\n\nWriting this software takes an incredible amount of spare time,\nand it is extremely difficult to get funding for its development.\n\nPlease show your support by registering your email address,\nso I can convince the bean-counters that this software is of interest.\n\nI will only use this address to count users and to email you,\nroughly once every half a year, with a bit of news about Cadabra.\n\nMany thanks for your support!\n\nKasper Peeters, <a href=\"mailto:info@cadabra.science\">info@cadabra.science</a>");
		txt.set_line_wrap();
		txt.set_margin_top(10);
		txt.set_margin_left(10);
		txt.set_margin_right(10);
		txt.set_margin_bottom(10);
		box->pack_start(txt, Gtk::PACK_EXPAND_WIDGET);

		Gtk::Grid grid;
		grid.set_column_homogeneous(false);
		grid.set_hexpand(true);
		grid.set_margin_left(10);
		grid.set_margin_right(10);
		box->pack_start(grid, Gtk::PACK_EXPAND_WIDGET);

		Gtk::Label name_label("Name:");
		Gtk::Entry name;
		name_label.set_alignment(0, 0.5);
		name.set_hexpand(true);
		grid.attach(name_label,  0,0, 1,1);
		grid.attach(name,        1,0, 1,1);
		Gtk::Label email_label("Email address:");
		email_label.set_alignment(0, 0.5);
		Gtk::Entry email;
		email.set_hexpand(true);
		grid.attach(email_label, 0,1, 1,1);
		grid.attach(email,       1,1, 1,1);
		Gtk::Label affiliation_label("Affiliation:");
		Gtk::Entry affiliation;
		affiliation_label.set_alignment(0, 0.5);
		affiliation.set_hexpand(true);
		grid.attach(affiliation_label, 0,2, 1,1);
		grid.attach(affiliation,       1,2, 1,1);

		Gtk::HBox hbox;
		box->pack_end(hbox, Gtk::PACK_SHRINK);
		Gtk::Button reg("Register my support"), nothanks("I prefer to stay anonymous");
		hbox.pack_end(reg, Gtk::PACK_SHRINK, 10);
		hbox.pack_start(nothanks, Gtk::PACK_SHRINK,10);
		reg.signal_clicked().connect([&]() {
				nw->set_user_details(name.get_text(), email.get_text(), affiliation.get_text());
				md.hide();
				});
		nothanks.signal_clicked().connect([&]() {
				md.hide();
				});
		box->show_all();
		md.run();
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
