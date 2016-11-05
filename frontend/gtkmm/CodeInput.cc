
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

CodeInput::exp_input_tv::exp_input_tv(DTree::iterator it, Glib::RefPtr<Gtk::TextBuffer> tb, double scale)
	: Gtk::TextView(tb), scale_(scale), datacell(it)
	{
	set_events(Gdk::STRUCTURE_MASK);
//	get_buffer()->signal_insert().connect(sigc::mem_fun(this, &exp_input_tv::on_my_insert), false);
//	get_buffer()->signal_erase().connect(sigc::mem_fun(this, &exp_input_tv::on_my_erase), false);
	}

//CodeInput::CodeInput()
//	: buffer(Gtk::TextBuffer::create()), edit(buffer)
//	{
//	init();
//	}

CodeInput::CodeInput(DTree::iterator it, Glib::RefPtr<Gtk::TextBuffer> tb, double s, int font_step)
	: buffer(tb), edit(it, tb, s)
	{
	init(font_step);
	}

CodeInput::CodeInput(DTree::iterator it, const std::string& txt, double s, int font_step)
	: buffer(Gtk::TextBuffer::create()), edit(it, buffer, s)
	{
	buffer->set_text(txt);
	init(font_step);
	}

void CodeInput::init(int font_step) 
	{
//	scroll_.set_size_request(-1,200);
//	scroll_.set_border_width(1);
//	scroll_.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
#ifndef __APPLE__
	set_font_size(font_step);
#endif
	edit.set_wrap_mode(Gtk::WRAP_NONE);

//	edit.override_background_color(Gdk::RGBA("white"), Gtk::STATE_FLAG_ACTIVE);

//	edit.set_name("mywidget");
//	gtk_rc_parse_string("style \"mywidget\"\n"
//							  "{\n"
//							  "  bg[NORMAL] = white\n"
//							  "}\n"
//							  "widget \"*.mywidget\" style \"mywidget\"");	
//
	
	edit.set_pixels_above_lines(1);
	edit.set_pixels_below_lines(1);
	edit.set_pixels_inside_wrap(1);
	set_margin_top(10);
	set_margin_bottom(10);
//	edit.set_pixels_below_lines(Gtk::LINE_SPACING);
//	edit.set_pixels_inside_wrap(2*Gtk::LINE_SPACING);
	
	// Padding using CSS does not work on earlier Gtk versions, so we use set_left_margin there.
	edit.set_left_margin(20);
//	if(gtk_get_minor_version()<11 || gtk_get_minor_version()>=14)

	edit.set_accepts_tab(true);
	Pango::TabArray tabs(10);
	// FIXME: use character width measured, instead of '8', or at least
	// understand how Pango units are supposed to work.
	for(int i=0; i<10; ++i) 
		tabs.set_tab(i, Pango::TAB_LEFT, 4*8*i);
	edit.set_tabs(tabs);

	edit.signal_button_press_event().connect(sigc::mem_fun(this, 
																				&CodeInput::handle_button_press), 
															 false);

	edit.get_buffer()->signal_insert().connect(sigc::mem_fun(this, &CodeInput::handle_insert), true);
	edit.get_buffer()->signal_erase().connect(sigc::mem_fun(this, &CodeInput::handle_erase), false);

	edit.set_can_focus(true);

	add(edit);
//	set_border_width(3);
	show_all();
	}

bool CodeInput::exp_input_tv::on_key_press_event(GdkEventKey* event)
	{
	bool is_shift_return = get_editable() && event->keyval==GDK_KEY_Return && (event->state&Gdk::SHIFT_MASK);
//	bool is_shift_tab    = get_editable() && event->keyval==GDK_KEY_Tab && (event->state&Gdk::SHIFT_MASK);
	bool retval=false;
	// std::cerr << event->keyval << ", " << event->state << " pressed" << std::endl;
	
	if(!is_shift_return) 
		retval=Gtk::TextView::on_key_press_event(event);

	Glib::RefPtr<Gtk::TextBuffer> textbuf=get_buffer();
	std::string tmp(textbuf->get_text(get_buffer()->begin(), get_buffer()->end()));
	
	if(is_shift_return) {
//		content_changed(tmp, datacell);
		content_execute(datacell);
		return true;
		}
//	else {
//		// If this was a real key press (i.e. not just SHIFT or ALT or similar), emit a
//		// signal so that the cell can be scrolled into view if necessary.
//		// FIXME: I do not know how to do this correctly, check docs.
//		// FIXME: this should be done by monitoring the buffer for changes, see
//		// http://stackoverflow.com/questions/9250238/detecting-text-view-change-in-gtk-and-mono
//
//		if(event->keyval < 65000L)
//			content_changed(tmp, datacell);
//
//		return retval;
//		}

	return retval;
	}

