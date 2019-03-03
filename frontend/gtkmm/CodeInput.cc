#include "CodeInput.hh"
#include <gdkmm/general.h>
#include <gtkmm/messagedialog.h>
#include <gdk/gdkkeysyms.h>
#include <gdkmm/rgba.h>
#include <iostream>
#include <regex>
#include "Keywords.hh"

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
	set_name("CodeInput"); // to be able to style it with CSS
	}

//CodeInput::CodeInput()
//	: buffer(Gtk::TextBuffer::create()), edit(buffer)
//	{
//	init();
//	}

CodeInput::CodeInput(DTree::iterator it, Glib::RefPtr<Gtk::TextBuffer> tb, double s, const Prefs& prefs)
	: buffer(tb), edit(it, tb, s)
	{
	init(prefs);
	}

CodeInput::CodeInput(DTree::iterator it, const std::string& txt, double s, const Prefs& prefs)
	: buffer(Gtk::TextBuffer::create()), edit(it, buffer, s)
	{
	buffer->set_text(txt);
	init(prefs);
	}

void CodeInput::init(const Prefs& prefs)
	{
	//	scroll_.set_size_request(-1,200);
	//	scroll_.set_border_width(1);
	//	scroll_.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
	set_font_size(prefs.font_step);
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

	edit.signal_button_press_event().connect(sigc::mem_fun(this, &CodeInput::handle_button_press), false);
	edit.get_buffer()->signal_insert().connect(sigc::mem_fun(this, &CodeInput::handle_insert), true);
	edit.get_buffer()->signal_erase().connect(sigc::mem_fun(this, &CodeInput::handle_erase), false);
	if (prefs.highlight) {
		using namespace std::string_literals;
		switch (edit.datacell->cell_type) {
		// Fallthrough
		case DataCell::CellType::python:
		case DataCell::CellType::latex:
			enable_highlighting(edit.datacell->cell_type, prefs);
			break;
		default:
			break;
			}
		}
	edit.set_can_focus(true);

	add(edit);
	//	set_border_width(3);
	show_all();
	}

gunichar deref(Gtk::TextBuffer::iterator it, size_t n)
	{
	it.forward_chars(n);
	return it.is_end() ? '\0' : *it;
	}

