#include "CodeInput.hh"
#include <gdkmm/general.h>
#include <gtkmm/messagedialog.h>
#include <gdk/gdkkeysyms.h>
#include <gdkmm/rgba.h>
//#include <gdkmm/root.h>
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

CodeInput::exp_input_tv::exp_input_tv(DTree::iterator it, Glib::RefPtr<Gtk::TextBuffer> tb, double scale,
												  Glib::RefPtr<Gtk::Adjustment> vadjustment_)
	: Gtk::TextView(tb), scale_(scale), datacell(it), vadjustment(vadjustment_)
	{
	set_events(Gdk::STRUCTURE_MASK);
	//	get_buffer()->signal_insert().connect(sigc::mem_fun(this, &exp_input_tv::on_my_insert), false);
	//	get_buffer()->signal_erase().connect(sigc::mem_fun(this, &exp_input_tv::on_my_erase), false);
	get_buffer()->signal_changed().connect(sigc::mem_fun(this, &exp_input_tv::on_textbuf_change), false);
	set_name("CodeInput"); // to be able to style it with CSS
	}

CodeInput::CodeInput(DTree::iterator it, Glib::RefPtr<Gtk::TextBuffer> tb, double s, const Prefs& prefs,
							Glib::RefPtr<Gtk::Adjustment> vadjustment)
	: Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL)
	, buffer(tb)
	, edit(it, tb, s, vadjustment)
	{
	init(prefs);
	}

CodeInput::CodeInput(DTree::iterator it, const std::string& txt, double s, const Prefs& prefs,
							Glib::RefPtr<Gtk::Adjustment> vadjustment)
	: Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL)
	, buffer(Gtk::TextBuffer::create())
	, edit(it, buffer, s, vadjustment)
	{
	buffer->set_text(txt);
	init(prefs);
	}

void CodeInput::on_size_allocate(Gtk::Allocation& allocation)
	{
//	allocation.set_width(edit.window_width); // Fixed width

//	Gtk::Widget* current = this;
//	while (current->get_parent()) {
//		current = current->get_parent();
//		}
//	
//	Gtk::Window *mywindow = static_cast<Gtk::Window *>(current);
//	if(mywindow) {
//		std::cerr << "The root window of CodeInput has " << mywindow->get_allocation().get_width() << std::endl;
//		}	
////	allocation.set_width(thewidth);
//
	Gtk::Box::on_size_allocate(allocation);
	}


Gtk::SizeRequestMode CodeInput::exp_input_tv::get_request_mode_vfunc() const
	{
	return Gtk::SizeRequestMode::SIZE_REQUEST_WIDTH_FOR_HEIGHT;
	}

void CodeInput::exp_input_tv::get_preferred_width_for_height_vfunc(int height,
																						 int& minimum_width, int& natural_width) const
	{
	if(datacell->cell_type==DataCell::CellType::latex) {
		minimum_width = window_width-20;
		natural_width = window_width-20;
		// std::cerr << "requested widths for " << datacell->textbuf.substr(0, 20) << ": " << window_width << std::endl;
		}
	else {
		return Gtk::TextView::get_preferred_width_for_height_vfunc(height, minimum_width, natural_width);
		}
	}

void CodeInput::exp_input_tv::on_size_allocate(Gtk::Allocation& allocation)
	{
	// std::cerr << "allocation requested for " << datacell->textbuf.substr(0, 20) << ": " << allocation.get_width() << std::endl;
	if(datacell->cell_type==DataCell::CellType::latex) 
		allocation.set_width(window_width-20);
	Gtk::TextView::on_size_allocate(allocation);
	}