void CodeInput::exp_input_tv::shift_enter_pressed()
	{
	Glib::RefPtr<Gtk::TextBuffer> textbuf=get_buffer();
	std::string tmp(textbuf->get_text(get_buffer()->begin(), get_buffer()->end()));

//	content_changed(tmp, datacell);
	content_execute(datacell);
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
//	cr->rectangle(5,3,8,h-3);
//	cr->fill();

	if(datacell->cell_type==DataCell::CellType::latex) 
		cr->set_source_rgba(.2, .7, .2, 1.0);
	else
		cr->set_source_rgba(.2, .2, .7, 1.0);
	double line_width=2.0/1.6*scale_;
	cr->set_line_width(line_width);
	cr->set_antialias(Cairo::ANTIALIAS_NONE);
	int hor=5;
	cr->move_to(5+hor,line_width);
	cr->line_to(5,line_width);
	cr->line_to(5,h-line_width); 
	cr->line_to(5+hor,h-line_width); 
	cr->stroke();

	// Mark whether cell is executing.
	if(datacell->running) {
//	cr->set_source_rgba(.8, .2, .2, 1.0);
		cr->set_source_rgba(.2, .2, .7, 0.5);
		int rem=hor-line_width;
		cr->set_line_width(2*rem);
		cr->set_antialias(Cairo::ANTIALIAS_NONE);
		cr->move_to(5+rem,  rem);
		cr->line_to(5+rem,  h-rem);
		cr->stroke();
		}

	
	return ret;
	}

bool CodeInput::exp_input_tv::on_focus_in_event(GdkEventFocus *event) 
	{
	cell_got_focus(datacell);
	return Gtk::TextView::on_focus_in_event(event);
	}

void CodeInput::exp_input_tv::on_show() 
	{
	if(!datacell->hidden)
		Gtk::TextView::on_show();
	}

void CodeInput::update_buffer()
	{
	std::string newtxt = edit.datacell->textbuf;
	Glib::RefPtr<Gtk::TextBuffer> textbuf=edit.get_buffer();
	std::string oldtxt = textbuf->get_text(edit.get_buffer()->begin(), edit.get_buffer()->end());
	if(newtxt!=oldtxt) {
		// std::cerr << "setting buffer from " 
		// 			 << oldtxt
		// 			 << " to " << newtxt << std::endl;
		buffer->set_text(newtxt);
		}
	}

void CodeInput::handle_insert(const Gtk::TextIter& pos, const Glib::ustring& text, int bytes)
	{
	Glib::RefPtr<Gtk::TextBuffer> buf=edit.get_buffer();
	// warning: pos contains the cursor pos, and because we get to this handler
	// _after_ the default handler has run, the cursor will have moved by
	// the length of the insertion.
	edit.content_insert(text, std::distance(buf->begin(), pos)-bytes, edit.datacell);
	}

void CodeInput::handle_erase(const Gtk::TextIter& start, const Gtk::TextIter& end)
	{
	//std::cerr << "handle_erase: " << start << ", " << end << std::endl;
	Glib::RefPtr<Gtk::TextBuffer> buf=edit.get_buffer();
	edit.content_erase(std::distance(buf->begin(), start), std::distance(buf->begin(), end), edit.datacell);
	}

void CodeInput::slice_cell(std::string& before, std::string& after)
	{
	Glib::RefPtr<Gtk::TextBuffer> textbuf=edit.get_buffer();

	Gtk::TextBuffer::iterator it=textbuf->get_iter_at_mark(textbuf->get_insert());
	before=textbuf->get_slice(textbuf->begin(), it);
	after =textbuf->get_slice(it, textbuf->end());
	}

void CodeInput::set_font_size(int num)
	{
	std::ostringstream fstr;
	fstr << "monospace " << 9+(num*2); 
	edit.override_font(Pango::FontDescription(fstr.str()));
	}
