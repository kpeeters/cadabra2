
#include <iostream>
#include "Actions.hh"
#include "NotebookWindow.hh"
#include "DataCell.hh"
#include <gtkmm/box.h>
#include <gtkmm/filechooserdialog.h>
#include <fstream>

using namespace cadabra;

NotebookWindow::NotebookWindow()
	: DocumentThread(this),
	  current_canvas(0),
	  b_help(Gtk::Stock::HELP), b_stop(Gtk::Stock::STOP), b_undo(Gtk::Stock::UNDO), b_redo(Gtk::Stock::REDO), modified(false)
	{
   // Connect the dispatcher.
	dispatcher.connect(sigc::mem_fun(*this, &NotebookWindow::process_todo_queue));
	
	// Setup menu.
	actiongroup = Gtk::ActionGroup::create();
	actiongroup->add( Gtk::Action::create("MenuFile", "_File") );
	actiongroup->add( Gtk::Action::create("Save", Gtk::Stock::SAVE), Gtk::AccelKey("<control>S"),
							sigc::mem_fun(*this, &NotebookWindow::on_file_save) );
	actiongroup->add( Gtk::Action::create("SaveAs", Gtk::Stock::SAVE_AS),
							sigc::mem_fun(*this, &NotebookWindow::on_file_save_as) );
	actiongroup->add( Gtk::Action::create("Quit", Gtk::Stock::QUIT),
							sigc::mem_fun(*this, &NotebookWindow::on_file_quit) );

	uimanager = Gtk::UIManager::create();
	uimanager->insert_action_group(actiongroup);
	add_accel_group(uimanager->get_accel_group());
	Glib::ustring ui_info =
		"<ui>"
		"  <menubar name='MenuBar'>"
		"    <menu action='MenuFile'>"
		"      <menuitem action='Save'/>"
		"      <menuitem action='SaveAs'/>"
		"      <separator/>"
		"      <menuitem action='Quit'/>"
		"    </menu>"
		"  </menubar>"
		"</ui>";

	uimanager->add_ui_from_string(ui_info);
	Gtk::Widget *menubar = uimanager->get_widget("/MenuBar");

	// Main box structure dividing the window.
	add(topbox);
	topbox.pack_start(*menubar, Gtk::PACK_SHRINK);
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
	b_stop.set_sensitive(false);
	b_run.set_label("Run all");
	b_run_to.set_label("Run to cursor");
	b_run_from.set_label("Run from cursor");
	b_kill.set_label("Restart kernel");
	buttonbox.pack_start(b_help, Gtk::PACK_SHRINK);
	buttonbox.pack_start(b_run, Gtk::PACK_SHRINK);
	buttonbox.pack_start(b_run_to, Gtk::PACK_SHRINK);
	buttonbox.pack_start(b_run_from, Gtk::PACK_SHRINK);
	buttonbox.pack_start(b_stop, Gtk::PACK_SHRINK);
	buttonbox.pack_start(b_kill, Gtk::PACK_SHRINK);

	// The three main widgets
	mainbox.pack_start(buttonbox, Gtk::PACK_SHRINK, 0);

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

void NotebookWindow::add_cell(const DTree& tr, DTree::iterator it)
	{
	// Add a visual cell corresponding to this document cell in 
	// every canvas.
	
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
				w=newcell.outbox;
				break;
			case DataCell::CellType::input: {
				CodeInput *ci;
				// Ensure that all CodeInput cells share the same text buffer.
				if(i==0) {
					ci = new CodeInput();
					global_buffer=ci->buffer;
					}
				else ci = new CodeInput(global_buffer);

				ci->edit.content_changed.connect( sigc::bind( sigc::mem_fun(this, &NotebookWindow::cell_content_changed), it, i ) );
				ci->edit.content_execute.connect( sigc::bind( sigc::mem_fun(this, &NotebookWindow::cell_content_execute), it, i ) );
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
			canvasses[i]->ebox.add(*w);
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
		if(parent->cell_type==DataCell::CellType::document)
			parentbox=parent_visual.document;
		else
			parentbox=parent_visual.inbox;

		std::cout << "adding cell to canvas " << i << std::endl;
		parentbox->pack_start(*w, false, false);	
		unsigned int index=tr.index(it);
		unsigned int numch=tr.number_of_children(parent);
		std::cout << "is index " << index << " vs " << numch << std::endl;
		if(index!=numch-1) {
			std::cout << "need to re-order" << std::endl;
			parentbox->reorder_child(*w, index);
			}
		}
	}

void NotebookWindow::remove_cell(const DTree& doc, DTree::iterator it)
	{
	std::cout << "request to remove gui cell" << std::endl;

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
		}	
	}

void NotebookWindow::update_cell(const DTree&, DTree::iterator)
	{
	std::cout << "request to update gui cell" << std::endl;
	}

void NotebookWindow::position_cursor(const DTree& doc, DTree::iterator it)
	{
	// std::cout << "positioning cursor at cell " << it->textbuf << std::endl;

	VisualCell& target = canvasses[current_canvas]->visualcells[&(*it)];
	target.inbox->edit.grab_focus();
	}

bool NotebookWindow::cell_content_changed(const std::string& content, DTree::iterator it, int canvas_number)
	{
	current_canvas=canvas_number;
	// std::cout << "received: " << content << std::endl;
	it->textbuf=content;

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
	compute->execute_cell(*it);

	return true;
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
