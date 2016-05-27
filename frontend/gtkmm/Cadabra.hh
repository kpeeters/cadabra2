
#pragma once

#include <gtkmm/application.h>
#include <gtkmm/grid.h>
#include "NotebookWindow.hh"
#include "ComputeThread.hh"

/// The Cadabra notebook application.

class Cadabra : public Gtk::Application {
	public:
		static Glib::RefPtr<Cadabra> create(int, char **);

		void open_help(const std::string&);

	protected:
		Cadabra(int, char**);
		virtual ~Cadabra();

		void on_activate() override;
		void on_open(const Gio::Application::type_vec_files& files, const Glib::ustring& hint) override;

	private:
		std::vector<cadabra::NotebookWindow *> windows;
		cadabra::ComputeThread                 compute;
		std::thread                            compute_thread;
};
