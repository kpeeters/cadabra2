
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

#include <thread>
#include <mutex>

#include "DocumentThread.hh"
#include "ComputeThread.hh"
#include "GUIBase.hh"
#include "NotebookCanvas.hh"
#include "../common/TeXEngine.hh"

/// \defgroup gtkmm Gtk+ 
/// \ingroup frontend
/// Notebook user interface implemented using gtkmm.

class Cadabra;

namespace cadabra {
	
	/// \ingroup gtkmm
	///
   /// Each notebook has one main window which controls it. It has a menu bar, a
   /// status pane and one or more panels that represent a view on the document.

   class NotebookWindow : public Gtk::Window, public DocumentThread, public GUIBase {
      public:
         NotebookWindow(Cadabra *);
         ~NotebookWindow();
        
         // Virtual functions from GUIBase.

         virtual void add_cell(const DTree&, DTree::iterator, bool) override;
         virtual void remove_cell(const DTree&, DTree::iterator) override;
         virtual void remove_all_cells() override;
         virtual void update_cell(const DTree&, DTree::iterator) override;
			virtual void position_cursor(const DTree&, DTree::iterator) override;
			virtual size_t get_cursor_position(const DTree&, DTree::iterator) override;

         virtual void on_connect() override;
         virtual void on_disconnect(const std::string&) override;
         virtual void on_network_error() override;
			virtual void on_kernel_runstatus(bool) override;

			virtual void process_data() override;

			// TeX stuff
			TeXEngine        engine;
			double           scale; // highdpi scale

			// For grabbing focus of widgets which are not yet allocated.
			void on_widget_size_allocate(Gtk::Allocation&, Gtk::Widget *w);
			sigc::connection grab_connection;

			void set_name(const std::string&);
			void load_file(const std::string& notebook_contents);


		protected:
			virtual bool on_key_press_event(GdkEventKey*) override;
			virtual bool on_delete_event(GdkEventAny*) override;
			virtual bool on_configure_event(GdkEventConfigure *cfg) override;

			DTree::iterator current_cell;

      private:
			Cadabra *cdbapp;

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
			void             scroll_into_view(DTree::iterator);
			std::string      name;
			bool             modified;

			// Menu and button callbacks.
			void on_file_new();
			void on_file_open();
			void on_file_save();
			void on_file_save_as();
			void on_file_export_html();
			void on_file_export_html_segment();
			void on_file_export_latex();
			void on_file_export_python();
			void on_file_quit();
			bool quit_safeguard(bool quit);

			void on_edit_undo();
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

			void on_help_about();
			void on_help() const;

			void on_kernel_restart();

			// FIXME: move to DocumentThread
			std::string save(const std::string& fn) const;

			// Todo deque processing logic. This gets called by the dispatcher, but it
			// is also allowed to call this from within NotebookWindow itself. The important
			// thing is that it is run on the GUI thread.
			void process_todo_queue();

			// The following are handlers that get called when the cell
			// gets focus, the content of a cell is changed, the user
			// requests to run it (shift-enter). The last two parameters are
			// always the cell in the DTree and the canvas number.

			bool cell_got_focus(DTree::iterator, int);
			bool cell_toggle_visibility(DTree::iterator it, int);
			bool cell_content_changed(const std::string&, DTree::iterator, int);
			bool cell_content_execute(DTree::iterator, int);
			
			void dim_output_cells(DTree::iterator it);

			// Handler for callbacks from TeXView cells.

			bool on_tex_error(const std::string&, DTree::iterator);

			// Styling through CSS
			void                           setup_css_provider();
			Glib::RefPtr<Gtk::CssProvider> css_provider;
			Glib::RefPtr<Gio::Settings>    settings;
			void on_text_scaling_factor_changed(const std::string& key);

			int last_configure_width;
	};

};
