
#pragma once

#include <gtkmm/window.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/box.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/spinner.h>
#include <gtkmm/label.h>
// #include <gtkmm/stock.h>
#include <gtkmm/button.h>
#include <gtkmm/builder.h>
#include <gtkmm/cssprovider.h>
// #include <gtkmm/toolbar.h>
#include <glibmm/dispatcher.h>
#include <giomm/settings.h>
#include <giomm/actiongroup.h>

#include <thread>
#include <mutex>

#include "nlohmann/json.hpp"

#include "DocumentThread.hh"
#include "ComputeThread.hh"
#include "Console.hh"
#include "GUIBase.hh"
#include "NotebookCanvas.hh"
#include "../common/TeXEngine.hh"
#include "DiffViewer.hh"

class Cadabra;

namespace cadabra {

	/// \ingroup frontend
	///
	/// Each notebook has one main window which controls it. It has a menu bar, a
	/// status pane and one or more panels that represent a view on the document.

	class NotebookWindow : public Gtk::ApplicationWindow, public DocumentThread, public GUIBase {
		public:
			NotebookWindow(Cadabra *, bool read_only=false, std::string geometry="", std::string title="");
			~NotebookWindow();

			// Virtual functions from GUIBase.

			virtual void   add_cell(const DTree&, DTree::iterator, bool) override;
			virtual void   remove_cell(const DTree&, DTree::iterator) override;
			virtual void   remove_all_cells() override;
			virtual void   update_cell(const DTree&, DTree::iterator) override;
			virtual void   position_cursor(const DTree&, DTree::iterator, int pos) override;
			virtual size_t get_cursor_position(const DTree&, DTree::iterator) override;
			virtual void   hide_visual_cells(DTree::iterator it) override;
			virtual void   dim_output_cells(DTree::iterator it) override;

			void           select_range(const DTree&, DTree::iterator, int start, int len);

			// Implementations of the functions which the compute thread will
			// call directly. If these need to modify the GUI, they need to do
			// so by calling one of the dispatchers.
			virtual void on_connect() override;
			virtual void on_disconnect(const std::string&) override;
			virtual void on_network_error() override;
			virtual void on_kernel_runstatus(bool) override;
			virtual void process_data() override;

			// TeX stuff
			TeXEngine        engine;
			double           scale; // total scale factor (hdpi and textscale)
			double           display_scale; // hdpi scale only

			// General setup.
			void on_realize() override;
			
			// Handler for vertical scrollbar changes. This gets connected to
			// all the `NotebookCanvas::scroll_event` signals.
			bool on_scroll_changed();

			// Handler for SliderView change events.
			void on_slider_changed(std::string variable, double value);
			
			// When something inside the large notebook canvas changes, we need
			// to make sure that the current cell stays into view (if we are
			// editing that cell). We can only do that once all size info is
			// known, which is when the scrolledwindow gets its size_allocate
			// signal. Here's the handler for it.
			void on_scroll_size_allocate(Gtk::Allocation&);

			// Ensure that the current cell is visible. This will assume
			// that all size allocations of widgets inside the scrolled window
			// have been made; it only does scrolling, based on the current
			// allocations. Calls `scroll_cell_into_view`.
			void scroll_current_cell_into_view();

			// Ensure that the indicated cell is visible.
			void scroll_cell_into_view(DTree::iterator cell);

			void set_name(const std::string&);
			void set_title_prefix(const std::string&);

			void refresh_highlighting();
			void on_help_register();

			void set_statusbar_message(const std::string& message = "", int line = -1, int col = -1);

			/// Functionality for the diff viewer.
			void select_git_path();
			void compare_to_file();
			void compare_git_latest();
			void compare_git_choose();
			void compare_git_specific();
			void compare_git(const std::string& commit_hash);
			std::string run_git_command(const std::string& args);

			virtual void load_from_string(const std::string& notebook_contents) override;
			virtual void set_compute_thread(ComputeThread* compute) override;

			virtual void on_interactive_output(const nlohmann::json& msg) override;
			virtual void set_progress(const std::string& msg, int cur_step, int total_steps) override;
		protected:
			virtual bool on_key_press_event(GdkEventKey*) override;
			virtual bool on_delete_event(GdkEventAny*) override;
			virtual bool on_configure_event(GdkEventConfigure *cfg) override;
			virtual bool on_unhandled_error(const std::exception& err) override;

