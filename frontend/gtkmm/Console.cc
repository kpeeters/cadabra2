#include <iostream>
#include <gdk/gdkkeysyms.h>
#include "Console.hh"
#include "GtkCompat.hh"
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
#ifdef USE_GTK4
	// GTK4 cannot invoke another widget's default key handler, so instead of
	// having the visible view forward events here, this proxy takes the
	// keyboard focus itself and edits its own buffer natively. Only the keys
	// the console interprets are intercepted; everything else is ordinary
	// TextView editing, mirrored to the view when the buffer changes.
	key_controller = Gtk::EventControllerKey::create();
	key_controller->signal_key_pressed().connect(
	   [this](guint keyval, guint /* keycode */, Gdk::ModifierType state) {
		return handle_key_press(keyval, state);
		}, false);
	add_controller(key_controller);

	get_buffer()->signal_changed().connect([this]() {
		if(updating_buffer) return;
		remember_partial_input();
		sync_to_view();
		});
#endif
	}

/// Handle the keys the console interprets itself: Return submits, Up/Down walk
/// the history. Returns true when the key was consumed; false means the
/// TextView's own editing should run. Shared between gtkmm-3's
/// on_key_press_event vfunc and GTK4's Gtk::EventControllerKey.

bool Console::TextViewProxy::handle_key_press(guint keyval, Gdk::ModifierType /* state */)
	{
	auto buf = get_buffer();

	parent.scroll_to_bottom();

	// Run command if return key is pressed
	if (keyval == GDK_KEY_Return) {
		std::string code = buf->get_text();
		set_text_quietly("");
		temp_in.clear();
		parent.send_input(code);
		history.push_back(code);
		if (history.size() > history_max_length)
			history.pop_front();
		history_ptr = history.end();
		return true;
		}

	if (keyval == GDK_KEY_Up) {
		// Move up the history
		if (history_ptr == history.begin()) {
			// Do nothing
			}
		else {
			--history_ptr;
			set_text_quietly(*history_ptr);
			}
		// Note: temp_in is deliberately *not* refreshed here; it holds the
		// partially typed line that Down restores at the end of the history.
		sync_to_view();
		return true;
		}

	if (keyval == GDK_KEY_Down) {
		// Move down the history
		if (history_ptr == history.end()) {
			set_text_quietly("");
			}
		else {
			++history_ptr;
			if (history_ptr == history.end())
				set_text_quietly(temp_in);
			else
				set_text_quietly(*history_ptr);
			}
		sync_to_view();
		return true;
		}

	return false;
	}

/// Replace the buffer contents without it counting as user editing.

void Console::TextViewProxy::set_text_quietly(const Glib::ustring& text)
	{
	updating_buffer = true;
	get_buffer()->set_text(text);
	updating_buffer = false;
	}

/// Remember the partially typed line, so that walking down off the end of the
/// history can restore it.

void Console::TextViewProxy::remember_partial_input()
	{
	if (history_ptr == history.end())
		temp_in = get_buffer()->get_text();
	}

/// Mirror this (model) buffer, and its selection, into the parent's visible
/// buffer.

void Console::TextViewProxy::sync_to_view()
	{
	auto buf = get_buffer();

	// Update the position of the insertion point
	Gtk::TextBuffer::iterator range_start, range_end;
	buf->get_selection_bounds(range_start, range_end);

	// Update the parent's buffer to reflect the keystroke
	parent.set_input(
	   buf->get_text(),
	   std::distance(buf->begin(), range_start),
	   std::distance(buf->begin(), range_end)
	);
	}

#ifndef USE_GTK4

bool Console::TextViewProxy::on_key_press_event(GdkEventKey* key_event)
	{
	if (handle_key_press(key_event->keyval, static_cast<Gdk::ModifierType>(key_event->state)))
		return true;

	// Let the (hidden) TextView edit its own buffer, then mirror the result.
	bool ret = Gtk::TextView::on_key_press_event(key_event);
	remember_partial_input();
	sync_to_view();

	// Defensive: the proxy must never become visible.
	set_visible(false);

	return ret;
	}

