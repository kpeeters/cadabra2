
#include <iostream>
#include "Actions.hh"
#include "NotebookWindow.hh"
#include "DataCell.hh"
#include <gtkmm/box.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/messagedialog.h>
#include <fstream>

using namespace cadabra;

NotebookWindow::NotebookWindow()
	: DocumentThread(this),
	  current_canvas(0),
//	  b_help(Gtk::Stock::HELP), b_stop(Gtk::Stock::STOP), b_undo(Gtk::Stock::UNDO), b_redo(Gtk::Stock::REDO), 
	  modified(false)
	{
   // Connect the dispatcher.
	dispatcher.connect(sigc::mem_fun(*this, &NotebookWindow::process_todo_queue));

	// Setup styling.
	css_provider = Gtk::CssProvider::create();
	Glib::ustring data = "GtkTextView { color: blue; margin-left: 15px; margin-top: 20px; margin-bottom: 0px; padding: 20px; padding-bottom: 0px; }";
	if(!css_provider->load_from_data(data)) {
		std::cerr << "Failed to parse widget css information." << std::endl;
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
	actiongroup->add( Gtk::Action::create("Quit", Gtk::Stock::QUIT),
							sigc::mem_fun(*this, &NotebookWindow::on_file_quit) );
	actiongroup->add( Gtk::Action::create("MenuEdit", "_Edit") );
	actiongroup->add( Gtk::Action::create("EditUndo", Gtk::Stock::UNDO),
							sigc::mem_fun(*this, &NotebookWindow::on_edit_undo) );
	actiongroup->add( Gtk::Action::create("MenuRun", "_Run") );
	actiongroup->add( Gtk::Action::create("RunAll", Gtk::Stock::GO_FORWARD, "Run all"),
							sigc::mem_fun(*this, &NotebookWindow::on_run_runall) );
	actiongroup->add( Gtk::Action::create("RunToCursor", Gtk::Stock::GOTO_LAST, "Run to cursor"),
							sigc::mem_fun(*this, &NotebookWindow::on_run_runtocursor) );
	actiongroup->add( Gtk::Action::create("RunStop", Gtk::Stock::STOP, "Stop"),
							sigc::mem_fun(*this, &NotebookWindow::on_run_stop) );
	actiongroup->add( Gtk::Action::create("MenuKernel", "_Kernel") );
	actiongroup->add( Gtk::Action::create("KernelRestart", Gtk::Stock::REFRESH, "Restart"),
							sigc::mem_fun(*this, &NotebookWindow::on_kernel_restart) );

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
		"      <separator/>"
		"      <menuitem action='Quit'/>"
		"    </menu>"
		"    <menu action='MenuEdit'>"
		"      <menuitem action='EditUndo' />"
		"    </menu>"
		"    <menu action='MenuRun'>"
		"      <menuitem action='RunAll' />"
		"      <menuitem action='RunToCursor' />"
		"      <menuitem action='RunStop' />"
		"    </menu>"
		"    <menu action='MenuKernel'>"
		"      <menuitem action='KernelRestart' />"
		"    </menu>"
		"  </menubar>"
		"  <toolbar name='ToolBar'>"
		"    <toolitem action='Open' />"
		"    <toolitem action='RunAll' name='run all'/>"
		"    <toolitem action='RunStop' />"
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
	canvasses.push_back(manage( new NotebookCanvas() ));
	mainbox.pack_start(*canvasses[0], Gtk::PACK_EXPAND_WIDGET, 0);
	mainbox.pack_start(*canvasses[1], Gtk::PACK_EXPAND_WIDGET, 0);


	// Window size and title, and ready to go.
	set_size_request(800,800);
	update_title();
	show_all();

	new_document();
	}

NotebookWindow::~NotebookWindow()
	{
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
	Gtk::Widget *stop = uimanager->get_widget("/ToolBar/RunStop");
	stop->set_sensitive(s);
	stop = uimanager->get_widget("/MenuBar/MenuRun/RunStop");
	stop->set_sensitive(s);	
	}

void NotebookWindow::process_data() 
	{
	std::cout << "notified" << std::endl;
	dispatcher.emit();
	}


void NotebookWindow::on_connect()
	{
	std::lock_guard<std::mutex> guard(status_mutex);
	kernel_string = "connected";
	dispatcher.emit();
	}

void NotebookWindow::on_disconnect()
	{
	std::lock_guard<std::mutex> guard(status_mutex);
	kernel_string = "not connected";
	dispatcher.emit();
	}

void NotebookWindow::on_network_error()
	{
	std::lock_guard<std::mutex> guard(status_mutex);
	kernel_string = "cannot reach server, retrying...";
	dispatcher.emit();
	}

void NotebookWindow::process_todo_queue()
	{
	// Update the status/kernel messages into the corresponding widgets.
	std::lock_guard<std::mutex> guard(status_mutex);
	kernel_label.set_text(kernel_string);
	status_label.set_text(status_string);

	// Perform any ActionBase actions.
	process_action_queue();
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
		set_stop_sensitive( compute->number_of_cells_running()>0 );
	
	Glib::RefPtr<Gtk::TextBuffer> global_buffer;
	

	for(unsigned int i=0; i<canvasses.size(); ++i) {

		// Create a cell of the appropriate type.

		VisualCell newcell;
		Gtk::Widget *w=0;
		switch(it->cell_type) {
			case DataCell::CellType::document:
				newcell.document = manage( new Gtk::VBox() );
				w=newcell.document;
				break;
			case DataCell::CellType::output:
				// FIXME: would be good to share the output of TeXView too.
				newcell.outbox = manage( new TeXView(engine, it->textbuf) );
				newcell.outbox->tex_error.connect( sigc::bind( sigc::mem_fun(this, &NotebookWindow::on_tex_error), it ) );
				w=newcell.outbox;
				break;
			case DataCell::CellType::input: {
				CodeInput *ci;
				// Ensure that all CodeInput cells share the same text buffer.
				if(i==0) {
					ci = new CodeInput(it->textbuf);
					global_buffer=ci->buffer;
					}
				else ci = new CodeInput(global_buffer);
				ci->get_style_context()->add_provider(css_provider, GTK_STYLE_PROVIDER_PRIORITY_USER);

				ci->edit.content_changed.connect( sigc::bind( sigc::mem_fun(this, &NotebookWindow::cell_content_changed), it, i ) );
				ci->edit.content_execute.connect( sigc::bind( sigc::mem_fun(this, &NotebookWindow::cell_content_execute), it, i ) );
				ci->edit.cell_got_focus.connect( sigc::bind( sigc::mem_fun(this, &NotebookWindow::cell_got_focus), it, i ) );
				newcell.inbox = manage( ci );
				w=newcell.inbox;
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
		if(visible)
			w->show_all();
		}
	}

void NotebookWindow::remove_cell(const DTree& doc, DTree::iterator it)
	{
//	std::cout << "request to remove gui cell" << std::endl;

	// Can only remove cells which have a parent (i.e. not the top-level document cell).

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
		parentbox->remove(*actual.inbox);
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

void NotebookWindow::update_cell(const DTree&, DTree::iterator)
	{
	std::cout << "request to update gui cell" << std::endl;
	}

void NotebookWindow::position_cursor(const DTree& doc, DTree::iterator it)
	{
	std::cout << "positioning cursor at cell " << it->textbuf << std::endl;
	set_stop_sensitive( compute->number_of_cells_running()>0 );

	VisualCell& target = canvasses[current_canvas]->visualcells[&(*it)];
	target.inbox->edit.grab_focus();
	current_cell=it;
	}

bool NotebookWindow::cell_content_changed(const std::string& content, DTree::iterator it, int canvas_number)
	{
	current_canvas=canvas_number;
	// std::cout << "received: " << content << std::endl;
	it->textbuf=content;

	return false;
	}

bool NotebookWindow::cell_got_focus(DTree::iterator it, int canvas_number)
	{
	current_cell=it;
	current_canvas=canvas_number;
	return false;
	}

bool NotebookWindow::cell_content_execute(DTree::iterator it, int canvas_number)
	{
	current_canvas=canvas_number;
	// Remove child nodes, if any.
	// FIXME: use ActionRemoveCell so we can undo.
	DTree::sibling_iterator sib=doc.begin(it);
	while(sib!=doc.end(it)) {
		std::cout << "removing one output cell" << std::endl;

		std::shared_ptr<ActionBase> action = std::make_shared<ActionRemoveCell>(sib);
		queue_action(action);

		++sib;
		}

	// Execute
	set_stop_sensitive(true);
	compute->execute_cell(*it);

	return true;
	}

bool NotebookWindow::on_tex_error(const std::string& str, DTree::iterator it)
	{
	Gtk::MessageDialog md("TeX error", false, Gtk::MESSAGE_WARNING, 
								 Gtk::BUTTONS_OK, true);
	md.set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
	md.set_secondary_text(str);
	md.run();
	return true;
	}

void NotebookWindow::on_file_new()
	{
	}

void NotebookWindow::on_file_open()
	{
	std::cout << "open" << std::endl;
	
	Gtk::FileChooserDialog dialog("Please choose a notebook to open",
											Gtk::FILE_CHOOSER_ACTION_OPEN);

	dialog.set_transient_for(*this);
	dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	dialog.add_button("Select", Gtk::RESPONSE_OK);

	int result=dialog.run();

	switch(result) {
		case(Gtk::RESPONSE_OK): {
			std::string filename = dialog.get_filename();			
			std::ifstream file(filename);
			std::string content, line;
			
			while(std::getline(file, line)) 
				content+=line;

			doc.clear();
			JSON_deserialise(content, doc);
			remove_all_cells();
			build_visual_representation();
			engine.convert_all();
			mainbox.show_all();
			
//			file << out << std::endl;
			break;
			}
		}
	}

void NotebookWindow::on_file_save()
	{
	on_file_save_as();
	}

void NotebookWindow::on_file_save_as()
	{
	std::string out = JSON_serialise(doc);

	Gtk::FileChooserDialog dialog("Please choose a file name",
											Gtk::FILE_CHOOSER_ACTION_SAVE);

	dialog.set_transient_for(*this);
	dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
	dialog.add_button("Select", Gtk::RESPONSE_OK);

	int result=dialog.run();

	switch(result) {
		case(Gtk::RESPONSE_OK): {
			std::string filename = dialog.get_filename();			
			std::ofstream file(filename);
			file << out << std::endl;
			break;
			}
		}
	}

void NotebookWindow::on_file_quit()
	{
	}

void NotebookWindow::on_edit_undo()
	{
	}

void NotebookWindow::on_run_runall()
	{
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
