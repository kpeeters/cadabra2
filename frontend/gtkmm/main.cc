
#include "Cadabra.hh"

#include <gtkmm/application.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/label.h>
#include <glibmm/spawn.h>
#include <glibmm/init.h>

#ifdef _WIN32
#include <Windows.h>
#include <stdlib.h>
#include <fstream>
#include <streambuf>
#endif

#include <iostream>

#define NDEBUG 1

// Cadabra frontend with GTK+ interface (using gtkmm).
// Makes use of the client classes in the client_server directory.

int main(int argc, char **argv)
	{
#ifdef _WIN32
   // We are going to disable console creation. However,
	// that also kills any output to stdout or stderr, which may
	// have been useful in case of missing dlls or other errors.
	// Write them to a log file.
	std::ofstream logFile("C:\\Windows\\Temp\\cadabra_log.txt");
	std::streambuf* oldCout = std::cout.rdbuf(logFile.rdbuf());
	std::streambuf* oldCerr = std::cerr.rdbuf(logFile.rdbuf());
	
	FreeConsole();
#endif
	
	// glibmm-2.68 changed Glib::init() -- which Gtk::Application's constructor
	// calls -- to set the C *and C++* global locale to the user's preferred one
	// by default; glibmm-2.4 never did. That makes every std::ostringstream
	// group digits, so a port number serialises as "43,559" and mathematical
	// output and notebook serialisation would pick up separators too. Cadabra
	// wants the classic locale for all of that, so opt out.
#if GLIBMM_MAJOR_VERSION > 2 || (GLIBMM_MAJOR_VERSION == 2 && GLIBMM_MINOR_VERSION >= 68)
	Glib::set_init_to_users_preferred_locale(false);
#endif

	try {
		auto application = Cadabra::create();
		const int status = application->run(argc, argv);
		return status;
		}
	catch (Glib::Error& er) {
		std::cerr << er.what() << std::endl;
		return -1;
		}
	catch (std::exception& ex) {
		std::cerr << ex.what() << std::endl;
		}

#ifdef _WIN32
	std::cout.rdbuf(oldCout);
	std::cerr.rdbuf(oldCerr);
#endif
	}

// #if defined(_WIN32) && defined(NDEBUG)
// 
// void display_error(const std::string& message)
// {
// 	MessageBoxA(NULL, message.c_str(), "Error!", MB_ICONEXCLAMATION | MB_OK);
// }
// 
// int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
// 	{
// //	if (getenv("CADABRA_LOG_OUTPUT") != NULL) {
// 		auto stdout_path = Glib::get_user_data_dir() + "/cadabra2-stdout.log";
// 		if (freopen(stdout_path.c_str(), "w", stdout) == NULL) {
// 			display_error("Could not redirect stdout to " + stdout_path);
// 			exit(1);
// 		}
// 
// 		auto stderr_path = Glib::get_user_data_dir() + "/cadabra2-stderr.log";
// 		if (freopen(stderr_path.c_str(), "w", stderr) == NULL) {
// 			display_error("Could not redirect stdout to " + stderr_path);
// 			exit(1);
// 		}
// 
// 		std::ios_base::sync_with_stdio();
// //	}
// 
// 	try {
// 		auto application = Cadabra::create();
// 		const int status = application->run(__argc, __argv);
// 		return status;
// 		}
// 	catch (Glib::Error& er) {
// 		display_error(er.what());
// 		return -1;
// 		}
// 	catch (std::exception& ex) {
// 		display_error(ex.what());
// 		return -1;
// 		}
// 	catch (...) {
// 		display_error("An unknown error occurred");
// 		return -1;
// 		}
// 	}
// #endif
// 
