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

Console::TextViewProxy::TextViewProxy(Console& parent)
	: history_max_length(50)
	, parent(parent)
	, history_ptr(history.begin())
	{

	}

bool Console::TextViewProxy::on_key_press_event(GdkEventKey* key_event)
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
	: needs_focus(false)
	, input(*this)
	{
	set_name("Console");
	buffer = tv.get_buffer();

	input_begin = buffer->create_mark(buffer->begin());
	prompt_begin = buffer->create_mark(buffer->begin());

	run.connect(run_slot);
	dispatch_message.connect(sigc::mem_fun(this, &Console::process_message_queue));

	auto tag_table = tv.get_buffer()->get_tag_table();
	input_tag = Gtk::TextTag::create();
	input_tag->property_foreground() = "black";
	tag_table->add(input_tag);

	output_tag = Gtk::TextTag::create();
	output_tag->property_foreground() = "dim gray";
	tag_table->add(output_tag);

	warning_tag = Gtk::TextTag::create();
	warning_tag->property_foreground() = "orange";
	tag_table->add(warning_tag);

	error_tag = Gtk::TextTag::create();
	error_tag->property_foreground() = "red";
	tag_table->add(error_tag);

	prompt_tag = Gtk::TextTag::create();
	prompt_tag->property_foreground() = "blue";
	tag_table->add(prompt_tag);

	win.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);

	tv.set_wrap_mode(Gtk::WRAP_WORD_CHAR);
	tv.set_hexpand(true);
	tv.set_vexpand(true);
	tv.set_monospace(true);

	tv.signal_key_press_event().connect(
	   sigc::mem_fun(&input, &TextViewProxy::on_key_press_event),
	   false);

	tv.signal_size_allocate().connect([this](Gtk::Allocation& ) {
		scroll_to_bottom();
		});

	add(input);
	input.hide();
	add(win);
	win.add(tv);
	show_all_children();

	set_sensitive(false);
	}

void Console::initialize()
	{
	std::string welcome_message =
		"Cadabra v" CADABRA_VERSION_MAJOR "." CADABRA_VERSION_MINOR "." CADABRA_VERSION_PATCH
		" Interactive Console\nFor more information, type help(console)\n";
	buffer->insert_with_tag(buffer->begin(), welcome_message, output_tag);
	server_cell_id = 0;
	prompt(false, true);
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
		server_cell_id = 0;
		auto oneback = buffer->insert(buffer->end(), "\n");
		oneback.backward_char();
		auto k = run_queue.emplace(collect, buffer->create_mark(oneback, false));
		collect.clear();
		prompt(false);
		needs_focus = true;
		run();
	}
	else {
		prompt(true, true);
	}
}

std::string Console::grab_input(uint64_t& id)
{
	const auto& cell = run_queue.front();
	id = generate_uuid<uint64_t>();
	auto input = cell.first;
	cells.emplace(id, cell.second);
	run_queue.pop();
	return input;
}

void Console::signal_message(const nlohmann::json& message)
{
	message_queue.push(message);
	dispatch_message();
}

void Console::scroll_to_bottom()
{
	Glib::RefPtr<Gtk::Adjustment> adj = win.get_vadjustment();
	adj->set_value(adj->get_upper());
}

void Console::set_input(const Glib::ustring& line, size_t range_start, size_t range_end)
{
	buffer->erase(buffer->get_iter_at_mark(input_begin), buffer->end());
	buffer->insert_with_tag(buffer->get_iter_at_mark(input_begin), line, input_tag);

	if (range_start == std::string::npos)
		range_start = line.size();
	if (range_end == std::string::npos)
		range_end = line.size();

	auto it_start = buffer->get_iter_at_mark(input_begin);;
	auto it_end = it_start;
	it_start.forward_chars(range_start);
	it_end.forward_chars(range_end);

	buffer->select_range(it_start, it_end);
}

void Console::prompt(bool continuation, bool newline)
{
	if (newline)
		buffer->insert(buffer->end(), "\n");

	prompt_begin = buffer->create_mark(buffer->end());
	buffer->insert_with_tag(buffer->end(), continuation ? ". " : "> ", prompt_tag);
	input_begin = buffer->create_mark(buffer->end());
}

void Console::create_cell(uint64_t parent_id, uint64_t cell_id)
	{
	if (cells.find(parent_id) == cells.end()) {
		// Create a zombie input cell
		std::string in = input.get_buffer()->get_text();
		set_input("<received input from " + std::to_string(parent_id) + ">\n");
		auto backone = buffer->end();
		backone.backward_char();
		parent_id = generate_uuid<uint64_t>();
		cells.emplace(parent_id, buffer->create_mark(backone, false));
		prompt(false, true);
		set_input(in);
		}
	auto it = buffer->insert(buffer->get_iter_at_mark(cells.at(parent_id)), " ");
	cells.emplace(cell_id, buffer->create_mark(it));
	}

