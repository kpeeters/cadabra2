#pragma once

#include <queue>
#include <list>
#include <gtkmm.h>
#include "nlohmann/json.hpp"
#include "../common/TeXEngine.hh"
#include "DataCell.hh"

namespace cadabra {
	class Console;

	// Interactive Console for Cadabra. Deived from Gtk::Box so can be packed into most
	// other widgets.
	class Console : public Gtk::Box {
		public:
			enum class Position : int {
				Hidden,
				DockedH,
				DockedV,
				Floating
			};

			// run_slot is the callback which should be run when the console sends input to
			// be run. This callback should call console.grab_input to get a line of input,
			// process it and then send the output block back to the console.
			Console(sigc::slot<void> run_slot);

			// Display welcome message and first prompt
			void initialize();

			// Process a line of code, either creating a new prompt asking for
			// more input or adding a string to the run_queue and calling the run_slot callback
			void send_input(const std::string& code);

			// Grab a line of input, this should only be called fom the run_slot callback.
			// The id parameter will be set to a unique id which should be the parent_id of the
			// block when it is sent back to the console
			std::string grab_input(uint64_t& id);

			// Add a message to the console's message queue and ask it to process the message queue
			void signal_message(const nlohmann::json& msg);

			// Scroll the console to the latest output
			void scroll_to_bottom();

		private:
			// Hidden proxy TextView which stores the user input by intercepting key press
			// events from the main console TextView
			class TextViewProxy : public Gtk::TextView {
			public:
				TextViewProxy(Console& parent);
				bool on_key_press_event(GdkEventKey* key_event) override;
				size_t history_max_length;

			private:
				Console& parent;
				std::string temp_in;
				std::list<std::string> history;
				std::list<std::string>::iterator history_ptr;
			};

			// Set the current prompt to particular text
			void set_input(const Glib::ustring& line, size_t range_start = std::string::npos, size_t range_end = std::string::npos);

			// Create a new prompt
			void prompt(bool continuation, bool newline = false);

			// Allocate a mark to a cell
			void create_cell(uint64_t parent_id, uint64_t cell_id);

			// Return the current server cell (pseudo-input cell for inputs received from the server) or
			// create one and return it if none exist
			uint64_t get_server_cell();

			void insert_text(uint64_t cell_id, const std::string& text, const Glib::RefPtr<Gtk::TextTag>& tag);
			void insert_graphic(uint64_t cell_id, const std::string& bytes);
			void insert_tex(uint64_t cell_id, const std::shared_ptr<TeXEngine::TeXRequest>& content);

			// Handle current incoming messages
			void process_message_queue();

			virtual bool on_configure_event(GdkEventConfigure* cfg) override;

			Glib::Dispatcher dispatch_message; // callback attached to process_message_queue
			Glib::Dispatcher run; // initialized by run_slot, callback when cell needs running

			bool needs_focus; // True if console should grab focus on output
			Glib::RefPtr<Gtk::TextBuffer> buffer; // Reference to tv.get_buffer()
			std::string collect; // Collected input if prompt is in continuation mode
			std::queue<nlohmann::json> message_queue; // Current messages needing processing
			std::queue<std::pair<std::string, Glib::RefPtr<Gtk::TextMark>>> run_queue; // Cells waiting to be run, .first is input string and .second is the input cell
			TeXEngine tex_engine; // Engine for compiling TeX outputs
			std::map<uint64_t, Glib::RefPtr<Gtk::TextMark>> cells; // Storage of currently displayed cells
			uint64_t server_cell_id; // ID of current server cell, or 0 if invalidated
			Glib::RefPtr<Gtk::TextMark> input_begin; // Beginning of input
			Glib::RefPtr<Gtk::TextMark> prompt_begin; // Beginning of input prompt
			Glib::RefPtr<Gtk::TextTag> prompt_tag, input_tag, output_tag, warning_tag, error_tag;
			TextViewProxy input; // Input prompt
			Gtk::TextView tv; // Main view
			Gtk::ScrolledWindow win; // Main window
		};

	}