void CodeInput::highlight_python()
	{

	auto buf = edit.get_buffer();

	// Remove all tags that are currently set
	buf->remove_all_tags(buf->begin(), buf->end());

	// 0 = not set
	// 1 = number
	// 2 = comment
	// 3 = name
	// 4 = single-quoted string
	// 5 = double-quoted string
	// 6 = triple single-quoted string
	// 7 = triple double-quoted string
	// 8 = multiline math mode
	// 9 = singleline math mode

	auto it = buffer->begin();
	auto start = it;
	int wordtype = 0;
	int count = 0;
	bool finished = false;

	while (!finished) {
		if (it.is_end())
			finished = true;

		auto cur = deref(it, 0);
		auto next = deref(it, 1);

		if (wordtype == 1) { // End on non-number
			if (g_unichar_isxdigit(cur) || cur == '.' ||
			      cur == 'x' || cur == 'o' || cur == 'j' ||
			      cur == 'X' || cur == 'O' || cur == 'J');
			else if (cur == 'e' || cur == 'E') {
				if (next == '+' || next == '-')
					++it;
				}
			else {
				buf->apply_tag_by_name("number", start, it);
				wordtype = 0;
				}
			}
		else if (wordtype == 2) {   // End on linebreak
			if (cur == '\n') {
				buf->apply_tag_by_name("comment", start, it);
				wordtype = 0;
				}
			// We might have mistakenly assumed the line is a comment
			// due to a # sign; but this could also be in the middle of
			// the implicit math mode at the start of a property declaration.
			// Therefore, if inside comment mode we find a :: then we should
			// exit comment mode and let the line be reformatted accordingly,
			// even though this does the wrong thing if someone has typed '::'
			// in a comment.
			if (cur == ':' && next == ':') {
				wordtype = 0;
				}
			}
		else if (wordtype == 3) {   // End if not alnum or _
			if (cur == '_' || g_unichar_isalnum(cur));
			else {
				if (*start == '@') {
					buf->apply_tag_by_name("decorator", start, it);
					}
				else {
					std::string name(start, it);
					auto tag = get_keyword_group(name);
					if (tag) {
						buf->apply_tag_by_name(tag, start, it);
						}
					}
				wordtype = 0;
				}
			}
		else if (wordtype == 4) {   // End on ' but skip \'
			if (cur == '\'') {
				++it;
				buf->apply_tag_by_name("string", start, it);
				wordtype = 0;
				}
			else if (cur == '\\' && next == '\'') {
				++it;
				}
			}
		else if (wordtype == 5) {   // End on " but skip \"
			if (cur == '"') {
				++it;
				buf->apply_tag_by_name("string", start, it);
				wordtype = 0;
				}
			else if (cur == '\\' && next == '\"') {
				++it;
				}
			}
		else if (wordtype == 6) {   // End on '''
			if (cur == '\'') {
				++count;
				if (count == 3) {
					count = 0;
					++it;
					buf->apply_tag_by_name("string", start, it);
					wordtype = 0;
					}
				}
			else {
				if (cur == '\\' && next == '\'')
					++it;
				count = 0;
				}

			}
		else if (wordtype == 7) {   // End on """
			if (cur == '"') {
				++count;
				if (count == 3) {
					count = 0;
					++it;
					buf->apply_tag_by_name("string", start, it);
					wordtype = 0;
					}
				}
			else {
				if (cur == '\\' && next == '"')
					++it;
				count = 0;
				}

			}
		else if (wordtype == 8) {   // End on :;.
			if (cur == ':' || cur == ';' || cur == '.') {
				buf->apply_tag_by_name("maths", start, it);
				wordtype = 0;
				}
			}
		else if (wordtype == 9) {   // End on $
			if (cur == '$') {
				++it;
				buf->apply_tag_by_name("maths", start, it);
				wordtype = 0;
				}
			}
		else if (wordtype == 10) {   // Highlight and move on
			buf->apply_tag_by_name("brace", start, it);
			wordtype = 0;
			}
		else if (wordtype == 11) {   //Highlight and move on
			buf->apply_tag_by_name("operator", start, it);
			wordtype = 0;
			}

		if (wordtype == 0) {
			cur = deref(it, 0);
			next = deref(it, 1);

			// Set word start iterator
			start = it;

			// Start a number on 0-9, or .[0-9]
			if (g_unichar_isdigit(cur) || (cur == '.' && g_unichar_isdigit(next))) {
				wordtype = 1;
				}
			// Start a comment on # if the next symbol isn't }
			else if (cur == '#') {
				wordtype = 2;
				}
			// Start a name on a letter or _ @
			else if (g_unichar_isalpha(cur) || cur == '_' || cur == '@') {
				wordtype = 3;
				}
			// Start a single quoted string on '
			else if (cur == '\'') {
				// Decide if this is triple quoted
				if (next == '\'' && deref(it, 2) == '\'') {
					wordtype = 6;
					it.forward_chars(2);
					}
				else {
					wordtype = 4;
					}
				}
			// Start a double quoted string on "
			else if (cur == '"') {
				// Decide if this is triple quoted
				if (next == '"' && deref(it, 2) == '"') {
					wordtype = 7;
					it.forward_chars(2);
					}
				else {
					wordtype = 5;
					}
				}
			// Start multiline math mode on :=
			else if (cur == ':' && next == '=') {
				wordtype = 8;
				it.forward_chars(2);
				buf->apply_tag_by_name("operator", start, it);
				start = it;
				}
			// Reformat current line as maths if :: found
			else if (cur == ':' && next == ':') {
				wordtype = 11;
				auto line_begin = it;
				auto line_end = it;
				line_begin.backward_find_char([](gunichar c) {
					return c == '\n';
					});
				line_end.forward_to_line_end();
				buf->remove_all_tags(line_begin, line_end);
				buf->apply_tag_by_name("maths", line_begin, it);
				}
			// Start inline math mode on $
			else if (cur == '$')
				wordtype = 9;
			// Highlight as brace
			else if (cur == '{' || cur == '[' || cur == '(' ||
			         cur == '}' || cur == ']' || cur == ')') {
				wordtype = 10;
				}
			else if (g_unichar_ispunct(cur)) {
				wordtype = 11;
				}
			}
		++it;
		}
	}

void CodeInput::highlight_latex()
	{
	auto buf = edit.get_buffer();
	buf->remove_all_tags(buf->begin(), buf->end());

	// 0 = none
	// 1 = comment
	// 2 = command
	// 3 = can expect a parameter to begin
	// 4 = inline math mode
	auto it = buf->begin();
	auto start = it;
	int wordtype = 0;
	int curly_depth = 0;
	int square_depth = 0;
	bool finished = false;

	while (!finished) {
		if (it.is_end())
			finished = true;

		auto cur = deref(it, 0);

		if (wordtype == 1) {
			if (cur == '\n') {
				buf->apply_tag_by_name("comment", start, it);
				start = it;
				wordtype = 0;
				}
			}
		else if (wordtype == 2) {
			if (!g_unichar_isalpha(cur)) {
				buf->apply_tag_by_name("command", start, it);
				start = it;
				wordtype = 5;
				}
			}
		else if (wordtype == 4) {
			if (cur == '$') {
				++it;
				cur = deref(it, 0);
				buf->apply_tag_by_name("maths", start, it);
				wordtype = 0;
				}
			}

		if (wordtype == 5) {
			if (cur == '{') {
				++curly_depth;
				start = it;
				++it;
				cur = deref(it, 0);
				}
			else if (cur == '[') {
				++square_depth;
				start = it;
				++it;
				cur = deref(it, 0);
				}
			wordtype = 0;
			}

		if (wordtype == 0) {
			if (cur == '}' && curly_depth) {
				auto next = it;
				next.forward_char();
				buf->apply_tag_by_name("parameter", start, next);
				--curly_depth;
				wordtype = 5;
				}
			else if (cur == ']' && square_depth) {
				auto next = it;
				next.forward_char();
				buf->apply_tag_by_name("parameter", start, next);
				--square_depth;
				wordtype = 5;
				}
			else if (cur == '\\') {
				if (curly_depth || square_depth)
					buf->apply_tag_by_name("parameter", start, it);
				start = it;
				wordtype = 2;
				}
			else if (cur == '%') {
				if (curly_depth || square_depth)
					buf->apply_tag_by_name("parameter", start, it);
				start = it;
				wordtype = 1;
				}
			else if (cur == '$') {
				wordtype = 4;
				start = it;
				}
			}

		++it;
		}
	}

