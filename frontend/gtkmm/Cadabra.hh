
#pragma once

#include <gtkmm/application.h>
#include <gtkmm/grid.h>
#include "NotebookWindow.hh"
#include "ComputeThread.hh"

/// The Cadabra notebook application.

class Cadabra : public Gtk::Application {
	public:
		static Glib::RefPtr<Cadabra> create(int, char **);

		bool open_help(const std::string& filename, const std::string& title);

	protected:
		Cadabra(int, char**);
		virtual ~Cadabra();

		virtual void on_activate() override;
		virtual void on_open(const Gio::Application::type_vec_files& files, const Glib::ustring& hint) override;

		int on_handle_local_options(const Glib::RefPtr<Glib::VariantDict> &);

	private:
		cadabra::ComputeThread                *compute;
		std::thread                           *compute_thread;

		int server_port;
};
