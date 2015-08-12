
#include "TeXEdit.hh"
#include "../common/TeXEngine.hh"

using namespace cadabra;

TeXEdit::exp_input_tv::exp_input_tv(DTree::iterator it, Glib::RefPtr<Gtk::TextBuffer> tb)
	: Gtk::TextView(tb), datacell(it), is_modified(false), folded_away(false)
	{
	}

TeXEdit::TeXEdit(DTree::iterator it, Glib::RefPtr<Gtk::TextBuffer> textbuf, TeXEngine& engine)
	: buffer(textbuf), edit(it, textbuf), texview(engine, "")
	{
	init();
	}

void TeXEdit::init()
	{
	edit.override_font(Pango::FontDescription("monospace")); 
	edit.set_wrap_mode(Gtk::WRAP_NONE);
	edit.set_pixels_above_lines(1);
	edit.set_pixels_below_lines(1);
	edit.set_pixels_inside_wrap(1);
	set_margin_top(10);
	set_margin_bottom(10);
	edit.set_left_margin(20);
	edit.set_accepts_tab(true);
	Pango::TabArray tabs(10);
	// FIXME: use character width measured, instead of '8', or at least
	// understand how Pango units are supposed to work.
	for(int i=0; i<10; ++i) 
		tabs.set_tab(i, Pango::TAB_LEFT, 4*8*i);
	edit.set_tabs(tabs);

	pack_start(edit);
	pack_start(texview);
	}

TeXEdit::TeXEdit(DTree::iterator it, const std::string& str, TeXEngine& engine)
	: buffer(Gtk::TextBuffer::create()), edit(it, buffer), texview(engine, "")
	{
	buffer->set_text(str);
	init();
	}

bool TeXEdit::is_folded() const
	{
	return edit.folded_away;
	}

void TeXEdit::set_folded(bool onoff)
	{
	if(edit.folded_away==onoff) return;

	edit.folded_away=onoff;
	if(edit.folded_away) 
		remove(edit);
	else {
		pack_start(edit);
		reorder_child(edit, 0);
		edit.show();
		}
	}

bool TeXEdit::exp_input_tv::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
	{
	bool ret=Gtk::TextView::on_draw(cr);
	return ret;
	}

bool TeXEdit::exp_input_tv::on_key_press_event(GdkEventKey* event)
	{
	return true;
	}

bool TeXEdit::exp_input_tv::on_focus_in_event(GdkEventFocus *event) 
	{
	cell_got_focus(datacell);
	return Gtk::TextView::on_focus_in_event(event);
	}
