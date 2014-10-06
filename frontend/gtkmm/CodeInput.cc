
#include "CodeInput.hh"
#include <gdkmm/general.h>
#include <gtkmm/messagedialog.h>
#include <gdk/gdkkeysyms.h>

using namespace cadabra;

// General tool to strip spaces from both ends
std::string trim(const std::string& s) 
	{
	if(s.length() == 0)
		return s;
	int b = s.find_first_not_of(" \t\n");
	int e = s.find_last_not_of(" \t\n");
	if(b == -1) // No non-spaces
		return "";
	return std::string(s, b, e - b + 1);
	}

CodeInput::exp_input_tv::exp_input_tv(Glib::RefPtr<Gtk::TextBuffer> tb)
	: Gtk::TextView(tb)
	{
//	get_buffer()->signal_insert().connect(sigc::mem_fun(this, &exp_input_tv::on_my_insert), false);
//	get_buffer()->signal_erase().connect(sigc::mem_fun(this, &exp_input_tv::on_my_erase), false);
	}

CodeInput::CodeInput(Glib::RefPtr<Gtk::TextBuffer> tb, const std::string& fontname, int hmargin)
	: edit(tb)
	{
//	scroll_.set_size_request(-1,200);
//	scroll_.set_border_width(1);
//	scroll_.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
	edit.override_font(Pango::FontDescription(fontname)); 
	edit.set_wrap_mode(Gtk::WRAP_NONE);
	edit.override_color(Gdk::RGBA("blue"));
//	edit.set_pixels_above_lines(Gtk::LINE_SPACING);
//	edit.set_pixels_below_lines(Gtk::LINE_SPACING);
//	edit.set_pixels_inside_wrap(2*Gtk::LINE_SPACING);
	edit.set_left_margin(hmargin);
	edit.set_accepts_tab(false);

	edit.signal_button_press_event().connect(sigc::mem_fun(this, 
																				&CodeInput::handle_button_press), 
															 false);
//	edit.get_buffer()->signal_changed().connect(sigc::mem_fun(this, &CodeInput::handle_changed));


//	add(hbox);
//	hbox.add(vsep);
//	hbox.add(edit);
	add(edit);
//	set_border_width(3);
	}

bool CodeInput::exp_input_tv::on_key_press_event(GdkEventKey* event)
	{
//	std::cerr << event->keyval << ", " << event->state << " pressed" << std::endl;
	if(get_editable() && event->keyval==GDK_KEY_Return && (event->state&Gdk::SHIFT_MASK)) {// shift-return
		Glib::RefPtr<Gtk::TextBuffer> textbuf=get_buffer();
		std::string tmp(trim(textbuf->get_text(get_buffer()->begin(), get_buffer()->end())));
		// Determine whether this is a valid input cell: should end either on a delimiter or
		// on a delimeter-space-quoted-file-name combination.
		bool is_ok=false;
		if(tmp.size()>0) {
			 if(tmp[0]!='#' && tmp[tmp.size()-1]!=';' && tmp[tmp.size()-1]!=':' && tmp[tmp.size()-1]!='.' 
				 && tmp[tmp.size()-1]!='\"') {
				  is_ok=false;
				  }
			 else is_ok=true;
			 }
		if(!is_ok) {
			 Gtk::MessageDialog md("Input error");
			 md.set_secondary_text("This cell does not end with a delimiter (a \":\", \";\" or \".\")");
			 md.set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
			 md.run();
			 }
		else {
#ifdef DEBUG
			 std::cerr << "sending: " << tmp << std::endl;
#endif
			 content_changed();
			 emitter(tmp);
			 }
		return true;
		}
	else {
		bool retval=Gtk::TextView::on_key_press_event(event);
		while (gtk_events_pending ())
			gtk_main_iteration ();

		// If this was a real key press (i.e. not just SHIFT or ALT or similar), emit a
		// signal so that the cell can be scrolled into view if necessary.
		// FIXME: I do not know how to do this correctly, check docs.

		if(event->keyval < 65000L)
			 content_changed();
		return retval;
		}
	}

bool CodeInput::handle_button_press(GdkEventButton* button)
	{
	if(button->button!=2) return false;

	Glib::RefPtr<Gtk::Clipboard> refClipboard = Gtk::Clipboard::get(GDK_SELECTION_PRIMARY);

	std::vector<Glib::ustring> sah=refClipboard->wait_for_targets();
	bool hastext=false;
	bool hasstring=false;
	Gtk::SelectionData sd;

	// find out _where_ to insert
	Gtk::TextBuffer::iterator insertpos;
	int somenumber;
	edit.get_iter_at_position(insertpos, somenumber, button->x, button->y);
	++insertpos;

	for(unsigned int i=0; i<sah.size(); ++i) {
		 if(sah[i]=="cadabra") {
			  sd=refClipboard->wait_for_contents("cadabra");
			  std::string topaste=sd.get_data_as_string();
			  insertpos=edit.get_buffer()->insert(insertpos, topaste);
			  edit.get_buffer()->place_cursor(insertpos);
			  return true;
			  }
		 else if(sah[i]=="TEXT")
			  hastext=true;
		 else if(sah[i]=="STRING")
			  hasstring=true;
		 }
	
	if(hastext)        sd=refClipboard->wait_for_contents("TEXT");
	else if(hasstring) sd=refClipboard->wait_for_contents("STRING");
	if(hastext || hasstring) {
		 insertpos=edit.get_buffer()->insert(insertpos, sd.get_data_as_string());
		 edit.get_buffer()->place_cursor(insertpos);
		 }

	return true;
	}

bool CodeInput::exp_input_tv::on_expose_event(GdkEventExpose *event)
	{
	Glib::RefPtr<Gdk::Window> win = Gtk::TextView::get_window(Gtk::TEXT_WINDOW_TEXT);

// FIXME: what does this do?
//	bool ret=Gtk::TextView::on_expose_event(event);

	int w, h, x, y;
	win->get_geometry(x,y,w,h);

	Cairo::RefPtr<Cairo::Context> cr = win->create_cairo_context();

	// paint the background
	cr->set_source_rgba(1.0, 1.0, 1.0, 1.0);
	cr->rectangle(5,3,8,h-3);
	cr->fill();

	cr->set_source_rgba(.2, .2, .7, 1.0);
	cr->set_line_width(1.0);
	cr->set_antialias(Cairo::ANTIALIAS_NONE);
	cr->move_to(8,3);
	cr->line_to(5,3);
	cr->line_to(5,h-3); 
	cr->line_to(8,h-3); 
	cr->stroke();
	
   return true;
//	return ret;
	}
