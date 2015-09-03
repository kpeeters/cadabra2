
#pragma once

#include <gtkmm/application.h>
#include "NotebookWindow.hh"
#include "ComputeThread.hh"

/// The Cadabra notebook application.

class Cadabra : public Gtk::Application {
	public:
		static Glib::RefPtr<Cadabra> create();

	protected:
		Cadabra();
		virtual ~Cadabra();

		void on_activate() override;
		void on_open(const Gio::Application::type_vec_files& files, const Glib::ustring& hint) override;

	private:
		cadabra::NotebookWindow nw;
		cadabra::ComputeThread  compute;
		std::thread             compute_thread;
};
