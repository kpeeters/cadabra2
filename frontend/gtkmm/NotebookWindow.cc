
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
#include <fstream>
#if GTKMM_MINOR_VERSION < 10
#include <gtkmm/main.h>
#endif

using namespace cadabra;

NotebookWindow::NotebookWindow(Cadabra *c)
	: DocumentThread(this),
	  cdbapp(c),
	  current_canvas(0),
//	  b_help(Gtk::Stock::HELP), b_stop(Gtk::Stock::STOP), b_undo(Gtk::Stock::UNDO), b_redo(Gtk::Stock::REDO), 
	  kernel_spinner_status(false),
	  modified(false)
	{
   // Connect the dispatcher.
	dispatcher.connect(sigc::mem_fun(*this, &NotebookWindow::process_todo_queue));

	// Query high-dpi settings. For now only for cinnamon.
#ifdef __APPLE__
	scale = 1.0;
#else
	settings = Gio::Settings::create((strcmp(std::getenv("DESKTOP_SESSION"), "cinnamon") == 0) ? "org.cinnamon.desktop.interface" : "org.gnome.desktop.interface");
	scale = settings->get_double("text-scaling-factor");
#endif
	engine.set_scale(scale);

#ifndef __APPLE__
	settings->signal_changed().connect(
		sigc::mem_fun(*this, &NotebookWindow::on_text_scaling_factor_changed));
#endif

	// Setup styling. Note that 'margin-left' and so on do not work; you need
	// to use 'padding'. However, 'padding-top' fails because it does not make the
   // widget larger enough... So we still do that with set_margin_top(...).
	css_provider = Gtk::CssProvider::create();
	// padding-left: 20px; does not work on some versions of gtk, so we use margin in CodeInput
	Glib::ustring data = "GtkTextView { color: blue;  }\n";
	data += "GtkTextView { background: white; -GtkWidget-cursor-aspect-ratio: 0.2; }\n";
	data += "*:focused { background-color: #eee; }\n";
	data += "*:selected { background-color: #ccc; }\n";
	data += "#ImageView { background-color: white; transition-property: padding, background-color; transition-duration: 1s; }\n";

	if(!css_provider->load_from_data(data)) {
		throw std::logic_error("Failed to parse widget CSS information.");
		}
	auto screen = Gdk::Screen::get_default();
	Gtk::StyleContext::add_provider_for_screen(screen, css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	// Setup menu.
	actiongroup = Gtk::ActionGroup::create();
	actiongroup->add( Gtk::Action::create("MenuFile", "_File") );
	actiongroup->add( Gtk::Action::create("New", Gtk::Stock::NEW), Gtk::AccelKey("<control>N"),
							sigc::mem_fun(*this, &NotebookWindow::on_file_new) );
	actiongroup->add( Gtk::Action::create("Open", Gtk::Stock::OPEN), Gtk::AccelKey("<control>O"),
							sigc::mem_fun(*this, &NotebookWindow::on_file_open) );
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
	actiongroup->add( Gtk::Action::create("EditUndo", Gtk::Stock::UNDO),
							sigc::mem_fun(*this, &NotebookWindow::on_edit_undo) );
	actiongroup->add( Gtk::Action::create("EditInsertAbove", "Insert cell above"),
							sigc::mem_fun(*this, &NotebookWindow::on_edit_insert_above) );
	actiongroup->add( Gtk::Action::create("EditInsertBelow", "Insert cell below"),
							sigc::mem_fun(*this, &NotebookWindow::on_edit_insert_below) );
	actiongroup->add( Gtk::Action::create("EditDelete", "Delete cell"), Gtk::AccelKey("<ctrl>Delete"),
							sigc::mem_fun(*this, &NotebookWindow::on_edit_delete) );
	actiongroup->add( Gtk::Action::create("EditMakeCellTeX", "Cell is LaTeX"), Gtk::AccelKey("<control><shift>L"),
							sigc::mem_fun(*this, &NotebookWindow::on_edit_cell_is_latex) );
	actiongroup->add( Gtk::Action::create("EditMakeCellPython", "Cell is Python"), Gtk::AccelKey("<control><shift>P"),
							sigc::mem_fun(*this, &NotebookWindow::on_edit_cell_is_python) );

	actiongroup->add( Gtk::Action::create("MenuView", "_View") );
	actiongroup->add( Gtk::Action::create("ViewSplit", "Split view"),
							sigc::mem_fun(*this, &NotebookWindow::on_view_split) );
	actiongroup->add( Gtk::Action::create("ViewClose", "Close view"),
							sigc::mem_fun(*this, &NotebookWindow::on_view_close) );

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
		"      <separator/>"
		"      <menuitem action='Save'/>"
		"      <menuitem action='SaveAs'/>"
		"      <menuitem action='ExportHtml'/>"
		"      <menuitem action='ExportHtmlSegment'/>"
		"      <menuitem action='ExportLaTeX'/>"
		"      <menuitem action='ExportPython'/>"
		"      <separator/>"
		"      <menuitem action='Quit'/>"
		"    </menu>"
		"    <menu action='MenuEdit'>"
		"      <menuitem action='EditUndo' />"
		"      <separator/>"
		"      <menuitem action='EditInsertAbove' />"
		"      <menuitem action='EditInsertBelow' />"
		"      <menuitem action='EditDelete' />"
		"      <separator/>"
		"      <menuitem action='EditMakeCellTeX' />"
		"      <menuitem action='EditMakeCellPython' />"
		"    </menu>"
		"    <menu action='MenuView'>"
		"      <menuitem action='ViewSplit' />"
		"      <menuitem action='ViewClose' />"
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
		"    </menu>"
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

	// Buttons
	set_stop_sensitive(false);

	// The three main widgets
//	mainbox.pack_start(buttonbox, Gtk::PACK_SHRINK, 0);

	// We always have at least one canvas.
	canvasses.push_back(manage( new NotebookCanvas() ));
//	canvasses.push_back(manage( new NotebookCanvas() ));
	mainbox.pack_start(*canvasses[0], Gtk::PACK_EXPAND_WIDGET, 0);
//	mainbox.pack_start(*canvasses[1], Gtk::PACK_EXPAND_WIDGET, 0);


	// Window size and title, and ready to go.
	set_default_size(screen->get_width()/2, screen->get_height()*0.8);
	// FIXME: the subtraction for the margin and scrollbar made below
	// is estimated but should be computed.
	engine.set_geometry(screen->get_width()/2 - 2*30);
	update_title();
	show_all();
	kernel_spinner.hide();

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
	if(cfg->width != last_configure_width) 
		engine.set_geometry(cfg->width-2*30);

	bool ret=Gtk::Window::on_configure_event(cfg);
	
	if(cfg->width != last_configure_width) {
		last_configure_width = cfg->width;
		try {
			std::cerr << "running TeX" << std::endl;
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

void NotebookWindow::update_title()
	{
	if(name.size()>0) {
		if(modified)
			set_title("Cadabra: "+name+"*");
		else
			set_title("Cadabra: "+name);
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

	if(kernel_string=="not connected") {
		Gtk::MessageDialog md("Kernel crashed", false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
		md.set_transient_for(*this);
		md.set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
		md.set_secondary_text("The kernel crashed unexpectedly, and has been restarted. You will need to re-run all cells.");
		md.run();
		}

	running=false;
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

				w=newcell.outbox;
				break;

			case DataCell::CellType::python:
			case DataCell::CellType::latex: {
				CodeInput *ci;
				// Ensure that all CodeInput cells share the same text buffer.
				if(i==0) {
					ci = new CodeInput(it, it->textbuf,scale);
					global_buffer=ci->buffer;
					}
				else ci = new CodeInput(it, global_buffer,scale);
				ci->get_style_context()->add_provider(css_provider, GTK_STYLE_PROVIDER_PRIORITY_USER);

				ci->edit.content_changed.connect( 
					sigc::bind( sigc::mem_fun(this, &NotebookWindow::cell_content_changed), i ) );
				ci->edit.content_execute.connect( 
				sigc::bind( sigc::mem_fun(this, &NotebookWindow::cell_content_execute), i ) );
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

//	if(current_cell!=doc.end()) 
//		scroll_into_view(current_cell);
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
		// The pointers are all in a union, and Gtkmm does not care
		// about the precise type, so we just remove imagebox, knowing
		// that it may actually be an inbox or outbox.
		parentbox->remove(*actual.imagebox);
		// The above does not delete the Gtk widget, despite having been
		// wrapped in manage at construction. So we have to delete it 
		// ourselves. Fortunately the container does not try to delete
		// it again in its destructor.
		delete actual.imagebox;
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
		if(it->cell_type==DataCell::CellType::python || it->cell_type==DataCell::CellType::latex) 
			vc.inbox->queue_draw();
		}
	
	}

void NotebookWindow::position_cursor(const DTree& doc, DTree::iterator it)
	{
//	if(it==doc.end()) return;
//	std::cerr << "cadabra-client: positioning cursor at cell " << it->textbuf << std::endl;
	set_stop_sensitive( compute->number_of_cells_executing()>0 );

	if(canvasses[current_canvas]->visualcells.find(&(*it))==canvasses[current_canvas]->visualcells.end()) {
		std::cerr << "cadabra-client: Cannot find cell to position cursor." << std::endl;
		return;
		}

	VisualCell& target = canvasses[current_canvas]->visualcells[&(*it)];

	// Grab widgets focus, which will scroll it into view. If the widget has not yet
	// had its size and position allocated, we need to setup a signal handler which
	// gets fires as soon as size/position allocation happens.

	Gtk::Allocation alloc=target.inbox->get_allocation();
	if(alloc.get_y()!=-1)
		target.inbox->edit.grab_focus();
	else {
		grab_connection = target.inbox->signal_size_allocate().connect(
			sigc::bind(
				sigc::mem_fun(*this, &NotebookWindow::on_widget_size_allocate),
				&(target.inbox->edit)
						  ));
		}
	
	current_cell=it;
	}

void NotebookWindow::scroll_into_view(DTree::iterator it)
	{
	if(current_canvas<0 || current_canvas>=(int)canvasses.size()) return;
	if(canvasses[current_canvas]->visualcells.find(&(*it))==canvasses[current_canvas]->visualcells.end()) {
		std::cerr << "cadabra-client: Cannot find cell to scroll into view." << std::endl;
		return;
		}

	VisualCell& target = canvasses[current_canvas]->visualcells[&(*it)];
	// std::cerr << "Scrolling into view " << it->textbuf << std::endl;

	// Grab widgets focus, which will scroll it into view. If the widget has not yet
	// had its size and position allocated, we need to setup a signal handler which
	// gets fires as soon as size/position allocation happens.

	Gtk::Allocation alloc=target.inbox->get_allocation();
	if(alloc.get_y()!=-1) {
		// std::cerr << "grabbing focus" << std::endl;
		target.inbox->edit.grab_focus();
		}
	else {
		grab_connection = target.inbox->signal_size_allocate().connect(
			sigc::bind(
				sigc::mem_fun(*this, &NotebookWindow::on_widget_size_allocate),
				&(target.inbox->edit)
						  ));
		}
	}

void NotebookWindow::on_widget_size_allocate(Gtk::Allocation&, Gtk::Widget *w)
	{
	grab_connection.disconnect();
	w->grab_focus();
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
	current_canvas=canvas_number;
	// std::cout << "received: " << content << std::endl;
	it->textbuf=content;

	dim_output_cells(it);

	modified=true;
	update_title();

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

bool NotebookWindow::cell_content_execute(DTree::iterator it, int canvas_number)
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
	compute->execute_cell(it);

	return true;
	}

bool NotebookWindow::on_tex_error(const std::string& str, DTree::iterator it)
	{
	Gtk::MessageDialog md("TeX error", false, Gtk::MESSAGE_WARNING, 
								 Gtk::BUTTONS_OK, true);
	md.set_transient_for(*this);
	md.set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
	md.set_secondary_text(str);
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
		position_cursor(doc, doc.begin(doc.begin()));
		name="";
		update_title();
		}
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
	if(quit_safeguard(true)) 
		hide();
	}

void NotebookWindow::on_edit_undo()
	{
	// FIXME: to be implemented
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
			cell_content_execute(DTree::iterator(sib), current_canvas);
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
	cdbapp->open_help(CMAKE_INSTALL_PREFIX"/share/cadabra2/manual/algorithms/distribute.cnb");
	}

void NotebookWindow::on_help_about()
	{
	Glib::RefPtr<Gdk::Pixbuf> logo=Gdk::Pixbuf::create_from_file(CMAKE_INSTALL_PREFIX"/share/cadabra2/images/cadabra2.png");

	Gtk::AboutDialog about;
	about.set_transient_for(*this);
	about.set_program_name("Cadabra");
	about.set_comments("A field-theory motivated approach to computer algebra");
	about.set_version("Version "+std::to_string(CADABRA_VERSION_MAJOR)+"."+std::to_string(CADABRA_VERSION_MINOR)
							+" (build "+std::string(CADABRA_VERSION_BUILD)+")");
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
