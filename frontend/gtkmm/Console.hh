#pragma once

#include <gtkmm.h>
#include <json/json.h>
#include <queue>
#include <list>

namespace cadabra {
	class Console;

	class TextViewProxy : public Gtk::TextView {
		public:
			TextViewProxy(Console& parent);

			size_t history_max_length;

			bool on_key_press_event(GdkEventKey* key_event) override;

		private:
			Console& parent;

			std::string temp_in;
			std::list<std::string> history;
			std::list<std::string>::iterator history_ptr;
		};

	class Console : public Gtk::Box {
		public:
			Console(sigc::slot<void> run_slot);
			~Console();

			void initialize();

			void set_input(const Glib::ustring& line, size_t range_start = std::string::npos, size_t range_end = std::string::npos);
			std::string grab_input();
			void send_input(const std::string& code);
			void signal_message(const Json::Value& msg);

			void set_height(int px);

			void scroll_to_bottom();
			uint64_t get_id() const;
			Glib::RefPtr<Gtk::TextBuffer> get_buffer();

		private:
			void insert_text(const std::string& text, Glib::RefPtr<Gtk::TextTag> tag);
			void insert_graphic(const std::string& bytes);
			void prompt(bool continuation, bool newline = false);

			void process_message_queue();

			Glib::Dispatcher dispatch_message;
			Glib::Dispatcher run;
			uint64_t id_;

			bool needs_focus;
			std::string collect;
			std::queue<Json::Value> message_queue;
			std::queue<std::string> run_queue;
			Gtk::ScrolledWindow win;
			TextViewProxy input;
			Gtk::TextView tv;
			Glib::RefPtr<Gtk::TextBuffer::Mark> input_begin, prompt_begin;
			Glib::RefPtr<Gtk::TextTag> input_tag, output_tag, warning_tag, error_tag;
		};

	}
