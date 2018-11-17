#include <iostream>
#include <gdk/gdkkeysyms.h>
#include "Console.hh"
#include "Config.hh"
#include <internal/uuid.h>

using namespace cadabra;

bool is_greedy(const std::string& str) 
{
	auto last = str.find_last_not_of(" \t");

	if (last == std::string::npos)
		return false;
	return (str[last] == ':' || str[last] == '\\');
}

bool is_empty(const std::string& str)
{
	return str.find_first_not_of(" \t") == std::string::npos;
}

TextViewProxy::TextViewProxy(Console& parent)
	: history_max_length(50)
	, parent(parent)
	, history_ptr(history.begin())
{

}

bool TextViewProxy::on_key_press_event(GdkEventKey* key_event)
{
	auto buf = get_buffer();

	parent.scroll_to_bottom();

	// Run command if return key is pressed
	if (key_event->keyval == GDK_KEY_Return) {
		std::string code = buf->get_text();
		buf->set_text("");
		temp_in.clear();
		parent.send_input(code);
		history.push_back(code);
		if (history.size() > history_max_length)
			history.pop_front();
		history_ptr = history.end();
		return true;
	}

	bool ret;

	if (key_event->keyval == GDK_KEY_Up) {
		// Move up the history
		if (history_ptr == history.begin()) {
			// Do nothing
		}
		else {
			--history_ptr;
			buf->set_text(*history_ptr);
		}
		ret = true;
	}
	else if (key_event->keyval == GDK_KEY_Down) {
		// Move down the history
		if (history_ptr == history.end()) {
			buf->set_text("");
		}
		else {
			++history_ptr;
			if (history_ptr == history.end())
				buf->set_text(temp_in);
			else
				buf->set_text(*history_ptr);
		}
		ret = true;
	}
	else {
		// Process the keystroke
		ret = Gtk::TextView::on_key_press_event(key_event);
		if (history_ptr == history.end())
			temp_in = buf->get_text();
	}

	// Update the position of the insertion point
	Gtk::TextBuffer::iterator range_start, range_end;
	buf->get_selection_bounds(range_start, range_end);

	// Update the parent's buffer to reflect the keystroke
	parent.set_input(
		buf->get_text(), 
		std::distance(buf->begin(), range_start),
		std::distance(buf->begin(), range_end)
	);

	hide();

	return ret;
}

Console::Console(sigc::slot<void> run_slot)
	: id_(generate_uuid())
	, needs_focus(false)
	, input(*this)
{
   set_name("Console");

	input_begin = get_buffer()->create_mark(get_buffer()->begin());
	prompt_begin = get_buffer()->create_mark(get_buffer()->begin());

	run.connect(run_slot);
	dispatch_message.connect(sigc::mem_fun(this, &Console::process_message_queue));

	input_tag = Gtk::TextTag::create();
	input_tag->property_foreground() = "black";
	tv.get_buffer()->get_tag_table()->add(input_tag);

	output_tag = Gtk::TextTag::create();
	output_tag->property_foreground() = "dim gray";
	tv.get_buffer()->get_tag_table()->add(output_tag);

	warning_tag = Gtk::TextTag::create();
	warning_tag->property_foreground() = "orange";
	tv.get_buffer()->get_tag_table()->add(warning_tag);

	error_tag = Gtk::TextTag::create();
	error_tag->property_foreground() = "red";
	tv.get_buffer()->get_tag_table()->add(error_tag);

	win.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);

	tv.set_wrap_mode(Gtk::WRAP_WORD_CHAR);
	tv.set_hexpand(true);
	tv.set_monospace(true);

	tv.signal_key_press_event().connect(
		sigc::mem_fun(&input, &TextViewProxy::on_key_press_event),
		false);

	tv.signal_size_allocate().connect([this](Gtk::Allocation& alloc) {
		scroll_to_bottom();
	});

	add(input);
	input.hide();
	add(win);
	win.add(tv);
	show_all_children();

	set_sensitive(false);
}

void Console::scroll_to_bottom()
{
	Glib::RefPtr<Gtk::Adjustment> adj = win.get_vadjustment();
	adj->set_value(adj->get_upper());
}

Console::~Console()
{

}

void Console::initialize()
{
	send_input("print(\"Cadabra v"
		CADABRA_VERSION_MAJOR "." CADABRA_VERSION_MINOR "." CADABRA_VERSION_PATCH
		" Interactive Console\\n"
		"For more information, type help(console)\")");
}

void Console::set_height(int px)
{
	set_size_request(-1, px);
	win.set_min_content_height(px);
#if GTKMM_MINOR_VERSION>=22	
	win.set_max_content_height(px);
#endif
}