			bool handle_outbox_select(GdkEventButton *, DTree::iterator it);
			DTree::iterator selected_cell;
			void unselect_output_cell();
			void on_outbox_copy(Glib::RefPtr<Gtk::Clipboard> refClipboard, DTree::iterator it);

		private:
			Glib::RefPtr<Cadabra> cdbapp;

			std::vector<Glib::RefPtr<Gio::SimpleAction>> default_actions;

			// Main handler which fires whenever the Client object signals
			// that the document is changing or the network status is modified.
			// Runs on the GUI thread.

			Glib::Dispatcher dispatcher;

			// GUI elements.

			Glib::RefPtr<Gio::SimpleActionGroup> actiongroup;
			Glib::RefPtr<Gtk::Builder>           uimanager;

			Gtk::Box                       topbox;
			Gtk::Box                       toolbar;
			Gtk::Button                    tool_open, tool_save, tool_save_as;
			Gtk::Button                    tool_run, tool_run_to, tool_stop, tool_restart, tool_restart_and_run_all;		
			Gtk::Box                       supermainbox;
			Gtk::Paned                     dragbox;
			Gtk::Box                       mainbox;
			Gtk::SearchBar                 searchbar;
			Gtk::Box                       search_hbox;
			Gtk::SearchEntry               searchentry;
			Gtk::CheckButton               search_case_insensitive;
			Gtk::Label                     search_result;
			Gtk::Box                       statusbarbox;

			Console console;
			Gtk::Dialog console_win;

			std::unique_ptr<DiffViewer> diffviewer;

			// All canvasses which are stored in the ...
			// These pointers are managed by gtkmm.
			std::vector<NotebookCanvas *>  canvasses;
			int                            current_canvas;

			// Buttons
			//         Gtk::Button                    b_kill, b_run, b_run_to, b_run_from, b_help, b_stop, b_undo, b_redo;

			// Status bar
			Gtk::ProgressBar               progressbar;
			Gtk::Spinner                   kernel_spinner;
			bool                           kernel_spinner_status;
			Gtk::Label                     status_label, kernel_label, top_label;

			// GUI data which is the autoritative source for things displayed in
			// the status bars declared above. These strings are filled on the
			// compute thread and then updated into the gui on the gui thread.

			std::mutex                     status_mutex;
			std::string                    status_string, kernel_string, progress_string;
			double                         progress_frac;
			int                            status_line, status_col;

			// Functions which get called on the compute thread can signal to
			// the GUI thread that elements need to be updated, by sending
			// signals using the following dispatchers.
			Glib::Dispatcher               dispatch_update_status, dispatch_refresh, dispatch_tex_error;

			// Update the status line and progress bar. This should only
			// be called on the GUI thread, so typically gets called
			// indirectly by calling `dispatch_update_status.emmit()`
			// from the compute thread, which calls this function.
			void                           update_status();

			// Run the TeX engine on a separate thread, then call
			// `dispatch_refresh` to update the display.
			void tex_run_async();

			// Name and modification data.
			void             update_title();
			void             set_stop_sensitive(bool);
			std::string      name, title_prefix, geometry_string;
			bool             modified, read_only;

			// Menu and button callbacks.
			void on_file_new();
			void on_file_open();
			void on_file_close();
			void on_file_save();
			void on_file_save_as();
			void on_file_save_as_jupyter();
			void on_file_export_html();
			void on_file_export_html_segment();
			void on_file_export_latex();
			void on_file_export_python();
			void on_file_quit();
			bool quit_safeguard(bool quit);
			bool on_first_redraw();

			void on_edit_undo(const Glib::VariantBase&);
			void on_edit_redo(const Glib::VariantBase&);
			void on_edit_copy(const Glib::VariantBase&);
			void on_edit_paste();
			void on_edit_insert_above();
			void on_edit_insert_below();
			void on_edit_delete();
			void on_edit_split();
			void on_edit_cell_is_latex();
			void on_edit_cell_is_python();
			void on_ignore_cell_on_import();
			void on_edit_find();

