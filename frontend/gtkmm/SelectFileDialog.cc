#include "SelectFileDialog.hh"
#include "GtkCompat.hh"

SelectFileDialog::SelectFileDialog(const Glib::ustring& title, Gtk::Window& parent, bool modal)
	: Gtk::Dialog(title, parent, modal)
	, choose("...")
	, hbox(cdb::ORIENT_HORIZONTAL)
	{
	set_resizable(false);
	entry.set_width_chars(60);

	choose.signal_clicked().connect(sigc::mem_fun(*this, &SelectFileDialog::choose_dialog));
	choose.set_hexpand(false);

	// 'entry' expands, so packing 'choose' after it puts it hard right, which is
	// what pack_end used to do. GTK4 has no pack_end.
	entry.set_hexpand(true);
	cdb::box_append(hbox, entry, cdb::Expand::yes);
	cdb::box_append(hbox, choose);

	cdb::box_append(*get_content_area(), hbox);
	add_button("Ok", cdb::RESPONSE_OK);

	cdb::show_all(*this);
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
	fc.add_button("Cancel", cdb::RESPONSE_CANCEL);
	fc.add_button("Select", cdb::RESPONSE_OK);

	if (cdb::dialog_run(fc) == cdb::RESPONSE_OK)
		entry.set_text(fc.get_file()->get_path());
	}