uint64_t Console::get_server_cell()
{
	if (server_cell_id == 0) {
		server_cell_id = generate_uuid<uint64_t>();
		std::string in = input.get_buffer()->get_text();
		set_input("<received input from server>\n");
		auto backone = buffer->end();
		backone.backward_char();
		cells.emplace(server_cell_id, buffer->create_mark(backone, false));
		prompt(false, true);
		set_input(in);
	}
	return server_cell_id;
}


void Console::insert_text(uint64_t cell_id, const std::string& text, const Glib::RefPtr<Gtk::TextTag>& tag)
	{
	auto& mark = cells.at(cell_id);
	buffer->insert_with_tag(buffer->get_iter_at_mark(mark), "\n" + text, tag);
	}

void Console::insert_graphic(uint64_t cell_id, const std::string& bytes)
	{
	// Load bytes into a pixbuf
	try {
		auto& mark = cells.at(cell_id);
		auto str = Gio::MemoryInputStream::create();
		std::string dec = Glib::Base64::decode(bytes);
		str->add_data(dec.c_str(), dec.size());
		auto pixbuf = Gdk::Pixbuf::create_from_stream_at_scale(str, 400, -1, true);

		buffer->insert(buffer->get_iter_at_mark(mark), "\n");
		buffer->insert_pixbuf(buffer->get_iter_at_mark(mark), pixbuf);
	}
	catch (const std::exception& err) {
		insert_text(cell_id, err.what(), error_tag);
	}
	}

void Console::insert_tex(uint64_t cell_id, const std::shared_ptr<TeXEngine::TeXRequest>& content)
{
	auto& mark = cells.at(cell_id);
	auto pixbuf = Gdk::Pixbuf::create_from_data(
		content->image().data(), Gdk::COLORSPACE_RGB,
		true,
		8,
		content->width(), content->height(),
		4 * content->width());

	buffer->insert(buffer->get_iter_at_mark(mark), "\n");
	buffer->insert_pixbuf(buffer->get_iter_at_mark(mark), pixbuf);
}

void Console::process_message_queue()
{
	using namespace nlohmann;

	if (tv.has_focus())
		needs_focus = true;
	set_sensitive(false);

	while (!message_queue.empty()) {
		const json& msg = message_queue.front();
		json header = msg.value("header", json(json::value_t::object));
		json content = msg.value("content", json(json::value_t::object));

		std::string msg_type = msg.value("msg_type", header.value("msg_type", ""));
		std::string output = content.value("output", "");
		uint64_t parent_id = header.value<uint64_t>("parent_id", 0);
		uint64_t cell_id = header.value<uint64_t>("cell_id", 0);
		uint64_t source_id;

		bool from_server = header.value("from_server", false);
		if (from_server) {
			source_id = get_server_cell();
		}
		else {
			source_id = parent_id;
			needs_focus = true;
		}

		create_cell(source_id, cell_id);

		// Process message
		if (msg_type == "exit") {
			insert_text(cell_id, "Restarting kernel...", warning_tag);
		}
		else if (
			msg_type == "output" ||
			msg_type == "verbatim" ||
			msg_type == "input_form" ||
			msg_type == "csl_out") {
			insert_text(cell_id, output, output_tag);
		}
		else if (msg_type == "csl_warn") {
			insert_text(cell_id, "Warning: " + output, warning_tag);
		}
		else if (msg_type == "csl_clear") {
			buffer->set_text("");
			tex_engine.checkout_all();
			//cells.clear();
			prompt(false);
		}
		else if (msg_type == "latex_view") {
			try {
				auto request = tex_engine.checkin(output, "", "");
				tex_engine.set_geometry(get_allocation().get_width());
				tex_engine.convert_all();
				insert_tex(source_id, request);
			}
			catch (const std::exception& err) {
				insert_text(cell_id, err.what(), error_tag);
			}
		}
		else if (msg_type == "error") {
			insert_text(cell_id, output, error_tag);
		}
		else if (msg_type == "warning") {
			insert_text(cell_id, output, warning_tag);
		}
		else if (msg_type == "image_png") {
			insert_graphic(cell_id, output);
		}
		else {
			std::stringstream ss;
			ss << msg.dump(2) << std::endl;
			insert_text(cell_id, "cadabra-client (console): do not know how to handle message:\n" + msg.dump(2), error_tag);
		}

		message_queue.pop();
	}

	set_sensitive(true);
	if (needs_focus)
		tv.grab_focus();
	needs_focus = false;
}

bool Console::on_configure_event(GdkEventConfigure* cfg)
{
	bool res = Gtk::Box::on_configure_event(cfg);
	tv.set_size_request(cfg->width, cfg->height);
	return res;
}
