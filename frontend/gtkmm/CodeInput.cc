
#include "CodeInput.hh"
#include <gdkmm/general.h>
#include <gtkmm/messagedialog.h>
#include <gdk/gdkkeysyms.h>
#include <iostream>

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

CodeInput::CodeInput()
	: buffer(Gtk::TextBuffer::create()), edit(buffer)
	{
	init();
	}

CodeInput::CodeInput(Glib::RefPtr<Gtk::TextBuffer> tb)
	: buffer(tb), edit(tb)
	{
	init();
	}

void CodeInput::init() 
	{
//	scroll_.set_size_request(-1,200);
//	scroll_.set_border_width(1);
//	scroll_.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
	edit.override_font(Pango::FontDescription("monospace")); 
	edit.set_wrap_mode(Gtk::WRAP_NONE);
	edit.override_color(Gdk::RGBA("blue"));
	edit.override_background_color(Gdk::RGBA("white"));
	edit.set_pixels_above_lines(5);
	edit.set_pixels_below_lines(5);
//	edit.set_pixels_below_lines(Gtk::LINE_SPACING);
//	edit.set_pixels_inside_wrap(2*Gtk::LINE_SPACING);
	edit.set_left_margin(15);
	// Need CSS tweaks for top margin
//	edit.set_top_margin(15);
	edit.set_accepts_tab(true);
//	Pango::TabArray

	edit.signal_button_press_event().connect(sigc::mem_fun(this, 
																				&CodeInput::handle_button_press), 
															 false);
//	edit.get_buffer()->signal_changed().connect(sigc::mem_fun(this, &CodeInput::handle_changed));
	edit.set_can_focus(true);

//	add(hbox);
//	hbox.add(vsep);
//	hbox.add(edit);
	add(edit);
//	set_border_width(3);
	show_all();
	}

bool CodeInput::exp_input_tv::on_key_press_event(GdkEventKey* event)
	{
	bool is_shift_return = 	get_editable() && event->keyval==GDK_KEY_Return && (event->state&Gdk::SHIFT_MASK);
	bool retval;
	// std::cerr << event->keyval << ", " << event->state << " pressed" << std::endl;
	
	if(!is_shift_return) 
		retval=Gtk::TextView::on_key_press_event(event);

	Glib::RefPtr<Gtk::TextBuffer> textbuf=get_buffer();
	std::string tmp(textbuf->get_text(get_buffer()->begin(), get_buffer()->end()));
	
	if(get_editable() && event->keyval==GDK_KEY_Return && (event->state&Gdk::SHIFT_MASK)) { // shift-return
		content_changed(tmp);
		content_execute();
		return true;
		}
	else {
		// If this was a real key press (i.e. not just SHIFT or ALT or similar), emit a
		// signal so that the cell can be scrolled into view if necessary.
		// FIXME: I do not know how to do this correctly, check docs.

		if(event->keyval < 65000L)
			 content_changed(tmp);

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

bool CodeInput::exp_input_tv::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
	{
	Glib::RefPtr<Gdk::Window> win = Gtk::TextView::get_window(Gtk::TEXT_WINDOW_TEXT);

	bool ret=Gtk::TextView::on_draw(cr);

	int w, h, x, y;
	win->get_geometry(x,y,w,h);

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
	
	return ret;
	}
