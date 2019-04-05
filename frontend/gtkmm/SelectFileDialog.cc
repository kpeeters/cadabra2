#include "SelectFileDialog.hh"

SelectFileDialog::SelectFileDialog(const Glib::ustring& title, Gtk::Window& parent, bool modal)
	: Gtk::Dialog(title, parent, modal)
	, choose("...")
{
	set_resizable(false);
	entry.set_width_chars(60);

	choose.signal_clicked().connect(sigc::mem_fun(*this, &SelectFileDialog::choose_dialog));
	choose.set_hexpand(false);

	hbox.pack_start(entry);
	hbox.pack_end(choose);

	get_content_area()->pack_start(hbox);
	add_button("Ok", Gtk::RESPONSE_OK);

	show_all();
}

void SelectFileDialog::set_text(const Glib::ustring& text)
{
	entry.set_text(text);
}

Glib::ustring SelectFileDialog::get_text() const
{
	return entry.get_text();
}

Gtk::Entry& SelectFileDialog::get_entry()
{
	return entry;
}

void SelectFileDialog::choose_dialog()
{
	Gtk::FileChooserDialog fc("Select a file...");
	fc.set_transient_for(*this);
	fc.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	fc.add_button("Select", Gtk::RESPONSE_OK);

	if (fc.run() == Gtk::RESPONSE_OK)
		entry.set_text(fc.get_filename());
}