void CodeInput::init(const Prefs& prefs)
	{
	//	scroll_.set_size_request(-1,200);
	//	scroll_.set_border_width(1);
	//	scroll_.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
//	edit.set_wrap_mode(Gtk::WRAP_NONE); // WRAP_WORD_CHAR); wrapping leads to weird effects
	edit.set_pixels_above_lines(1);
	edit.set_pixels_below_lines(1);
	edit.set_pixels_inside_wrap(1);

//	set_size_request(400, -1);
	
	// The following two are margins around the vbox which contains the
	// text input and the LaTeX output(s).
	set_margin_top(10);
	set_margin_bottom(0);
	//	edit.set_pixels_below_lines(Gtk::LINE_SPACING);
	//	edit.set_pixels_inside_wrap(2*Gtk::LINE_SPACING);

	// Padding using CSS does not work on earlier Gtk versions, so we use set_left_margin there.
	edit.set_left_margin(20);
	//	if(gtk_get_minor_version()<11 || gtk_get_minor_version()>=14)

	// Determine the width of a tab.
	auto layout = Pango::Layout::create(edit.get_pango_context());
	Pango::Rectangle logical_rect;
	layout->set_text(" ");
	int space_width, space_height;
	layout->get_pixel_size(space_width, space_height);

	// Set 10 tab stops, each 3 spaces wide.
	edit.set_monospace(true);
	edit.set_accepts_tab(true);
	Pango::TabArray tabs(10);
	for(int i=0; i<10; ++i)
		tabs.set_tab(i, Pango::TAB_LEFT, 3*space_width*i);
	edit.set_tabs(tabs);

	edit.signal_button_press_event().connect(sigc::mem_fun(this, &CodeInput::handle_button_press), false);
	edit.get_buffer()->signal_insert().connect(sigc::mem_fun(this, &CodeInput::handle_insert), true /* run before default handler */);
	edit.get_buffer()->signal_erase().connect(sigc::mem_fun(this, &CodeInput::handle_erase), false);
	if (prefs.highlight) {
		using namespace std::string_literals;
		switch (edit.datacell->cell_type) {
			case DataCell::CellType::latex:
				enable_highlighting(edit.datacell->cell_type, prefs);
				break;
			case DataCell::CellType::python:
				enable_highlighting(edit.datacell->cell_type, prefs);
				break;
			default:
				break;
			}
		}

	edit.set_can_focus(true);

	auto dummy_adj = Gtk::Adjustment::create(0,0,0);
	edit.set_focus_hadjustment(dummy_adj);

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
		else if (wordtype == 2) {   // End on linebreak or end of document
			if (cur == '\n' || cur == '\0') {
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
			if (cur == '\n' || cur == '\0') {
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
#if GTKMM_MINOR_VERSION>=20		
		error_tag->property_underline_rgba() = Gdk::RGBA("red");
#endif
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

void CodeInput::relay_cursor_pos(std::function<void(int, int)> callback)
	{
	edit.get_buffer()->property_cursor_position().signal_changed().connect([this, callback]() {
		auto iter = edit.get_buffer()->get_iter_at_mark(edit.get_buffer()->get_insert());
		auto line = iter.get_line() + 1;
		auto line_offset = iter.get_line_offset() + 1;
		callback(line, line_offset);
		});
	}

bool CodeInput::exp_input_tv::on_key_press_event(GdkEventKey* event)
	{
	// The key symbols are in /usr/include/gtk-3.0/gdk/gdkkeysyms.h
	bool is_shift_return = get_editable() && event->keyval==GDK_KEY_Return && (event->state&Gdk::SHIFT_MASK);
	//	bool is_shift_tab    = get_editable() && event->keyval==GDK_KEY_Tab && (event->state&Gdk::SHIFT_MASK);
	bool is_tab = get_editable() && event->keyval==GDK_KEY_Tab;
	bool is_ctrl_k = get_editable() && event->keyval==GDK_KEY_k && (event->state&Gdk::CONTROL_MASK);
	bool is_ctrl_a = get_editable() && event->keyval==GDK_KEY_a && (event->state&Gdk::CONTROL_MASK);
	bool is_ctrl_e = get_editable() && event->keyval==GDK_KEY_e && (event->state&Gdk::CONTROL_MASK);
	bool is_ctrl_qm = get_editable() && event->keyval==GDK_KEY_question && (event->state&Gdk::CONTROL_MASK);
	
	bool retval=false;
//	std::cerr << event->keyval << ", " << event->state << " pressed, focus = " << has_focus()
//	 			 << ", editable = " << get_editable() << ", is_shift_return = " << is_shift_return << std::endl;


	Glib::RefPtr<Gtk::TextBuffer> textbuf=get_buffer();

	if(is_shift_return) {
		content_changed(datacell);
		content_execute(datacell);
		return true;
		}
	else if(is_tab) {
		// If one or more lines are selected, indent the whole block.
		Gtk::TextBuffer::iterator beg, end;
		if(get_buffer()->get_selection_bounds(beg, end)) {
			if(beg.starts_line()) {
				int start_line = beg.get_line();
				int end_line = end.get_line();
				for (int line = start_line; line <= end_line; ++line) {
					auto line_start = get_buffer()->get_iter_at_line(line);
					get_buffer()->insert(line_start, "\t"); 
					}
				// Move start of selection back to start of line.
				get_buffer()->get_selection_bounds(beg, end);
				int line_number = beg.get_line();
				beg = get_buffer()->get_iter_at_line(line_number);
				get_buffer()->select_range(beg, end);
				}
			else {
				std::cerr << "start of selection not at start of line" << std::endl;
				}
			return true;
			}
		else {
			// Only complete if the last character is not whitespace.
			Glib::RefPtr<Gtk::TextBuffer::Mark> ins = get_buffer()->get_insert();
			Gtk::TextBuffer::iterator it=textbuf->get_iter_at_mark(ins);
			int ipos=textbuf->get_slice(textbuf->begin(), it).bytes();
			
			if(complete_request(datacell, ipos))
				return true;
			else
				retval=Gtk::TextView::on_key_press_event(event);
			}
		}
	else if(is_ctrl_a) {
		Glib::RefPtr<Gtk::TextBuffer::Mark> ins = get_buffer()->get_insert();
		Gtk::TextBuffer::iterator iter=textbuf->get_iter_at_mark(ins);
		iter.set_line(iter.get_line());
		textbuf->place_cursor(iter);
		return true;
		}
	else if(is_ctrl_e) {
		Glib::RefPtr<Gtk::TextBuffer::Mark> ins = get_buffer()->get_insert();
		Gtk::TextBuffer::iterator iter=textbuf->get_iter_at_mark(ins);
		iter.forward_to_line_end();
		textbuf->place_cursor(iter);
		return true;
		}
	else if(is_ctrl_k) {
		Glib::RefPtr<Gtk::TextBuffer::Mark> ins = get_buffer()->get_insert();
		Gtk::TextBuffer::iterator iter=textbuf->get_iter_at_mark(ins);
		auto line = iter.get_line();
		Gtk::TextBuffer::iterator line_start = iter;
		Gtk::TextBuffer::iterator line_end = iter;

		if(line_end.ends_line())
			line_end.forward_char();
		else
			line_end.forward_to_line_end();

		get_buffer()->erase(line_start, line_end);		
		return true;
		}
	else {
		retval=Gtk::TextView::on_key_press_event(event);
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
//	Glib::RefPtr<Gtk::TextBuffer> textbuf=get_buffer();
//	std::string tmp(textbuf->get_text(get_buffer()->begin(), get_buffer()->end()));

	content_changed(datacell);
	content_execute(datacell);
	}

void CodeInput::exp_input_tv::on_textbuf_change()
	{
	// When a keypress happens, this function gets called first (and for every
	// widget which shares the TextBuffer). This feeds through that a change
	// was made, but nothing else happens. The next event is the handle_insert
	// signal.
	content_changed(datacell);
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
		// Figure out where the mouse cursor is, so we know where to insert.
		Gtk::TextBuffer::iterator insertpos;
		int somenumber;
		edit.get_iter_at_position(insertpos, somenumber, button->x, button->y);
		if(insertpos!=edit.get_buffer()->end())
			++insertpos;
		insertpos=edit.get_buffer()->insert(insertpos, sd.get_data_as_string());
		edit.get_buffer()->place_cursor(insertpos);
		}

	return true;
	}

bool CodeInput::exp_input_tv::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
	{
	Glib::RefPtr<Gdk::Window> win = Gtk::TextView::get_window(Gtk::TEXT_WINDOW_TEXT);

	// std::cerr << "on draw for " << get_buffer()->get_text() << std::endl;

	bool ret=Gtk::TextView::on_draw(cr);

	int w, h, x, y;
	win->get_geometry(x,y,w,h);

	// paint the background
	cr->set_source_rgba(1.0, 1.0, 1.0, 1.0);
	//	cr->rectangle(5,3,8,h-3);
	//	cr->fill();

	if(datacell->cell_type==DataCell::CellType::latex)
		cr->set_source_rgba(.2, .7, .2, 1.0);
	else {
		if(datacell->ignore_on_import)
			cr->set_source_rgba(.4, .4, .7, 0.5);
		else
			cr->set_source_rgba(.2, .2, .7, 1.0);
		}
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
	// std::cerr << "FOCUS IN" << std::endl;
	cell_got_focus(datacell);
	if(previous_value>=0) 
		vadjustment->set_value(previous_value);
	return Gtk::TextView::on_focus_in_event(event);
	}

bool CodeInput::exp_input_tv::on_focus_out_event(GdkEventFocus *event)
	{
	// std::cerr << "FOCUS OUT" << std::endl;
	previous_value = -99.0;
	return Gtk::TextView::on_focus_out_event(event);
	}

bool CodeInput::exp_input_tv::on_motion_notify_event(GdkEventMotion* event)
	{
	// static int count=0;
   //	std::cerr << "MOTION " << ++count << std::endl;
	previous_value = vadjustment->get_value();
	return Gtk::TextView::on_motion_notify_event(event);
	}

// bool CodeInput::exp_input_tv::on_move_cursor_event(Glib::RefPtr<Gtk::TextBuffer::Mark> iter, Gtk::MovementStep step, bool extend_selection)
// 	{
// 	std::cerr << "on move cursor" << std::endl;
// 	return false;
// //	return Gtk::TextView::on_move_cursor(iter, step, extend_selection);
// // 	auto mark_iter = get_buffer()->get_insert();
// // 	
// //    // Explicitly control scrolling with the custom parameters (minimal scrolling)
// //    // 0.0 means no scrolling, so it won't scroll automatically
// // 	get_buffer()->scroll_to_mark(mark_iter, 0.0, false, 0.0, 0.0);
// // 	
// // 	// Return false to allow the cursor movement to proceed without interfering with other behaviors
// // 	return false;
// 	}


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

	// Word wrapping
	switch (edit.datacell->cell_type) {
		case DataCell::CellType::latex:
			edit.set_hexpand(false);
			edit.set_wrap_mode(Gtk::WRAP_WORD_CHAR);
			// std::cerr << "enabled word wrapping" << std::endl;
			break;
		case DataCell::CellType::python:
			edit.set_hexpand(true);
			edit.set_wrap_mode(Gtk::WRAP_NONE);
			break;
		default:
			break;
		}

	}

void CodeInput::handle_insert(const Gtk::TextIter& pos, const Glib::ustring& text, int bytes)
	{
	// If we have two CodeInput widgets which share the same
	// TextBuffer, then manually inserting (typing) text into one will
	// fire handle_insert on both widgets. So we need to not propagate
	// this change if we are not focused.

	if(edit.has_focus()==false) {
		return;
		}
	
	Glib::RefPtr<Gtk::TextBuffer> buf=edit.get_buffer();
	// warning: pos contains the cursor pos, and because we get to this handler
	// _after_ the default handler has run, the cursor will have moved by
	// the length of the insertion.
   //	std::cerr << text << ", " << text.bytes() << "; " << pos.get_line_index() << ", " << std::distance(buf->begin(), pos) << ",  " << pos.get_offset() << ", " << bytes << std::endl;
	// there is no quick way to get the byte offset for the iterator, so we do:
	int ipos = buf->get_slice(buf->begin(), pos).bytes();
	edit.content_insert(text, ipos-bytes, edit.datacell);
	}

void CodeInput::handle_erase(const Gtk::TextIter& start, const Gtk::TextIter& end)
	{
	// See handle_insert for the 'focus' logic.
	if(edit.has_focus()==false) {
		return;
		}
	
	//std::cerr << "handle_erase: " << start << ", " << end << std::endl;
	Glib::RefPtr<Gtk::TextBuffer> buf=edit.get_buffer();
	int spos = buf->get_slice(buf->begin(), start).bytes();
	int epos = buf->get_slice(buf->begin(), end).bytes();	
	edit.content_erase(spos, epos, edit.datacell);
	}

void CodeInput::slice_cell(std::string& before, std::string& after)
	{
	Glib::RefPtr<Gtk::TextBuffer> textbuf=edit.get_buffer();

	Gtk::TextBuffer::iterator it=textbuf->get_iter_at_mark(textbuf->get_insert());
	before=textbuf->get_slice(textbuf->begin(), it);
	after =textbuf->get_slice(it, textbuf->end());
	}