			void on_view_split();
			void on_view_close(const Glib::VariantBase&);

			void on_run_cell();
			void on_run_runtocursor();
			void on_run_stop();

			void on_prefs_set_cv(int vis);
			void on_prefs_auto_close_latex(const Glib::VariantBase& vb);
			void on_prefs_hide_input_cells(const Glib::VariantBase& vb);			
			void on_prefs_font_size(int num);
			void on_prefs_highlight_syntax(bool on);
			void on_prefs_microtex(bool on);
			void on_prefs_choose_colours();
			void on_prefs_use_defaults();

			void on_tools_options();
			void on_tools_clear_cache();

			void on_help_about();
			void on_help() const;

			void on_kernel_restart();
			void on_kernel_restart_and_run_all();			

			/// Search handling.
			void on_search_text_changed();

			/// Clipboard handling
			void on_clipboard_get(Gtk::SelectionData&, guint info);
			void on_clipboard_clear();
			std::string clipboard_txt, clipboard_cdb;

			// FIXME: move to DocumentThread
			std::string save(const std::string& fn) const;

			/// Todo deque processing logic. This gets called by the dispatcher, but it
			/// is also allowed to call this from within NotebookWindow itself. The important
			/// thing is that it is run on the GUI thread.
			/// This is a wrapper around `Document::process_action_queue`, to set the
			/// spinner status and handle crashes.
			void process_todo_queue();

			/// Refresh the display after a TeX engine run has completed. The TeX
			/// engine is run on a different thread so as to not block the UI, and
			/// on completion triggers `dispatcher_refresh`, which calls this function
			/// on the main thread.
			void refresh_after_tex_engine_run();

			/// Handle a TeX error which occurred on a threaded TeX run (activated by
			/// `tex_run_async`) and is stored in `tex_error_string`.
			void handle_thread_tex_error();

			void on_crash_window_closed(int);
			bool crash_window_hidden;

			// The following are handlers that get called when the cell
			// gets focus, the content of a cell is changed, the user
			// requests to run it (shift-enter). The last two parameters are
			// always the cell in the DTree and the canvas number.

			bool cell_got_focus(DTree::iterator, int);
			bool cell_toggle_visibility(DTree::iterator it, int);
			bool cell_content_insert(const std::string&, int, DTree::iterator, int);
			bool cell_content_erase(int, int, DTree::iterator, int);
			bool cell_content_execute(DTree::iterator, bool shift_enter_pressed);
			bool cell_content_changed(DTree::iterator it, int i);
			bool cell_complete_request(DTree::iterator it, int pos, int i);

			void interactive_execute();

			void resize_codeinput_texview_all(int width);
			void resize_codeinput_texview(DTree::iterator, int width);			

			// Handler for callbacks from TeXView cells.

			bool on_tex_error(const std::string&, DTree::iterator);
			bool on_copy_as_latex(const DTree::iterator it);
	
			void propagate_global_hide_flag();
			
			// Styling through CSS
			void                           load_css();
			Glib::RefPtr<Gtk::CssProvider> css_provider;
			Glib::RefPtr<Gio::Settings>    settings;
			void on_text_scaling_factor_changed(const std::string& key);

			int             last_configure_width;

			// Mutex to protect the variables below.
			std::recursive_mutex         tex_need_width_mutex;
			std::unique_ptr<std::thread> tex_thread;
			bool                         tex_running;
			int                          tex_need_width;
			std::string                  tex_error_string;

			std::pair<DTree::iterator, size_t> last_find_location;
			std::string                        last_find_string;

			bool  is_configured;         // have received and handled a configure event
			bool  run_all_after_restart; // queue notebook running when kernel comes back

			// We keep references to a few menu actions so we can
			// enable/disable them at runtime.
			Glib::RefPtr<Gio::SimpleAction> action_copy, action_undo, action_redo,
				action_paste, action_view_close, action_fontsize, action_highlight,
				action_auto_close_latex, action_hide_input_cells,
				action_stop, action_register, action_console, action_microtex;

			// Transition animations.
#if GTKMM_MINOR_VERSION>=10
			std::vector<Gtk::Revealer *> to_reveal;
#endif
			bool idle_handler();
		};

	};