void Console::send_input(const std::string& code)
{
	bool send = false;

	if (collect.empty()) {
		collect += code;
		if (is_greedy(code))
			collect += '\n';
		else
			send = true;
	}
	else {
		if (is_empty(code))
			send = true;
		else
			collect += code + '\n';
	}

	if (send) {
		prompt(false, true);
		needs_focus = true;
		run_queue.push(collect);
		collect.clear();
		run();
	}
	else {
		prompt(true, true);
	}
}

std::string Console::grab_input()
{
	std::string ret = run_queue.front();
	run_queue.pop();
	return ret;
}

void Console::signal_message(const Json::Value& message)
{
	message_queue.push(message);
	dispatch_message();
}

void Console::process_message_queue()
{
	if (tv.has_focus())
		needs_focus = true;
	set_sensitive(false);

	while (!message_queue.empty()) {
		const Json::Value& msg = message_queue.front();

		std::string msg_type = msg["msg_type"].asString();
		if (msg_type.empty())
			msg_type = msg["header"]["msg_type"].asString();
		std::string content = msg["content"]["output"].asString();
		bool from_server = msg["header"]["from_server"].asBool();

		if (from_server) {
			std::string in = input.get_buffer()->get_text();
			set_input("<received input from server>");
			prompt(false, true);
			set_input(in);
		}
		needs_focus |= !from_server;
	
		// Process message
		if (msg_type == "exit") {
			insert_text("Restarting kernel...", warning_tag);
		}
		else if (
			msg_type == "output" ||
			msg_type == "verbatim" ||
			msg_type == "input_form" ||
			msg_type == "csl_out") {
			insert_text(content, output_tag);
		}
		else if (msg_type == "csl_clear") {
			get_buffer()->set_text("");
			prompt(false);
		}
		else if (msg_type == "latex_view") {
			// Don't handle latex output
		}
		else if (msg_type == "error") {
			insert_text(content, error_tag);
		}
		else if (msg_type == "warning") {
			insert_text(content, warning_tag);
		}
		else if (msg_type == "image_png") {
			insert_graphic(content);
		}
		else {
			insert_text("cadabra-client (console): received cell we did not expect: " + msg_type, error_tag);
		}

		message_queue.pop();
	}

	set_sensitive(true);
	if (needs_focus)
		tv.grab_focus();
	needs_focus = false;
}

Glib::RefPtr<Gtk::TextBuffer> Console::get_buffer()
{
	return tv.get_buffer();
}

void Console::set_input(const Glib::ustring& line, size_t range_start, size_t range_end)
{
	get_buffer()->erase(get_buffer()->get_iter_at_mark(input_begin), get_buffer()->end());
	get_buffer()->insert_with_tag(get_buffer()->get_iter_at_mark(input_begin), line, input_tag);

	if (range_start == std::string::npos)
		range_start = line.size();
	if (range_end == std::string::npos)
		range_end = line.size();

	auto it_start = get_buffer()->get_iter_at_mark(input_begin);;
	auto it_end = it_start;
	it_start.forward_chars(range_start);
	it_end.forward_chars(range_end);

	get_buffer()->select_range(it_start, it_end);
}

void Console::insert_text(const std::string& text, Glib::RefPtr<Gtk::TextTag> tag)
{
	if (!text.empty() && text.back() != '\n')
		get_buffer()->insert_with_tag(get_buffer()->get_iter_at_mark(prompt_begin), text + '\n', tag);
	else
		get_buffer()->insert_with_tag(get_buffer()->get_iter_at_mark(prompt_begin), text, tag);
}

void Console::insert_graphic(const std::string& bytes)
{
	// Load bytes into a pixbuf
	auto str = Gio::MemoryInputStream::create();
	std::string dec = Glib::Base64::decode(bytes);
	str->add_data(dec.c_str(), dec.size());
	auto pixbuf = Gdk::Pixbuf::create_from_stream_at_scale(str, 400, -1, true);
	
	// Display image
	auto next = get_buffer()->insert_pixbuf(get_buffer()->get_iter_at_mark(prompt_begin), pixbuf);
	get_buffer()->insert(next, "\n");

}

void Console::prompt(bool continuation, bool newline)
{
	if (newline)
		get_buffer()->insert(get_buffer()->end(), "\n");

	prompt_begin = get_buffer()->create_mark(get_buffer()->end());
	get_buffer()->insert_with_tag(get_buffer()->end(), continuation ? "... " : ">>> ", input_tag);
	input_begin = get_buffer()->create_mark(get_buffer()->end());
}

uint64_t Console::get_id() const
{
	return id_;
}