#endif

Console::Console(sigc::slot<void()> run_slot)
	: needs_focus(false)
	, input(*this)
	{
	set_name("Console");
	buffer = tv.get_buffer();

	input_begin = buffer->create_mark(buffer->begin());
	prompt_begin = buffer->create_mark(buffer->begin());

	run.connect(run_slot);
	dispatch_message.connect(sigc::mem_fun(*this, &Console::process_message_queue));

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

	win.set_policy(cdb::POLICY_NEVER, cdb::POLICY_ALWAYS);

	tv.set_wrap_mode(cdb::WRAP_WORD_CHAR);
	tv.set_hexpand(true);
	tv.set_vexpand(true);
	tv.set_monospace(true);

	// NOTE: tv is deliberately left editable. Making it non-editable would be
	// an improvement -- the scrollback is currently protected only by the key
	// handler below swallowing every keystroke, so paste and drag-and-drop can
	// still corrupt it -- but it is not needed for the port, so it is left for
	// the console rework rather than risked here.

#ifndef USE_GTK4
	tv.signal_key_press_event().connect(
	   sigc::mem_fun(input, &TextViewProxy::on_key_press_event),
	   false);

	tv.signal_size_allocate().connect([this](Gtk::Allocation& ) {
		scroll_to_bottom();
		});
#endif

	cdb::add_child(*this, input);
	cdb::add_child(*this, win);
	cdb::add_child(win, tv);

#ifdef USE_GTK4
	// The proxy takes the keyboard focus rather than being fed forwarded
	// events, so it has to be a real, focusable widget; it is kept visually
	// out of the way instead of hidden.
	input.set_size_request(0, 0);
	input.set_can_focus(true);
	tv.set_can_focus(false);
#else
	// The proxy is deliberately left visible here. It must be realized before
	// it can insert any text: Gtk::TextView routes key presses through an IM
	// context, and an IM context with no client window commits nothing. So it
	// is shown until the first key press, which hides it again (see
	// on_key_press_event) -- hiding unmaps but does not unrealize, so it keeps
	// working from then on.
	//
	// This was previously the accidental result of show_all_children() undoing
	// an earlier set_visible(false); it is now explicit. Hiding it here instead
	// makes the console silently reject all typed input.
#endif

	set_sensitive(false);
	}

Gtk::Widget& Console::focus_widget()
	{
#ifdef USE_GTK4
	return input;
#else
	return tv;
#endif
	}

#ifdef USE_GTK4
void Console::size_allocate_vfunc(int width, int height, int baseline)
	{
	Gtk::Box::size_allocate_vfunc(width, height, baseline);
	scroll_to_bottom();
	}
#endif

void Console::initialize()
	{
	std::string welcome_message =
		"Cadabra v" CADABRA_VERSION_SEM
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
		str->add_data(dec.c_str(), dec.size(), nullptr);
		auto pixbuf = Gdk::Pixbuf::create_from_stream_at_scale(str, 400, -1, true);

		buffer->insert(buffer->get_iter_at_mark(mark), "\n");
		cdb::insert_image(buffer, buffer->get_iter_at_mark(mark), pixbuf);
	}
	catch (const std::exception& err) {
		insert_text(cell_id, err.what(), error_tag);
	}
	}

void Console::insert_tex(uint64_t cell_id, const std::shared_ptr<TeXEngine::TeXRequest>& content)
{
	auto& mark = cells.at(cell_id);
	auto pixbuf = Gdk::Pixbuf::create_from_data(
		content->image().data(), cdb::COLORSPACE_RGB,
		true,
		8,
		content->width(), content->height(),
		4 * content->width());

	buffer->insert(buffer->get_iter_at_mark(mark), "\n");
	cdb::insert_image(buffer, buffer->get_iter_at_mark(mark), pixbuf);
}

void Console::process_message_queue()
{
	using namespace nlohmann;

	if (focus_widget().has_focus())
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
				tex_engine.set_geometry(get_width());
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
		focus_widget().grab_focus();
	needs_focus = false;
}

