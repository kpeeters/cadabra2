
#include "TeXEdit.hh"
#include "../common/TeXEngine.hh"

using namespace cadabra;

TeXEdit::exp_input_tv::exp_input_tv(Glib::RefPtr<Gtk::TextBuffer> tb)
	: Gtk::TextView(tb), is_modified(false), folded_away(false)
	{
	}

TeXEdit::TeXEdit(Glib::RefPtr<Gtk::TextBuffer> tb, Glib::RefPtr<TeXBuffer> texb, const std::string& fontname)
	: edit(tb), texview(texb, 10)
	{
//	scroll_.set_size_request(-1,200);
//	scroll_.set_border_width(1);
//	scroll_.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
	edit.modify_font(Pango::FontDescription(fontname));
	edit.set_wrap_mode(Gtk::WRAP_WORD);
	edit.modify_text(Gtk::STATE_NORMAL, Gdk::Color("darkgray"));
	edit.set_pixels_above_lines(LINE_SPACING);
	edit.set_pixels_below_lines(LINE_SPACING);
	edit.set_pixels_inside_wrap(2*LINE_SPACING);
	edit.set_left_margin(10);

	set_spacing(10);

//	add(expander);
//	expander.set_label_widget(texview);
//	expander.add(edit);
//	expander.set_expanded();
	pack_start(edit);
	pack_start(texview);
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

bool TeXEdit::exp_input_tv::on_key_press_event(GdkEventKey* event)
	{
	if(get_editable() && event->keyval==GDK_Return && (event->state&Gdk::SHIFT_MASK)) {// shift-return
//		std::cerr << "activate!!" << std::endl;
		Glib::RefPtr<Gtk::TextBuffer> textbuf=get_buffer();
//		std::cerr << textbuf->get_text(textbuf->get_start_iter(), textbuf->get_end_iter()) << std::endl;
		std::string tmp(textbuf->get_text(get_buffer()->begin(), get_buffer()->end()));
#ifdef DEBUG
		std::cerr << "running: " << tmp << std::endl;
#endif
		emitter(tmp);
		is_modified=false;
//		set_editable(false);
//		textbuf->set_text("");
		return true;
		}
	else {
		is_modified=true;
		bool retval=Gtk::TextView::on_key_press_event(event);
		return retval;
		}
	}
