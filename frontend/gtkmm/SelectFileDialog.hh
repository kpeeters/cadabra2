#pragma once

#include <gtkmm.h>

class SelectFileDialog : public Gtk::Dialog {
	public:
		SelectFileDialog(const Glib::ustring& title, Gtk::Window& parent, bool modal = false);

		void set_text(const Glib::ustring& text);
		Glib::ustring get_text() const;
		Gtk::Entry& get_entry();

	protected:
		void choose_dialog();

		Gtk::Entry entry;
		Gtk::Button choose;
		Gtk::Box hbox;
	};
