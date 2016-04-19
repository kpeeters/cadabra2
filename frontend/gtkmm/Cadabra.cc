
#include "Cadabra.hh"
#include <signal.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/entry.h>
#if GTKMM_MINOR_VERSION < 10
#include <gtkmm/main.h>
#endif

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
	: Gtk::Application(argc, argv, "com.phi-sci.cadabra.Cadabra", Gio::APPLICATION_HANDLES_OPEN | Gio::APPLICATION_NON_UNIQUE),
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

	if(!nw.is_registered()) {
		Gtk::Dialog md("Welcome to Cadabra!", nw, Gtk::MESSAGE_WARNING);
		md.set_transient_for(nw);
		md.set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
		Gtk::Box *box = md.get_content_area();
		Gtk::Label txt;
		txt.set_text("Writing this software takes an incredible amount of spare time.\nPlease help guarantee future development by registering your email address,\nso I can convince the bean-counters that this software is of interest.\n\nI will only use this address to email you, roughly once\nevery half a year, with a bit of news about Cadabra.\n\nMany thanks for your support!");
		txt.set_line_wrap();
		txt.set_margin_top(10);
		txt.set_margin_left(10);
		txt.set_margin_right(10);
		txt.set_margin_bottom(10);
		box->pack_start(txt, Gtk::PACK_EXPAND_WIDGET);
		Gtk::HBox  email_box;
		Gtk::Label email_label("Email address:");
		Gtk::Entry email;
		box->pack_start(email_box, Gtk::PACK_EXPAND_WIDGET, 10);
		email_box.pack_start(email_label, Gtk::PACK_SHRINK, 15);
		email_box.pack_end(email, Gtk::PACK_EXPAND_WIDGET, 10);
		Gtk::HBox hbox;
		box->pack_end(hbox, Gtk::PACK_SHRINK);
		Gtk::Button reg("Register"), nothanks("No thanks");
		hbox.pack_end(reg, Gtk::PACK_SHRINK, 10);
		hbox.pack_start(nothanks, Gtk::PACK_SHRINK,10);
		reg.signal_clicked().connect([&]() {
				nw.set_email(email.get_text());
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
	nw.set_name(files[0]->get_path());
	nw.load_file(text);
	Gtk::Application::on_open(files, hint);
	}

