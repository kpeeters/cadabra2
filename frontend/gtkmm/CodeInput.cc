
#include "CodeInput.hh"
#include <gdkmm/general.h>
#include <gtkmm/messagedialog.h>
#include <gdk/gdkkeysyms.h>
#include <gdkmm/rgba.h>
#include <iostream>
#include <regex>
#include <map>

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

void CodeInput::tag_by_regex(const std::string& tag, const std::string& regex_str, std::string& text)
{
	auto buf = edit.get_buffer();
	std::size_t cur_pos = 0;
	std::regex r(regex_str);
	std::smatch sm;
	std::string temp = text;

	while (std::regex_search(temp, sm, r)) {
		auto beg_it = buf->begin();
		beg_it.forward_chars(sm.position() + cur_pos);
		auto end_it = beg_it;
		end_it.forward_chars(sm.length());
		buf->apply_tag_by_name(tag, beg_it, end_it);
		text.replace(sm.position() + cur_pos, sm.length(), sm.length(), '_');
		cur_pos += sm.position() + sm.length();
		temp = sm.suffix();
	}
}

void CodeInput::highlight_python()
{
	using namespace std::string_literals;
	static const std::string keywords =	
		"False|None|True|and|as|assert|break|class|continue|def|del|elif|else|except|"
		"finally|for|from|global|if|import|in|is|lambda|nonlocal|not|or|pass|raise|"
		"return|try|while|with|yield";

	static const std::string builtins = 
		"abs|all|any|ascii|bin|bool|bytearray|bytes|callable|chr|classmethod|compile|"
		"complex|delattr|dict|dir|divmod|enumerate|eval|exec|filter|float|format|"
		"frozenset|getattr|globals|hashattr|hash|help|hex|id|input|int|isinstance|"
		"issubclass|iter|len|list|locals|map|max|memoryview|min|next|object|"
		"oct|open|ord|pow|print|property|range|repr|reversed|round|set|setattr|"
		"slice|sorted|staticmethod|str|sum|super|tuple|type|vars|zip|__import__"s;

	static const std::string algorithms = 
		"asym|canonicalise|collect_factors|collect_terms|combine|complete|decompose_product|"
		"distribute|drop_weight|eliminate_kronecker|eliminate_metric|epsilon_to_delta|"
		"evaluate|expand|expand_delta|expand_diracbar|expand_power|factor_in|factor_out|"
		"fierz|integrate_by_parts|join_gamma|keep_weight|lr_tensor|map_sympy|product_rule|"
		"reduce_delta|rename_dummies|rewrite_indices|simplify|sort_products|sort_sum|"
		"split_gamma|split_index|substitute|unwrap|vary|young_project_product|young_project_tensor";

	static const std::string properties =
		"Accent|AntiCommuting|AntiSymmetric|Commuting|CommutingAsProduct|CommutingAsSum|"
		"Coordinate|DAntiSymmetric|Depends|Derivative|Diagonal|DiracBar|EpsilonTensor|FilledTableau|GammaMatrix|ImplicitIndex|"
		"Indices|Integer|InverseMetric|KroneckerDelta|LaTeXForm|Metric|NonCommuting|PartialDerivative|RiemannTensor|SatisfiesBianchi|"
		"SelfAntiCommuting|SelfCommuting|SelfNonCommuting|SortOrder|Spinor|Symbol|Symmetric|Tableau|TableauSymmetry|WeightInherit";

	// Remove all tags that are currently set
	edit.get_buffer()->remove_all_tags(
		edit.get_buffer()->begin(),
		edit.get_buffer()->end()
	);

	std::string text = edit.get_buffer()->get_text();

	// Regex find things to highlight. Stolen with thanks from
	// https://wiki.python.org/moin/PyQt/Python%20syntax%20highlighting
	// Regexes are listed in order of priority such that anything that matches two items in the list
	// will only be highlighted according to the category that is higher up in this list
	tag_by_regex("comment", "(^|\\n)#[^\\n]*", text); // Single line comment
	tag_by_regex("string", "\"\"\"[^\"\\\\]*(\\\\.[^\"\\\\] * )*\"\"\"", text); // Triple double-quoted string
	tag_by_regex("string", "'''[^'\\\\]*(\\\\.[^'\\\\]*)*'''", text); // Triple single-quoted string
	tag_by_regex("string", "\"[^\"\\\\(\\r?\\n)]*(\\\\.[^\"\\\\] * )*\"", text); // Double-quoted string
	tag_by_regex("string", "'[^'\\\\(\\r?\\n)]*(\\\\.[^'\\\\]*)*'", text); // Single-quoted string
	tag_by_regex("keyword", "\\b(" + keywords + ")\\b", text); // Python keywords deliminated by word boundry
	tag_by_regex("function", "\\b(" + builtins + ")\\b", text); // Python builtins deliminated by word boundry
	tag_by_regex("algorithm", "\\b(" + algorithms + ")\\b", text); // Cadabra algorithms deliminated by word boundry
	tag_by_regex("property", "\\b(" + properties + ")\\b", text); // Cadabra properties deliminated by word boundry
	tag_by_regex("operator", "=|==|!=|<|<=|>|>=|\\+|-|\\*|\\/|\\/\\/|\\%|\\*\\*|\\+=|-=|\\*=|\\/=|\\%=|\\^|\\||\\&|\\~|>>|<<", text); // Python operators
	tag_by_regex("brace", "(\\{|\\}|\\(|\\)|\\[|\\])", text); // Braces
	tag_by_regex("maths", "\\$[^\\$]+\\$", text); // Latex-style inline maths
	tag_by_regex("number", "\\b[+-]?[0-9]+[lL]?\\b", text); // Integers 
	tag_by_regex("number", "\\b[+-]?0[xX][0-9A-Fa-f]+[lL]?\\b", text); // Hexadecimals
	tag_by_regex("number", "\\b[+-]?[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?\\b", text); // Floats
}

void CodeInput::highlight_latex()
{
	// Remove all tags that are currently set
	edit.get_buffer()->remove_all_tags(
		edit.get_buffer()->begin(),
		edit.get_buffer()->end()
	);

	std::string text = edit.get_buffer()->get_text();

	tag_by_regex("comment", "%[^\\n]*", text); // Single line comment
	tag_by_regex("maths", "\\$[^\\$]+\\$", text); // Inline maths
	tag_by_regex("parameter", "\\{(\\w)+\\}", text); // Curly-brace paramater list
	tag_by_regex("parameter", "\\[[^\\[\\]]+\\]", text); // Square-brace parameter list
	tag_by_regex("command", "\\\\(\\w)+", text); // Command 
	tag_by_regex("number", "\\b[+-]?[0-9]+[lL]?\\b", text); // Integers 
}

void CodeInput::enable_highlighting(DataCell::CellType cell_type, const Prefs& prefs)
{
	std::string map_idx;
	void (CodeInput::*callback)()=0;

	switch (cell_type)
	{
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