void CodeInput::enable_highlighting(DataCell::CellType cell_type, const Prefs& prefs)
	{
	std::string map_idx;
	void (CodeInput::*callback)()=0;

	switch (cell_type) {
	case DataCell::CellType::python:
		map_idx = "python";
		callback = &CodeInput::highlight_python;
		break;
	case DataCell::CellType::latex:
		map_idx = "latex";
		callback = &CodeInput::highlight_latex;
		break;
	default:
		break;
		}

	// Create tags
	for (const auto& elem : prefs.colours.at(map_idx)) {
		if (edit.get_buffer()->get_tag_table()->lookup(elem.first)) // Already set
			edit.get_buffer()->get_tag_table()->lookup(elem.first)->property_foreground_rgba() = Gdk::RGBA(elem.second);
		else // Need to create
			edit.get_buffer()->create_tag(elem.first)->property_foreground_rgba() = Gdk::RGBA(elem.second);
		}

	// Error tag
	if (!edit.get_buffer()->get_tag_table()->lookup("error")) {
		auto error_tag = edit.get_buffer()->create_tag("error");
		error_tag->property_underline() = Pango::Underline::UNDERLINE_ERROR;
		error_tag->property_underline_rgba() = Gdk::RGBA("red");
		}

	// Setup callback
	if (hl_conn.connected())
		hl_conn.disconnect();

	hl_conn = edit.get_buffer()->signal_changed().connect(sigc::mem_fun(*this, callback));

	// And perform an initial highlight
	if(callback!=0)
		(this->*callback)();
	}


void CodeInput::disable_highlighting()
	{
	// Remove all tags that are currently set
	edit.get_buffer()->remove_all_tags(
	   edit.get_buffer()->begin(),
	   edit.get_buffer()->end()
	);

	// Disconnect the signal
	if (hl_conn.connected())
		hl_conn.disconnect();
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
		content_changed(tmp, datacell);
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

	content_changed(tmp, datacell);
	content_execute(datacell);
	}

bool CodeInput::handle_button_press(GdkEventButton* button)
	{
	if(button->button!=2 || button->type!=GDK_BUTTON_PRESS) return false;

	Glib::RefPtr<Gtk::Clipboard> refClipboard = Gtk::Clipboard::get(GDK_SELECTION_PRIMARY);

	std::vector<Glib::ustring> sah=refClipboard->wait_for_targets();
	bool hascadabra=false;
	bool hastext=false;
	bool hasstring=false;
	Gtk::SelectionData sd;

	for(unsigned int i=0; i<sah.size(); ++i) {
		if(sah[i]=="cadabra") {
			hascadabra=true;
			break;
			}
		else if(sah[i]=="TEXT")
			hastext=true;
		else if(sah[i]=="STRING")
			hasstring=true;
		}

	if(hascadabra)     sd=refClipboard->wait_for_contents("cadabra");
	else if(hastext)   sd=refClipboard->wait_for_contents("TEXT");
	else if(hasstring) sd=refClipboard->wait_for_contents("STRING");
	if(hascadabra || hastext || hasstring) {
		// find out _where_ to insert
		Gtk::TextBuffer::iterator insertpos;
		int somenumber;
		edit.get_iter_at_position(insertpos, somenumber, button->x, button->y);
		if(insertpos!=edit.get_buffer()->end())
			++insertpos;

		// std::cerr << "inserting at " << insertpos << " text " << sd.get_data_as_string() << std::endl;
		insertpos=edit.get_buffer()->insert(insertpos, sd.get_data_as_string());
		// std::cerr << "placing cursor" << std::endl;
		edit.get_buffer()->place_cursor(insertpos);
		}

	return true;
	}

bool CodeInput::exp_input_tv::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
	{
	Glib::RefPtr<Gdk::Window> win = Gtk::TextView::get_window(Gtk::TEXT_WINDOW_TEXT);

	//	std::cerr << "on draw for " << get_buffer()->get_text() << std::endl;

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
	float size=9+(num*2);
#ifdef __APPLE__
	size*=1.5;
#endif
	fstr << "monospace " << size;
	edit.override_font(Pango::FontDescription(fstr.str()));
	edit.queue_resize();
	}
