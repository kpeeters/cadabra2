
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

		void on_activate() override;
		void on_open(const Gio::Application::type_vec_files& files, const Glib::ustring& hint) override;

	private:
		cadabra::ComputeThread                 compute;
		std::thread                            compute_thread;
};
