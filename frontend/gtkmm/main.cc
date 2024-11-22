
#include "Cadabra.hh"

#include <gtkmm/application.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/label.h>
#include <glibmm/spawn.h>

#ifdef _WIN32
#include <Windows.h>
#include <stdlib.h>
#endif

#define NDEBUG 1

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

void display_error(const std::string& message)
{
	MessageBoxA(NULL, message.c_str(), "Error!", MB_ICONEXCLAMATION | MB_OK);
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
	{
	if (getenv("CADABRA_LOG_OUTPUT") != NULL) {
		auto stdout_path = Glib::get_user_data_dir() + "/cadabra2-stdout.log";
		if (freopen(stdout_path.c_str(), "w", stdout) == NULL) {
			display_error("Could not redirect stdout to " + stdout_path);
			exit(1);
		}

		auto stderr_path = Glib::get_user_data_dir() + "/cadabra2-stderr.log";
		if (freopen(stderr_path.c_str(), "w", stderr) == NULL) {
			display_error("Could not redirect stdout to " + stderr_path);
			exit(1);
		}

		std::ios_base::sync_with_stdio();
	}

	try {
		auto application = Cadabra::create(__argc, __argv);
		const int status = application->run();
		return status;
		}
	catch (Glib::Error& er) {
		display_error(er.what());
		return -1;
		}
	catch (std::exception& ex) {
		display_error(ex.what());
		return -1;
		}
	catch (...) {
		display_error("An unknown error occurred");
		return -1;
		}
	}
#endif

