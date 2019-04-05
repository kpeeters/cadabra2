
#pragma once

#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/spinner.h>
#include <gtkmm/label.h>
#include <gtkmm/stock.h>
#include <gtkmm/button.h>
#include <gtkmm/uimanager.h>
#include <gtkmm/cssprovider.h>
#include <glibmm/dispatcher.h>
#include <giomm/settings.h>
#include <gtkmm/action.h>

#include <thread>
#include <mutex>

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

	class NotebookWindow : public Gtk::Window, public DocumentThread, public GUIBase {
		public:
			NotebookWindow(Cadabra *, bool read_only=false);
			~NotebookWindow();

			// Virtual functions from GUIBase.

			virtual void   add_cell(const DTree&, DTree::iterator, bool) override;
			virtual void   remove_cell(const DTree&, DTree::iterator) override;
			virtual void   remove_all_cells() override;
			virtual void   update_cell(const DTree&, DTree::iterator) override;
			virtual void   position_cursor(const DTree&, DTree::iterator, int pos) override;
			virtual size_t get_cursor_position(const DTree&, DTree::iterator) override;

			virtual void on_connect() override;
			virtual void on_disconnect(const std::string&) override;
			virtual void on_network_error() override;
			virtual void on_kernel_runstatus(bool) override;

			virtual void process_data() override;

			// TeX stuff
			TeXEngine        engine;
			double           scale; // total scale factor (hdpi and textscale)
			double           display_scale; // hdpi scale only

			// Handler for vertical scrollbar changes.
			bool on_vscroll_changed(Gtk::ScrollType, double);

			// Handler for mouse wheel events.
			// bool on_mouse_wheel(GdkEventButton*);

			// Handler for scroll events.
			bool on_scroll(GdkEventScroll*);

			// When something inside the large notebook canvas changes, we need
			// to make sure that the current cell stays into view (if we are
			// editing that cell). We can only do that once all size info is
			// known, which is when the scrolledwindow gets its size_allocate
			// signal. Here's the handler for it.
			void on_scroll_size_allocate(Gtk::Allocation&);

			// Ensure that the current cell is visible. This will assume
			// that all size allocations of widgets inside the scrolled window
			// have been made; it only does scrolling, based on the current
			// allocations.
			void scroll_current_cell_into_view();

			void set_name(const std::string&);
			void set_title_prefix(const std::string&);

			void load_file(const std::string& notebook_contents);
			void refresh_highlighting();
			void on_help_register();

			/// Functionality for the diff viewer.
			void select_git_path();
			void compare_to_file();
			void compare_git_latest();
			void compare_git_choose();
			void compare_git_specific();
			void compare_git(const std::string& commit_hash);
			std::string run_git_command(const std::string& args);

			virtual void set_compute_thread(ComputeThread* compute) override;

			virtual void on_interactive_output(const Json::Value& msg) override;

		protected:
			virtual bool on_key_press_event(GdkEventKey*) override;
			virtual bool on_delete_event(GdkEventAny*) override;
			virtual bool on_configure_event(GdkEventConfigure *cfg) override;

			DTree::iterator current_cell;

			bool handle_outbox_select(GdkEventButton *, DTree::iterator it);
			DTree::iterator selected_cell;
			void unselect_output_cell();
			void on_outbox_copy(Glib::RefPtr<Gtk::Clipboard> refClipboard, DTree::iterator it);

		private:
			Cadabra *cdbapp;

			std::vector<Glib::RefPtr<Gtk::Action>> default_actions;

			// Main handler which fires whenever the Client object signals
			// that the document is changing or the network status is modified.
			// Runs on the GUI thread.

			Glib::Dispatcher dispatcher;

			// GUI elements.

			Glib::RefPtr<Gtk::ActionGroup> actiongroup;
			Glib::RefPtr<Gtk::UIManager>   uimanager;

			Gtk::VBox                      topbox;
			Gtk::HBox                      supermainbox;
			Gtk::VBox                      mainbox;
			//			Gtk::HBox                      buttonbox;
			Gtk::HBox                      statusbarbox;

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
			Gtk::Label                     status_label, kernel_label;

			// GUI data which is the autoritative source for things displayed in
			// the status bars declared above. These strings are filled on the
			// compute thread and then updated into the gui on the gui thread.

			std::mutex                     status_mutex;
			std::string                    status_string, kernel_string;

			// Name and modification data.
			void             update_title();
			void             set_stop_sensitive(bool);
			std::string      name, title_prefix;
			bool             modified, read_only;

			// Menu and button callbacks.
			void on_file_new();
			void on_file_open();
			void on_file_close();
			void on_file_save();
			void on_file_save_as();
			void on_file_export_html();
			void on_file_export_html_segment();
			void on_file_export_latex();
			void on_file_export_python();
			void on_file_quit();
			bool quit_safeguard(bool quit);

			void on_edit_undo();
			void on_edit_copy();
			Glib::RefPtr<Gtk::Action> action_copy, action_paste;
			void on_edit_paste();
			void on_edit_insert_above();
			void on_edit_insert_below();
			void on_edit_delete();
			void on_edit_split();
			void on_edit_cell_is_latex();
			void on_edit_cell_is_python();

			void on_view_split();
			void on_view_close();

			void on_run_cell();
			void on_run_runall();
			void on_run_runtocursor();
			void on_run_stop();

			void on_prefs_set_cv(int vis);
			void on_prefs_font_size(int num);
			void on_prefs_highlight_syntax(int on);
			void on_prefs_choose_colours();
			void on_prefs_use_defaults();
			void on_help_about();
			void on_help() const;

			void on_kernel_restart();

			/// Clipboard handling
			void on_clipboard_get(Gtk::SelectionData&, guint info);
			void on_clipboard_clear();
			std::string clipboard_txt, clipboard_cdb;

			// FIXME: move to DocumentThread
			std::string save(const std::string& fn) const;

			/// Todo deque processing logic. This gets called by the dispatcher, but it
			/// is also allowed to call this from within NotebookWindow itself. The important
			/// thing is that it is run on the GUI thread.
			void process_todo_queue();

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
			bool cell_content_execute(DTree::iterator, int, bool shift_enter_pressed);
			bool cell_content_changed(const std::string& content, DTree::iterator it, int canvas_number);

			void interactive_execute();

			void dim_output_cells(DTree::iterator it);

			// Handler for callbacks from TeXView cells.

			bool on_tex_error(const std::string&, DTree::iterator);

			// Styling through CSS
			void                           load_css(const std::string&);
			Glib::RefPtr<Gtk::CssProvider> css_provider;
			Glib::RefPtr<Gio::Settings>    settings;
			void on_text_scaling_factor_changed(const std::string& key);

			int             last_configure_width;
			DTree::iterator follow_cell;


			bool  is_configured;

			Glib::RefPtr<Gtk::Action> menu_help_register;

			// Transition animations.
#if GTKMM_MINOR_VERSION>=10
			std::vector<Gtk::Revealer *> to_reveal;
#endif
			bool idle_handler();
		};

	};
