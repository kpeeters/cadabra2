
#include "Cadabra.hh"

#include <gtkmm/application.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/label.h>
#include <glibmm/spawn.h>

#ifdef _WIN32
#include <Windows.h>
#include <stdlib.h>
#endif

// Cadabra frontend with GTK+ interface (using gtkmm).
// Makes use of the client classes in the client_server directory.

int main(int argc, char **argv)
	{
	try {
		auto application = Cadabra::create(argc, argv);
		const int status = application->run();
		return status;
		}
	catch (Glib::Error& er) {
		std::cerr << er.what() << std::endl;
		return -1;
		}
	catch (std::exception& ex) {
		std::cerr << ex.what() << std::endl;
		}
	}

#if defined(_WIN32) && defined(NDEBUG)
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
	{
	try {
		auto application = Cadabra::create(__argc, __argv);
		const int status = application->run();
		return status;
		}
	catch (Glib::Error& er) {
		Gtk::MessageDialog(er.what()).run();
		return -1;
		}
	catch (std::exception& ex) {
		Gtk::MessageDialog(ex.what()).run();
		return -1;
		}
	catch (...) {
		Gtk::MessageDialog("An unknown error occured :(").run();
		throw;
		}
	}
#endif

