
#include <iostream>
#include "Actions.hh"
#include "Cadabra.hh"
#include "Config.hh"
#include "NotebookWindow.hh"
#include "DataCell.hh"
#include <gtkmm/box.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/aboutdialog.h>
#include <gtkmm/radioaction.h>
#include <gtkmm/scrollbar.h>
#include <fstream>
#if GTKMM_MINOR_VERSION < 10
#include <gtkmm/main.h>
#endif

using namespace cadabra;

NotebookWindow::Prefs::Prefs()
	: font_step(0)
	{
	}

NotebookWindow::NotebookWindow(Cadabra *c, bool ro)
	: DocumentThread(this),
	  current_cell(doc.end()),
	  cdbapp(c),
	  current_canvas(0),
//	  b_help(Gtk::Stock::HELP), b_stop(Gtk::Stock::STOP), b_undo(Gtk::Stock::UNDO), b_redo(Gtk::Stock::REDO), 
	  kernel_spinner_status(false), title_prefix("Cadabra: "),
	  modified(false), read_only(ro), crash_window_hidden(true), follow_cell(doc.end()), is_configured(false)
	{
   // Connect the dispatcher.
	dispatcher.connect(sigc::mem_fun(*this, &NotebookWindow::process_todo_queue));

	// Set the window icon.
//#ifdef __APPLE__
	set_icon_name("cadabra2-gtk");
//#else
//	std::cerr << CMAKE_INSTALL_PREFIX"/share/cadabra2/images/cadabra2-gtk.png" << std::endl;
//	set_icon_from_file("/usr/share/icons/hicolor/scalable/apps/cadabra2-gtk.svg");
//#endif

	// Query high-dpi settings. For now only for cinnamon.
	scale = 1.0;
#ifndef __APPLE__
	const char *ds = std::getenv("DESKTOP_SESSION");
	if(ds) {
	  settings = Gio::Settings::create((strcmp(ds, "cinnamon") == 0) ? "org.cinnamon.desktop.interface" : "org.gnome.desktop.interface");
	  scale = settings->get_double("text-scaling-factor");
	}
#endif
	engine.set_scale(scale);

#ifndef __APPLE__
	if(ds) {
	  settings->signal_changed().connect(
					     sigc::mem_fun(*this, &NotebookWindow::on_text_scaling_factor_changed));
	}
#endif

	// Setup styling. Note that 'margin-left' and so on do not work; you need
	// to use 'padding'. However, 'padding-top' fails because it does not make the
   // widget larger enough... So we still do that with set_margin_top(...).
	css_provider = Gtk::CssProvider::create();
	// padding-left: 20px; does not work on some versions of gtk, so we use margin in CodeInput
	Glib::ustring data = "GtkTextView { color: blue;  }\n";
	data += "GtkTextView { background: white; -GtkWidget-cursor-aspect-ratio: 0.2; }\n";
	data += "*:focus { background-color: #eee; }\n";
	data += "*:selected { background-color: #ccc; }\n";
	data += "GtkTextView.error { background: transparent; -GtkWidget-cursor-aspect-ratio: 0.2; color: @theme_fg_color; }\n";
	data += "#ImageView { background-color: white; transition-property: padding, background-color; transition-duration: 1s; }\n";
	//	data += "scrolledwindow { kinetic-scrolling: false; }\n";

	if(!css_provider->load_from_data(data)) {
	  std::cerr << "Cannot parse internal CSS." << std::endl;
		throw std::logic_error("Failed to parse widget CSS information.");
		}
	auto screen = Gdk::Screen::get_default();
	std::cerr << "cadabra-client: scale = " << screen->get_monitor_scale_factor(0) << std::endl;
	Gtk::StyleContext::add_provider_for_screen(screen, css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	// Setup menu.
	actiongroup = Gtk::ActionGroup::create();
	actiongroup->add( Gtk::Action::create("MenuFile", "_File") );
	actiongroup->add( Gtk::Action::create("New", Gtk::Stock::NEW), Gtk::AccelKey("<control>N"),
							sigc::mem_fun(*this, &NotebookWindow::on_file_new) );
	actiongroup->add( Gtk::Action::create("Open", Gtk::Stock::OPEN), Gtk::AccelKey("<control>O"),
							sigc::mem_fun(*this, &NotebookWindow::on_file_open) );
	actiongroup->add( Gtk::Action::create("Close", Gtk::Stock::CLOSE), Gtk::AccelKey("<control>W"),
							sigc::mem_fun(*this, &NotebookWindow::on_file_close) );
	actiongroup->add( Gtk::Action::create("Save", Gtk::Stock::SAVE), Gtk::AccelKey("<control>S"),
							sigc::mem_fun(*this, &NotebookWindow::on_file_save) );
	actiongroup->add( Gtk::Action::create("SaveAs", Gtk::Stock::SAVE_AS),
							sigc::mem_fun(*this, &NotebookWindow::on_file_save_as) );
	actiongroup->add( Gtk::Action::create("ExportHtml", "Export to standalone HTML"),
							sigc::mem_fun(*this, &NotebookWindow::on_file_export_html) );
	actiongroup->add( Gtk::Action::create("ExportHtmlSegment", "Export to HTML segment"),
							sigc::mem_fun(*this, &NotebookWindow::on_file_export_html_segment) );
	actiongroup->add( Gtk::Action::create("ExportLaTeX", "Export to standalone LaTeX"),
							sigc::mem_fun(*this, &NotebookWindow::on_file_export_latex) );
	actiongroup->add( Gtk::Action::create("ExportPython", "Export to Python/Cadabra source"),
							sigc::mem_fun(*this, &NotebookWindow::on_file_export_python) );
	actiongroup->add( Gtk::Action::create("Quit", Gtk::Stock::QUIT),
							sigc::mem_fun(*this, &NotebookWindow::on_file_quit) );

	actiongroup->add( Gtk::Action::create("MenuEdit", "_Edit") );
	actiongroup->add( Gtk::Action::create("EditUndo", Gtk::Stock::UNDO), Gtk::AccelKey("<control>Z"),
							sigc::mem_fun(*this, &NotebookWindow::on_edit_undo) );
	actiongroup->add( Gtk::Action::create("EditInsertAbove", "Insert cell above"), Gtk::AccelKey("<alt>Up"),
							sigc::mem_fun(*this, &NotebookWindow::on_edit_insert_above) );
	actiongroup->add( Gtk::Action::create("EditInsertBelow", "Insert cell below"), Gtk::AccelKey("<alt>Down"),
							sigc::mem_fun(*this, &NotebookWindow::on_edit_insert_below) );
	actiongroup->add( Gtk::Action::create("EditDelete", "Delete cell"), Gtk::AccelKey("<ctrl>Delete"),
							sigc::mem_fun(*this, &NotebookWindow::on_edit_delete) );
	actiongroup->add( Gtk::Action::create("EditSplit", "Split cell"), 
							sigc::mem_fun(*this, &NotebookWindow::on_edit_split) );
	actiongroup->add( Gtk::Action::create("EditMakeCellTeX", "Cell is LaTeX"), Gtk::AccelKey("<control><shift>L"),
							sigc::mem_fun(*this, &NotebookWindow::on_edit_cell_is_latex) );
	actiongroup->add( Gtk::Action::create("EditMakeCellPython", "Cell is Python"), Gtk::AccelKey("<control><shift>P"),
							sigc::mem_fun(*this, &NotebookWindow::on_edit_cell_is_python) );

	actiongroup->add( Gtk::Action::create("MenuView", "_View") );
	actiongroup->add( Gtk::Action::create("ViewSplit", "Split view"),
							sigc::mem_fun(*this, &NotebookWindow::on_view_split) );
	actiongroup->add( Gtk::Action::create("ViewClose", "Close view"),
							sigc::mem_fun(*this, &NotebookWindow::on_view_close) );

	Gtk::RadioAction::Group group_font_size;

	actiongroup->add( Gtk::Action::create("MenuFontSize", "Font size") );
	auto font_action0=Gtk::RadioAction::create(group_font_size, "FontSmall", "Small");
	font_action0->property_value()=-1;
	actiongroup->add( font_action0, sigc::bind(sigc::mem_fun(*this, &NotebookWindow::on_prefs_font_size),-1 ));
	if(prefs.font_step==-1) font_action0->set_active();

	auto font_action1=Gtk::RadioAction::create(group_font_size, "FontMedium", "Medium (default)");
	font_action1->property_value()= 0;
	actiongroup->add( font_action1, sigc::bind(sigc::mem_fun(*this, &NotebookWindow::on_prefs_font_size), 0));
	if(prefs.font_step==0) font_action1->set_active();

	auto font_action2=Gtk::RadioAction::create(group_font_size, "FontLarge", "Large");
	font_action2->property_value()= 2;
	actiongroup->add( font_action2, sigc::bind(sigc::mem_fun(*this, &NotebookWindow::on_prefs_font_size), 2));
	if(prefs.font_step==2) font_action2->set_active();

	auto font_action3=Gtk::RadioAction::create(group_font_size, "FontExtraLarge", "Extra large");
	font_action3->property_value()= 4;
	actiongroup->add( font_action3, sigc::bind(sigc::mem_fun(*this, &NotebookWindow::on_prefs_font_size), 4));
	if(prefs.font_step==4) font_action3->set_active();

	actiongroup->add( Gtk::Action::create("MenuEvaluate", "_Evaluate") );
 	actiongroup->add( Gtk::Action::create("EvaluateCell", "Evaluate cell"), Gtk::AccelKey("<shift>Return"),
							sigc::mem_fun(*this, &NotebookWindow::on_run_cell) );
 	actiongroup->add( Gtk::Action::create("EvaluateAll", Gtk::Stock::GO_FORWARD, "Evaluate all"),
							sigc::mem_fun(*this, &NotebookWindow::on_run_runall) );
	actiongroup->add( Gtk::Action::create("EvaluateToCursor", Gtk::Stock::GOTO_LAST, "Evaluate to cursor"),
							sigc::mem_fun(*this, &NotebookWindow::on_run_runtocursor) );
	actiongroup->add( Gtk::Action::create("EvaluateStop", Gtk::Stock::STOP, "Stop"), Gtk::AccelKey('.', Gdk::MOD1_MASK),
							sigc::mem_fun(*this, &NotebookWindow::on_run_stop) );
	actiongroup->add( Gtk::Action::create("MenuKernel", "_Kernel") );
	actiongroup->add( Gtk::Action::create("KernelRestart", Gtk::Stock::REFRESH, "Restart"),
							sigc::mem_fun(*this, &NotebookWindow::on_kernel_restart) );

	actiongroup->add( Gtk::Action::create("MenuHelp", "_Help") );
//	actiongroup->add( Gtk::Action::create("HelpNotebook", Gtk::Stock::HELP, "How to use the notebook"),
//							sigc::mem_fun(*this, &NotebookWindow::on_help_notebook) );
	actiongroup->add( Gtk::Action::create("HelpAbout", Gtk::Stock::ABOUT, "About Cadabra"),
							sigc::mem_fun(*this, &NotebookWindow::on_help_about) );
	actiongroup->add( Gtk::Action::create("HelpContext", Gtk::Stock::HELP, "Contextual help"),
							sigc::mem_fun(*this, &NotebookWindow::on_help) );

	uimanager = Gtk::UIManager::create();
	uimanager->insert_action_group(actiongroup);
	add_accel_group(uimanager->get_accel_group());
	Glib::ustring ui_info =
		"<ui>"
		"  <menubar name='MenuBar'>"
		"    <menu action='MenuFile'>"
		"      <menuitem action='New'/>"
		"      <menuitem action='Open'/>"
		"      <menuitem action='Close'/>"
		"      <separator/>"
		"      <menuitem action='Save'/>"
		"      <menuitem action='SaveAs'/>"
		"      <menuitem action='ExportHtml'/>"
		"      <menuitem action='ExportHtmlSegment'/>"
		"      <menuitem action='ExportLaTeX'/>"
		"      <menuitem action='ExportPython'/>"
		"      <separator/>"
		"      <menuitem action='Quit'/>"
		"    </menu>";
	if(!read_only) 
		ui_info+=
		"    <menu action='MenuEdit'>"
		"      <menuitem action='EditUndo' />"
		"      <separator/>"
		"      <menuitem action='EditInsertAbove' />"
		"      <menuitem action='EditInsertBelow' />"
		"      <menuitem action='EditDelete' />"
		"      <separator/>"
		"      <menuitem action='EditSplit' />"
		"      <separator/>"
		"      <menuitem action='EditMakeCellTeX' />"
		"      <menuitem action='EditMakeCellPython' />"
		"    </menu>"
		"    <menu action='MenuView'>"
		"      <menuitem action='ViewSplit' />"
		"      <menuitem action='ViewClose' />"
		"      <menu action='MenuFontSize'>"
		"         <menuitem action='FontSmall'/>"
		"         <menuitem action='FontMedium'/>"
		"         <menuitem action='FontLarge'/>"
		"         <menuitem action='FontExtraLarge'/>"
      "      </menu>"
		"    </menu>"
		"    <menu action='MenuEvaluate'>"
		"      <menuitem action='EvaluateCell' />"
		"      <menuitem action='EvaluateAll' />"
		"      <menuitem action='EvaluateToCursor' />"
		"      <separator/>"
		"      <menuitem action='EvaluateStop' />"
		"    </menu>"
		"    <menu action='MenuKernel'>"
		"      <menuitem action='KernelRestart' />"
			"    </menu>";
	ui_info+=
		"    <menu action='MenuHelp'>"
//		"      <menuitem action='HelpNotebook' />"
		"      <menuitem action='HelpAbout' />"
		"      <menuitem action='HelpContext' />"
		"    </menu>"
		"  </menubar>"
		"  <toolbar name='ToolBar'>"
		"    <toolitem action='Open' />"
//		"       <property name='tooltip_text' translatable='yes'>Open existing notebook</property>"
//		"    </toolitem>"
		"    <toolitem action='EvaluateAll' name='run all'/>"
		"    <toolitem action='EvaluateStop' />"
		"  </toolbar>"
		"</ui>";

	uimanager->add_ui_from_string(ui_info);

	// Main box structure dividing the window.
	add(topbox);
	Gtk::Widget *menubar = uimanager->get_widget("/MenuBar");
	topbox.pack_start(*menubar, Gtk::PACK_SHRINK);
	Gtk::Widget *toolbar = uimanager->get_widget("/ToolBar");
	topbox.pack_start(*toolbar, Gtk::PACK_SHRINK);
	topbox.pack_start(supermainbox, true, true);
	topbox.pack_start(statusbarbox, false, false);
	supermainbox.pack_start(mainbox, true, true);


	// Status bar
	status_label.set_alignment( 0.0, 0.5 );
	kernel_label.set_alignment( 0.0, 0.5 );
	status_label.set_size_request(200,-1);
	status_label.set_justify(Gtk::JUSTIFY_LEFT);
	kernel_label.set_justify(Gtk::JUSTIFY_LEFT);
	statusbarbox.pack_start(status_label);
	statusbarbox.pack_start(kernel_label);
	statusbarbox.pack_start(kernel_spinner);
	statusbarbox.pack_start(progressbar);
	progressbar.set_size_request(200,-1);
	progressbar.set_text("idle");
	progressbar.set_show_text(true);


	// The three main widgets
//	mainbox.pack_start(buttonbox, Gtk::PACK_SHRINK, 0);

	// We always have at least one canvas.
	canvasses.push_back(manage( new NotebookCanvas() ));
	mainbox.pack_start(*canvasses[0], Gtk::PACK_EXPAND_WIDGET, 0);

	// FIXME: need to do this for every canvas.
	canvasses[0]->scroll.signal_size_allocate().connect(
		sigc::mem_fun(*this, &NotebookWindow::on_scroll_size_allocate));
//	canvasses[0]->scroll.get_vadjustment()->signal_value_changed().connect(
//		sigc::mem_fun(*this, &NotebookWindow::on_vscroll_changed));
	canvasses[0]->scroll.get_vscrollbar()->signal_value_changed().connect(
		sigc::mem_fun(*this, &NotebookWindow::on_vscroll_changed));


	// Window size and title, and ready to go.
	set_default_size(screen->get_width()/2, screen->get_height()*0.8);
	// FIXME: the subtraction for the margin and scrollbar made below
	// is estimated but should be computed.
//	engine.set_geometry(screen->get_width()/2 - 2*30);
	update_title();
	show_all();
	kernel_spinner.hide();
	if(read_only) {
		statusbarbox.hide();
		progressbar.hide();
		toolbar->hide();
		} 
	else {
		// Buttons
		set_stop_sensitive(false);
		}

	new_document();
	}

NotebookWindow::~NotebookWindow()
	{
	}

bool NotebookWindow::on_delete_event(GdkEventAny* event)
	{
	if(quit_safeguard(true)) {
		return Gtk::Window::on_delete_event(event);
		}
	else
		return false;
	}

bool NotebookWindow::on_configure_event(GdkEventConfigure *cfg)
	{
//	std::cerr << "cadabra-client: on_configure_event " << cfg->width << " x " << cfg->height << std::endl;
	is_configured=true;
	if(cfg->width != last_configure_width) 
		engine.set_geometry(cfg->width-2*30);

	bool ret=Gtk::Window::on_configure_event(cfg);
	
	if(cfg->width != last_configure_width) {
		last_configure_width = cfg->width;
		try {
			engine.invalidate_all();
			engine.convert_all();
			for(unsigned int i=0; i<canvasses.size(); ++i) 
				canvasses[i]->refresh_all();
			}
		catch(TeXEngine::TeXException& ex) {
			on_tex_error(ex.what(), doc.end());
			}
		}

	return ret;
	}

void NotebookWindow::set_title_prefix(const std::string& pf)
	{
	title_prefix=pf;
	}

void NotebookWindow::update_title()
	{
	if(name.size()>0) {
		if(modified)
			set_title(title_prefix+name+"*");
		else
			set_title(title_prefix+name);
		}
	else {
		if(modified) 
			set_title("Cadabra*");
		else
			set_title("Cadabra");
		}
	}

void NotebookWindow::set_stop_sensitive(bool s)
	{
	Gtk::Widget *stop = uimanager->get_widget("/ToolBar/EvaluateStop");
	stop->set_sensitive(s);
	stop = uimanager->get_widget("/MenuBar/MenuEvaluate/EvaluateStop");
	stop->set_sensitive(s);	
	}

void NotebookWindow::process_data() 
	{
	dispatcher.emit();
	}


void NotebookWindow::on_connect()
	{
	std::lock_guard<std::mutex> guard(status_mutex);
	kernel_string = "connected";
	dispatcher.emit();
	}

void NotebookWindow::on_disconnect(const std::string& reason)
	{
	std::lock_guard<std::mutex> guard(status_mutex);
	kernel_string = reason;
	dispatcher.emit();
	}

void NotebookWindow::on_network_error()
	{
	std::lock_guard<std::mutex> guard(status_mutex);
	kernel_string = "cannot reach server, retrying...";
	dispatcher.emit();
	}

void NotebookWindow::on_kernel_runstatus(bool running)
	{
	std::lock_guard<std::mutex> guard(status_mutex);
	kernel_spinner_status=running;
	dispatcher.emit();
	}

void NotebookWindow::process_todo_queue()
	{
	static bool running=false;

	// Prevent from re-entering this from the process_action_queue entered below.
	if(running) return;
	running=true;

	// Update the status/kernel messages into the corresponding widgets.
	{
	std::lock_guard<std::mutex> guard(status_mutex);
	kernel_label.set_text(kernel_string);
	status_label.set_text(status_string);

	if(kernel_spinner_status) { 
		kernel_spinner.show();
		kernel_spinner.start();
		}
	else {
		kernel_spinner.stop();
		kernel_spinner.hide();
		}
		}

	// Perform any ActionBase actions.
	process_action_queue();

	// Before we pop up any dialogs, enable queue processing again, otherwise
	// subsequent calls to process_todo_queue will get postponed until a
	// dispatcher.emit() is called when the dialog is closed.
	running=false;

	if(crash_window_hidden && kernel_string=="not connected") {
		crash_window_hidden=false;
		Gtk::MessageDialog md("Kernel crashed", false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
		md.set_transient_for(*this);
		md.set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
		md.set_secondary_text("The kernel crashed unexpectedly, and has been restarted. You will need to re-run all cells.");
		md.signal_response().connect(sigc::mem_fun(*this, &NotebookWindow::on_crash_window_closed));
		md.run();
		}
	}

void NotebookWindow::on_crash_window_closed(int)
	{
	crash_window_hidden=true;
	}

bool NotebookWindow::on_key_press_event(GdkEventKey* event)
	{
	bool is_ctrl_up    = event->keyval==GDK_KEY_Up   && (event->state&Gdk::CONTROL_MASK);	
	bool is_ctrl_down  = event->keyval==GDK_KEY_Down && (event->state&Gdk::CONTROL_MASK);	

	if(is_ctrl_up) {
 		std::shared_ptr<ActionBase> actionpos =
			std::make_shared<ActionPositionCursor>(current_cell, ActionPositionCursor::Position::previous);
		queue_action(actionpos);
		process_todo_queue();
		return true;
		} 
	else if(is_ctrl_down) {
 		std::shared_ptr<ActionBase> actionpos =
			std::make_shared<ActionPositionCursor>(current_cell, ActionPositionCursor::Position::next);
		queue_action(actionpos);
		process_todo_queue();
		return true;
		}
	else {
		return Gtk::Window::on_key_press_event(event);
		}
	}

void NotebookWindow::add_cell(const DTree& tr, DTree::iterator it, bool visible)
	{
	// Add a visual cell corresponding to this document cell in 
	// every canvas.

	if(compute!=0)
		set_stop_sensitive( compute->number_of_cells_executing()>0 );
	
	Glib::RefPtr<Gtk::TextBuffer>          global_buffer;
	std::shared_ptr<TeXEngine::TeXRequest> global_texrequest;
	
	for(unsigned int i=0; i<canvasses.size(); ++i) {

		// If this data cell already has a representation in the current canvas 
		// we can continue to the next canvas. However, we need to set the global
		// buffer from existing cells.

		if(canvasses[i]->visualcells.find(&(*it))!=canvasses[i]->visualcells.end()) {
			if(i==0 && it->cell_type==DataCell::CellType::python) {
				global_buffer = canvasses[i]->visualcells[&(*it)].inbox->buffer;
				}
			continue;
			}

		// Create a visual cell of the appropriate type.

		VisualCell newcell;
		Gtk::Widget *w=0;
		switch(it->cell_type) {
			case DataCell::CellType::document:
				newcell.document = manage( new Gtk::VBox() );
				w=newcell.document;
				break;

			case DataCell::CellType::output:
			case DataCell::CellType::error:
			case DataCell::CellType::verbatim: {
				// FIXME: would be good to share the input and output of TeXView too.
				// Right now nothing is shared...
				//if(it->cell_type==DataCell::CellType::error) 
				//   std::cerr << "error cell" << std::endl;
				newcell.outbox = manage( new TeXView(engine, it) );
#if GTKMM_MINOR_VERSION>=10
				newcell.outbox->rbox.set_reveal_child(true);
#endif				
				w=newcell.outbox;
				break;
				}
			case DataCell::CellType::latex_view:
				// FIXME: would be good to share the input and output of TeXView too.
				// Right now nothing is shared...
				newcell.outbox = manage( new TeXView(engine, it) );
				newcell.outbox->tex_error.connect( 
					sigc::bind( sigc::mem_fun(this, &NotebookWindow::on_tex_error), it ) );

				newcell.outbox->show_hide_requested.connect( 
					sigc::bind( sigc::mem_fun(this, &NotebookWindow::cell_toggle_visibility), i ) );

#if GTKMM_MINOR_VERSION>=10
				to_reveal.push_back(&newcell.outbox->rbox);
#endif				
				
				w=newcell.outbox;
				break;

			case DataCell::CellType::python:
			case DataCell::CellType::latex: {
				CodeInput *ci;
				// Ensure that all CodeInput cells share the same text buffer.
				if(i==0) {
					ci = new CodeInput(it, it->textbuf,scale,prefs.font_step);
					global_buffer=ci->buffer;
					}
				else ci = new CodeInput(it, global_buffer,scale,prefs.font_step);
				if(read_only)
					ci->edit.set_editable(false);
				ci->get_style_context()->add_provider(css_provider, GTK_STYLE_PROVIDER_PRIORITY_USER);

				ci->edit.content_changed.connect( 
					sigc::bind( sigc::mem_fun(this, &NotebookWindow::cell_content_changed), i ) );
				ci->edit.content_insert.connect( 
					sigc::bind( sigc::mem_fun(this, &NotebookWindow::cell_content_insert), i ) );
				ci->edit.content_erase.connect( 
					sigc::bind( sigc::mem_fun(this, &NotebookWindow::cell_content_erase), i ) );

				ci->edit.content_execute.connect( 
					sigc::bind( sigc::mem_fun(this, &NotebookWindow::cell_content_execute), i, true ) );
				ci->edit.cell_got_focus.connect( 
					sigc::bind( sigc::mem_fun(this, &NotebookWindow::cell_got_focus), i ) );

				newcell.inbox = manage( ci );
				w=newcell.inbox;

				break;
				}
			case DataCell::CellType::image_png: {
				// FIXME: horribly memory inefficient
				ImageView *iv=new ImageView();
		
				iv->set_image_from_base64(it->textbuf);
				newcell.imagebox = manage( iv );
				w=newcell.imagebox;
				break;
				}

			default:
				throw std::logic_error("Unimplemented datacell type");
			}
		
		
		canvasses[i]->visualcells[&(*it)]=newcell;
		
		// Document cells are easy; just add. They have no parent in the DTree.

		if(it->cell_type==DataCell::CellType::document) {
			canvasses[i]->scroll.add(*w);
			w->show_all(); // FIXME: if you drop this, the whole document remains invisible
			continue;
			}

		// Figure out where to store this new VisualCell in the GUI widget
		// tree by exploring the DTree near the new DataCell. 
		// First determine the parent cell and the corresponding Gtk::Box
		// so that we can determine where to pack_start this cell. At this
		// stage, all cells have parents.

		DTree::iterator parent = DTree::parent(it);
		assert(tr.is_valid(parent));

		VisualCell& parent_visual = canvasses[i]->visualcells[&(*parent)];
		Gtk::VBox *parentbox=0;
		int offset=0;
		if(parent->cell_type==DataCell::CellType::document)
			parentbox=parent_visual.document;
		else {
			// FIXME: Since we are adding children of input cells to the vbox in which
			// the exp_input_tv widget is the 0th cell, we have to offset. Would be
			// cleaner to have a separate 'children' vbox in CodeInput (or in fact
			// every widget that can potentially contain children).
			offset=1;
			parentbox=parent_visual.inbox;
			}

//		std::cout << "adding cell to canvas " << i << std::endl;
		parentbox->pack_start(*w, false, false);	
		unsigned int index    =tr.index(it)+offset;
		unsigned int index_gui=parentbox->get_children().size()-1;
//		std::cout << "is index " << index << " vs " << index_gui << std::endl;
		if(index!=index_gui) {
//			std::cout << "need to re-order" << std::endl;
			parentbox->reorder_child(*w, index);
			}
		if(visible) {
			w->show_all();
			w->show_now();
			}
		
		}

	// Connect
	Glib::signal_idle().connect(sigc::mem_fun(*this, &NotebookWindow::idle_handler));
	
//	if(current_cell!=doc.end()) 
//		setup_focus_after_allocate(it);
	}

void NotebookWindow::remove_cell(const DTree& doc, DTree::iterator it)
	{
	// Remember: this member should only remove the visual cell; the
	// document tree will be updated by the ActionRemove that led to this
	// member being called. However, we should ensure that any references
	// to the visual cell are removed as well; in particular, if 
	// current_cell is pointing to this cell, we need to unset it.

	// Can only remove cells which have a parent (i.e. not the
	// top-level document cell).

	if(current_cell==it)
		current_cell=doc.end();
	
	DTree::iterator parent = DTree::parent(it);
	assert(doc.is_valid(parent));

	for(unsigned int i=0; i<canvasses.size(); ++i) {
		VisualCell& parent_visual = canvasses[i]->visualcells[&(*parent)];
		Gtk::VBox *parentbox=0;
		if(it->cell_type==DataCell::CellType::document)
			parentbox=parent_visual.document;
		else
			parentbox=parent_visual.inbox;
		VisualCell& actual = canvasses[i]->visualcells[&(*it)];

//		if(it->cell_type==DataCell::CellType::latex_view) {
//			actual.outbox->set_reveal_child(false);
//			}
//		else {
		// The pointers are all in a union, and Gtkmm does not care
		// about the precise type, so we just remove imagebox, knowing
		// that it may actually be an inbox or outbox.
		parentbox->remove(*actual.imagebox);
		// The above does not delete the Gtk widget, despite having been
		// wrapped in manage at construction. So we have to delete it 
		// ourselves. Fortunately the container does not try to delete
		// it again in its destructor.
		delete actual.imagebox;
//			}
		canvasses[i]->visualcells.erase(&(*it));
		}	
	}

void NotebookWindow::remove_all_cells()
	{
	// Simply removing the document cell should do the trick.
	for(unsigned int i=0; i<canvasses.size(); ++i) {
		canvasses[i]->scroll.remove();
		canvasses[i]->visualcells.clear();
		}
	engine.checkout_all();
	}

void NotebookWindow::update_cell(const DTree& tr, DTree::iterator it)
	{
	// We just do a redraw for now, but this may require more work later.

	for(unsigned int i=0; i<canvasses.size(); ++i) {
		VisualCell& vc = canvasses[i]->visualcells[&(*it)];
		if(it->cell_type==DataCell::CellType::python || it->cell_type==DataCell::CellType::latex) {
			vc.inbox->update_buffer();
			vc.inbox->queue_draw();
			}
		}
	
	}

void NotebookWindow::position_cursor(const DTree& doc, DTree::iterator it, int pos)
	{
//	if(it==doc.end()) return;
	// std::cerr << "cadabra-client: positioning cursor at cell " << it->textbuf << std::endl;
	set_stop_sensitive( compute->number_of_cells_executing()>0 );

	if(canvasses[current_canvas]->visualcells.find(&(*it))==canvasses[current_canvas]->visualcells.end()) {
		std::cerr << "cadabra-client: Cannot find cell to position cursor." << std::endl;
		return;
		}

	VisualCell& target = canvasses[current_canvas]->visualcells[&(*it)];

	Gtk::Allocation alloc=target.inbox->get_allocation();
	target.inbox->edit.grab_focus();
	
	if(pos>=0) {
		auto cursor=target.inbox->edit.get_buffer()->begin();
		cursor.forward_chars(pos);
		target.inbox->edit.get_buffer()->place_cursor(cursor);
		}
	
	current_cell=it;
	}

size_t NotebookWindow::get_cursor_position(const DTree& doc, DTree::iterator it)
	{
	if(canvasses[current_canvas]->visualcells.find(&(*it))==canvasses[current_canvas]->visualcells.end()) {
		std::cerr << "cadabra-client: Cannot find cell to retrieve cursor position for." << std::endl;
		return -1;
		}

	VisualCell& target = canvasses[current_canvas]->visualcells[&(*it)];
	size_t offset = target.inbox->buffer->get_insert()->get_iter().get_offset();
	
	return offset;
	}

void NotebookWindow::scroll_current_cell_into_view()
	{
	if(current_cell==doc.end()) return;
	if(current_canvas>=(int)canvasses.size()) return;

	if(canvasses[current_canvas]->visualcells.find(&(*current_cell))==canvasses[current_canvas]->visualcells.end()) return;
	
	VisualCell& focusbox = canvasses[current_canvas]->visualcells[&(*current_cell)];

	if(focusbox.inbox==0) return;
	
	Gtk::Allocation               al=focusbox.inbox->edit.get_allocation();
	Glib::RefPtr<Gtk::Adjustment> va=canvasses[current_canvas]->scroll.get_vadjustment();

	double upper_visible=va->get_value();
	double lower_visible=va->get_value()+va->get_page_size();

	// When we get called, the busybox has its allocation done and size set. However,
	// the edit box below still has its old position (but its correct height). So we
	// should make sure that busybox.y+busybox.height+editbox.height is at the bottom
	// of the scrollbox.
	//std::cerr << "-----" << std::endl;
	//std::cerr << "viewport = " << upper_visible << " - " << lower_visible << std::endl;
	//std::cerr << "current_cell = " << al.get_y() << " height " << al.get_height() << std::endl;

	double should_be_visible = al.get_y()+al.get_height()+10;
	double shift = should_be_visible - lower_visible;
	//std::cerr << "position " << should_be_visible << " should be visible" << std::endl;
	//std::cerr << "shift = " << shift << std::endl;
	if(shift>0) {
		va->set_value( upper_visible + shift);
		}
	}

void NotebookWindow::on_vscroll_changed()
	{
	std::cerr << "vscroll changed " << std::endl;
	}

void NotebookWindow::on_scroll_size_allocate(Gtk::Allocation& scroll_alloc)
	{
	// The auto-scroll logic is as follows. Whenever a cell is ran (by
	// user pressing shift-enter only, not by full run), we set to
	// auto-scroll as soon as output for that cell appears. If multiple
	// cells are sent to the queue, we follow output for the last one
	// sent. Any scrollbar event stops auto-scrolling. Perhaps only
	// auto-restart when scrolling to the bottom of the notebook,
	// though that is not of much extra use. Could have a 'follow
	// current' mode when running an entire notebook, which is again
	// stopped by scrollbar event.
	if(follow_cell!=doc.end())
		scroll_current_cell_into_view();
	}

bool NotebookWindow::cell_toggle_visibility(DTree::iterator it, int canvas_number)
	{
	// Find the parent node. If that one is a latex cell, toggle visibility of
	// the CodeInput widget (but not anything else in its vbox).

	auto parent=DTree::parent(it);
	if(parent->cell_type==DataCell::CellType::latex) {
		// FIXME: we are not allowed to do this directly, all should go through
		// actions.
		parent->hidden = !parent->hidden;
		for(unsigned int i=0; i<canvasses.size(); ++i) {
			auto vis = canvasses[i]->visualcells.find(&(*parent));
			if(vis==canvasses[i]->visualcells.end()) {
				throw std::logic_error("Cannot find visual cell.");
				}
			else {
				if(parent->hidden) {
					(*vis).second.inbox->edit.hide();
					}
				else
					(*vis).second.inbox->edit.show();
				}
			}
		}

	return false;
	}

 bool NotebookWindow::cell_content_changed(const std::string& content, DTree::iterator it, int canvas_number)
 	{
	modified=true;
	update_title();

 	// FIXME: need to keep track of individual characters inserted, otherwise we
 	// cannot build an undo stack. The it->textbuf=content needs to be replaced
 	// with an ActionAddText. CodeInput::handle_changed 
 
// 	current_canvas=canvas_number;
// 	if(it->textbuf!=content) {
// 		it->textbuf=content;
// 		dim_output_cells(it);
// 		modified=true;
// 		update_title();
// 		}
 
 	return false;
 	}

bool NotebookWindow::cell_content_insert(const std::string& content, int pos, DTree::iterator it, int canvas_number)
	{
	if(disable_stacks) return false;

	//std::cerr << "cell_content_insert" << std::endl;
	std::shared_ptr<ActionBase> action = std::make_shared<ActionInsertText>(it, pos, content);	
	queue_action(action);
	process_todo_queue();

	return false;
	}

bool NotebookWindow::cell_content_erase(int start, int end, DTree::iterator it, int canvas_number)
	{
	if(disable_stacks) return false;

	//std::cerr << "cell_content_erase" << std::endl;
	std::shared_ptr<ActionBase> action = std::make_shared<ActionEraseText>(it, start, end);
	queue_action(action);
	process_todo_queue();

	return false;
	}

void NotebookWindow::dim_output_cells(DTree::iterator it)
	{
	// Dim the corresponding output cell, if any.
	auto ch=doc.begin(it);
	while(ch!=doc.end(it)) {
		if(ch->cell_type==DataCell::CellType::latex_view) {
			for(unsigned int i=0; i<canvasses.size(); ++i) {
				auto vc = canvasses[i]->visualcells.find(&(*ch));
				if(vc!=canvasses[i]->visualcells.end()) 
					vc->second.outbox->dim(true);
				}
			}
		++ch;
		}

	}

bool NotebookWindow::cell_got_focus(DTree::iterator it, int canvas_number)
	{
	current_cell=it;
	current_canvas=canvas_number;

	return false;
	}

bool NotebookWindow::cell_content_execute(DTree::iterator it, int canvas_number, bool shift_enter_pressed)
	{
	// This callback runs on the GUI thread. The cell pointed to by 'it' is
	// guaranteed to be valid.

	// First ensure that this cell is not already running, otherwise all hell
	// will break loose when we try to double-remove the existing output cell etc.

	if(it->running) {
		return true;
		}

	// Ensure this cell is not empty either.

	if(it->textbuf.size()==0) return true;

	current_canvas=canvas_number;

	// Remove child nodes, if any.  
	// FIXME: Does it make more sense to do this only after the
	// execution result comes back from the server?

	DTree::sibling_iterator sib=doc.begin(it);
	dim_output_cells(it);
	while(sib!=doc.end(it)) {
		// std::cout << "cadabra-client: scheduling output cell for removal" << std::endl;
		std::shared_ptr<ActionBase> action = std::make_shared<ActionRemoveCell>(sib);
		queue_action(action);
		++sib;
		}

	// Execute the cell.
	set_stop_sensitive(true);
	follow_cell=it;
	compute->execute_cell(it);

	return true;
	}

bool NotebookWindow::on_tex_error(const std::string& str, DTree::iterator it)
	{
//	Gtk::Dialog md;
	Gtk::MessageDialog md("Generic TeX error", false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
	md.set_resizable(true);
//	Gtk::Button ok(Gtk::Stock::OK);
	md.set_transient_for(*this);
	md.set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
	auto box = md.get_message_area();
//	md.add_button(Gtk::Stock::OK, 1);
	Gtk::ScrolledWindow sw;
	Gtk::TextView tv;
	auto buffer = tv.get_buffer();
	buffer->set_text(str);
//	auto iter = buffer->get_iter_at_offset(0);
//	buffer->insert(iter, str);
	tv.set_editable(false);
	box->add(sw);
	sw.add(tv);
	auto context = tv.get_style_context();
	context->add_class("error");
	auto screen = Gdk::Screen::get_default();
	sw.set_size_request(screen->get_width()/4, screen->get_width()/4);
	sw.show_all();
	md.run();
	return true;
	}

void NotebookWindow::on_file_new()
	{
	if(quit_safeguard(false)) {	
		doc.clear();
		remove_all_cells();
		new_document();
		compute->restart_kernel();
		position_cursor(doc, doc.begin(doc.begin()), -1);
		name="";
		update_title();
		}
	}

void NotebookWindow::on_file_close()
	{
	if(quit_safeguard(true))
		hide();
	}

void NotebookWindow::on_file_open()
	{
	if(quit_safeguard(false)==false)
		return;
	
	Gtk::FileChooserDialog dialog("Please choose a Cadabra notebook (.cnb file) to open",
											Gtk::FILE_CHOOSER_ACTION_OPEN);

	dialog.set_transient_for(*this);
	dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	dialog.add_button("Select", Gtk::RESPONSE_OK);

	int result=dialog.run();

	switch(result) {
		case(Gtk::RESPONSE_OK): {
			name = dialog.get_filename();			
			std::ifstream file(name);
			std::string content, line;
			
			while(std::getline(file, line)) 
				content+=line;

			compute->restart_kernel();
			load_file(content);
			break;
			}
		}
	}

void NotebookWindow::set_name(const std::string& n)
	{
	name=n;
	update_title();
	}

void NotebookWindow::load_file(const std::string& notebook_contents)
	{
	load_from_string(notebook_contents);

	mainbox.show_all();
	modified=false;
	update_title();
	}

void NotebookWindow::on_file_save()
	{
	// check if name known, otherwise call save_as
	if(name.size()>0) {
		std::string res=save(name);
		if(res.size()>0) {
			Gtk::MessageDialog md("Error saving notebook "+name);
			md.set_transient_for(*this);
			md.set_secondary_text(res);
			md.set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
			md.run();
			}
		else {
			modified=false;
			update_title();
			}
		}
	else on_file_save_as();
	}

void NotebookWindow::on_file_save_as()
	{
	Gtk::FileChooserDialog dialog("Please choose a file name to save this notebook",
											Gtk::FILE_CHOOSER_ACTION_SAVE);

	dialog.set_transient_for(*this);
	dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	dialog.add_button("Select", Gtk::RESPONSE_OK);

	int result=dialog.run();

	switch(result) {
		case(Gtk::RESPONSE_OK): {
			name = dialog.get_filename();			
			std::string res=save(name);
			if(res.size()>0) {
				Gtk::MessageDialog md("Error saving notebook "+name);
				md.set_transient_for(*this);
				md.set_secondary_text(res);
				md.set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
				md.run();
				}
			else {
				modified=false;
				update_title();
				}
			break;
			}
		}
	}

void NotebookWindow::on_file_export_html()
	{
	Gtk::FileChooserDialog dialog("Please enter a file name for the HTML document",
											Gtk::FILE_CHOOSER_ACTION_SAVE);

	dialog.set_transient_for(*this);
	dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	dialog.add_button("Select", Gtk::RESPONSE_OK);

	int result=dialog.run();

	switch(result) {
		case(Gtk::RESPONSE_OK): {
			std::string name = dialog.get_filename();			
			std::ofstream temp(name);
			temp << export_as_HTML(doc);
			}
		}
	}

void NotebookWindow::on_file_export_latex()
	{
	Gtk::FileChooserDialog dialog("Please enter a file name for the LaTeX document",
											Gtk::FILE_CHOOSER_ACTION_SAVE);

	dialog.set_transient_for(*this);
	dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	dialog.add_button("Select", Gtk::RESPONSE_OK);

	int result=dialog.run();

	switch(result) {
		case(Gtk::RESPONSE_OK): {
			std::string name = dialog.get_filename();			
			std::ofstream temp(name);
			temp << export_as_LaTeX(doc);
			}
		}
	}

void NotebookWindow::on_file_export_python()
	{
	Gtk::FileChooserDialog dialog("Please enter a file name for the Python/Cadabra document",
											Gtk::FILE_CHOOSER_ACTION_SAVE);

	dialog.set_transient_for(*this);
	dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	dialog.add_button("Select", Gtk::RESPONSE_OK);

	int result=dialog.run();

	switch(result) {
		case(Gtk::RESPONSE_OK): {
			std::string name = dialog.get_filename();			
			std::ofstream temp(name);
			temp << export_as_python(doc);
			}
		}
	}

void NotebookWindow::on_file_export_html_segment()
	{
	// FIXME: merge with on_file_export_html.
	Gtk::FileChooserDialog dialog("Please enter a file name for the HTML segment",
											Gtk::FILE_CHOOSER_ACTION_SAVE);

	dialog.set_transient_for(*this);
	dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	dialog.add_button("Select", Gtk::RESPONSE_OK);

	int result=dialog.run();

	switch(result) {
		case(Gtk::RESPONSE_OK): {
			std::string name = dialog.get_filename();			
			std::ofstream temp(name);
			temp << export_as_HTML(doc, true);
			}
		}
	}

// FIXME: this logic can go into DocumentThread to be system independent.

std::string NotebookWindow::save(const std::string& fn) const
	{
	// Make a backup first, just in case things go wrong.
	std::ifstream old(fn.c_str());
	std::ofstream temp(std::string(fn+"~").c_str());

	if(old) { // only backup if there is something to backup
		if(temp) {
			std::string ln;
			while(std::getline(old, ln)) {
				temp << ln << "\n";
				if(!temp) return "Error writing backup file";
				}
			}
		else {
			return "Failed to create backup file";
			}
		}

	std::string out = JSON_serialise(doc);
	std::ofstream file(fn);
	file << out << std::endl;
	return "";
	}

bool NotebookWindow::quit_safeguard(bool quit)
	{
	if(modified) {
		std::string mes;
		if(quit) {
			if(name.size()>0) mes="Save changes to "+name+" before closing?";
			else              mes="Save changes before closing?";
			}
		else {
			if(name.size()>0) mes="Save changes to "+name+" before continuing?";
			else              mes="Save changes before continuing?";
			}
		Gtk::MessageDialog md(mes, false, Gtk::MESSAGE_WARNING, 
									 Gtk::BUTTONS_NONE, true);
		md.set_transient_for(*this);
		md.set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
		md.add_button("Save before continuing",1);
		md.add_button("Cancel",2);
		if(quit)
			md.add_button("No need to save, quit now",3);
		else 
			md.add_button("No need to save", 3);
		int action=md.run();
		switch(action) {
			case 1: 
				on_file_save();
				return true;
			case 2:
				break;
			case 3:
				return true;
			}
		}
	else return true;

	return false;
	}

void NotebookWindow::on_file_quit()
	{
	// FIXME: this needs to not just close the current window, but also all
	// other ones.
	if(quit_safeguard(true)) 
		hide();
	}

void NotebookWindow::on_edit_undo()
	{
	undo();
	}

void NotebookWindow::on_edit_insert_above()
	{
	if(current_cell==doc.end()) return;

	DataCell newcell(DataCell::CellType::python, "");
	std::shared_ptr<ActionBase> action = 
		std::make_shared<ActionAddCell>(newcell, current_cell, ActionAddCell::Position::before);
	queue_action(action);
	process_data();
	}

void NotebookWindow::on_edit_insert_below()
	{
	if(current_cell==doc.end()) return;

	DataCell newcell(DataCell::CellType::python, "");
	std::shared_ptr<ActionBase> action = 
		std::make_shared<ActionAddCell>(newcell, current_cell, ActionAddCell::Position::after);
	queue_action(action);
	process_data();
	}

void NotebookWindow::on_edit_delete()
	{
	if(current_cell==doc.end()) return;

	DTree::sibling_iterator nxt=doc.next_sibling(current_cell);
	if(current_cell->textbuf=="" && doc.is_valid(nxt)==false) return; // Do not delete last cell if it is empty.

	std::shared_ptr<ActionBase> action = 
		std::make_shared<ActionPositionCursor>(current_cell, ActionPositionCursor::Position::next);
	queue_action(action);
	std::shared_ptr<ActionBase> action2 = 
		std::make_shared<ActionRemoveCell>(current_cell);
	queue_action(action2);
	process_data();
	}

void NotebookWindow::on_edit_split()
	{
	std::shared_ptr<ActionBase> action = std::make_shared<ActionSplitCell>(current_cell);
	queue_action(action);
	process_data();
	}

void NotebookWindow::on_edit_cell_is_python()
	{
	if(current_cell==doc.end()) return;

	if(current_cell->cell_type==DataCell::CellType::latex) {
		current_cell->cell_type = DataCell::CellType::python;
		update_cell(doc, current_cell);
		}
	}

void NotebookWindow::on_edit_cell_is_latex()
	{
	if(current_cell==doc.end()) return;

	if(current_cell->cell_type==DataCell::CellType::python) {
		current_cell->cell_type = DataCell::CellType::latex;
		update_cell(doc, current_cell);
		}
	}

void NotebookWindow::on_view_split()
	{
	canvasses.push_back(new NotebookCanvas());
	// Add the new canvas into the bottom pane of the last visible canvas.
	canvasses[canvasses.size()-2]->pack2(*canvasses.back(), true, true);
	build_visual_representation();
	canvasses.back()->show_all();
	canvasses[canvasses.size()-2]->set_position(canvasses[canvasses.size()-2]->get_height()/2.0);
	}

void NotebookWindow::on_view_close()
	{
	// FIXME: this always removes the last canvas, not the current one.
	if(canvasses.size()>1) {
		canvasses[canvasses.size()-2]->remove(*canvasses.back());
		NotebookCanvas *oldcanvas = canvasses.back();
		canvasses.pop_back();
		delete oldcanvas;
		}
	}

void NotebookWindow::on_run_cell()
	{
	if(read_only) return;

	// This is actually handled by the CodeInput widget, which ensures that the
	// DTree is up to date and then calls execute.
	
	VisualCell& actual = canvasses[current_canvas]->visualcells[&(*current_cell)];
	actual.inbox->edit.shift_enter_pressed();

//	cell_content_execute(current_cell, current_canvas);
	}

void NotebookWindow::on_run_runall()
	{
	// FIXME: move to DocumentThread

	DTree::sibling_iterator sib=doc.begin(doc.begin());
	while(sib!=doc.end(doc.begin())) {
		if(sib->cell_type==DataCell::CellType::python) 
			cell_content_execute(DTree::iterator(sib), current_canvas, false);
		++sib;
		}
	}

void NotebookWindow::on_run_runtocursor()
	{
	}

void NotebookWindow::on_run_stop()
	{
	compute->stop();
	}

void NotebookWindow::on_kernel_restart()
	{
	// FIXME: add warnings

	compute->restart_kernel();
	}

void NotebookWindow::on_help() const
	{
	if(current_cell==doc.end()) return;
	if(current_cell->cell_type!=DataCell::CellType::python) return;

	// Figure out the keyword under the cursor.
	VisualCell& actual = canvasses[current_canvas]->visualcells[&(*current_cell)];
	std::string before, after;
	actual.inbox->slice_cell(before, after);

	help_t help_type;
	std::string help_topic;
	help_type_and_topic(before, after, help_type, help_topic);

	bool ret=false;
	if(help_type==help_t::algorithm)
		ret=cdbapp->open_help(CMAKE_INSTALL_PREFIX"/share/cadabra2/manual/algorithms/"+help_topic+".cnb",
									 help_topic);
	if(help_type==help_t::property)
		ret=cdbapp->open_help(CMAKE_INSTALL_PREFIX"/share/cadabra2/manual/properties/"+help_topic+".cnb",
									 help_topic);

	if(!ret) {
		Gtk::MessageDialog md("No help available", false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
//		md.set_transient_for(*this);
		md.set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
		md.set_secondary_text("No help available for '"+help_topic+"'.\nNot all algorithms and properties have manual pages yet, sorry.");
		md.run();
		}
	}

void NotebookWindow::on_help_about()
	{
	Glib::RefPtr<Gdk::Pixbuf> logo=Gdk::Pixbuf::create_from_file(CMAKE_INSTALL_PREFIX"/share/cadabra2/images/cadabra2-gtk.png");

	Gtk::AboutDialog about;
	about.set_transient_for(*this);
	about.set_program_name("Cadabra");
	about.set_comments("A field-theory motivated approach to computer algebra");
	about.set_version(std::string("Version ")+CADABRA_VERSION_MAJOR+"."+CADABRA_VERSION_MINOR
							+"."+CADABRA_VERSION_PATCH
							+" (build "+CADABRA_VERSION_BUILD+" dated "+CADABRA_VERSION_DATE+")");
	std::vector<Glib::ustring> authors;
	authors.push_back("Kasper Peeters");
	about.set_authors(authors);
	about.set_copyright(std::string("\xC2\xA9 ")+COPYRIGHT_YEARS+std::string(" Kasper Peeters"));
	about.set_license_type(Gtk::License::LICENSE_GPL_3_0);
	about.set_website("http://cadabra.science");
	about.set_website_label("cadabra.science");
	about.set_logo(logo);
	std::vector<Glib::ustring> special;
	special.push_back("José M. Martín-García (for the xPerm canonicalisation code)");
	special.push_back("James Allen (for writing much of the factoring code)");
	special.push_back("Software Sustainability Institute");
	about.add_credit_section("Special thanks", special);
	about.run();
	}

void NotebookWindow::on_text_scaling_factor_changed(const std::string& key)
	{
	if(key=="text-scaling-factor") {
		scale = settings->get_double("text-scaling-factor");
		std::cout << "cadabra-client: text-scaling-factor = " << scale << std::endl;
		engine.set_scale(scale);
		engine.invalidate_all();
		engine.convert_all();

		auto it=canvasses.begin();
		while(it!=canvasses.end()) {
			(*it)->refresh_all();
			++it;
			}
		}
	}

void NotebookWindow::on_prefs_font_size(int num)
	{
	if(prefs.font_step==num) return;

	prefs.font_step=num;

//	std::string res=save_config();
//	if(res.size()>0) {
//		 Gtk::MessageDialog md("Error");
//		 md.set_secondary_text(res);
//		 md.set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
//		 md.run();
//		 }

	// std::cerr << "cadabra-client: prefs_font_size = " << num << std::endl;
	engine.set_font_size(12+(num*2));

	if(is_configured) {
		std::cerr << "cadabra-client: re-running TeX to change font size" << std::endl;
		// No point in running TeX on all cells if we have not yet had an
		// on_configure_event signal; that will come after us and then we will
		// have to run all again.
		engine.invalidate_all();
		engine.convert_all();
		
		for(auto& canvas: canvasses) {
			for(auto& visualcell: canvas->visualcells) {
				if(visualcell.first->cell_type==DataCell::CellType::python || 
					visualcell.first->cell_type==DataCell::CellType::latex) {
					visualcell.second.inbox->set_font_size(num);
					}
				}
			}
		
		for(unsigned int i=0; i<canvasses.size(); ++i) 
			canvasses[i]->refresh_all();
		}


//	// Hack.
//	auto screen = Gdk::Screen::get_default();
//	if(get_window()!=0) {
//		std::cerr << "invalidating" << std::endl;
//		get_window()->invalidate_rect(Gdk::Rectangle(0, 0, screen->get_width()/2, screen->get_height()*0.8),true);
//		queue_draw();
//		}
	}

bool NotebookWindow::idle_handler()
	{
#if GTKMM_MINOR_VERSION>=10
	for(auto& reveal: to_reveal) {
		reveal->set_reveal_child(true);
		}
	to_reveal.clear();
#endif	
	return false; // disconnect
	}
