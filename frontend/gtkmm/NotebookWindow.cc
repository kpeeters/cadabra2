
#include <iostream>
#include "NotebookWindow.hh"
#include <gtkmm/box.h>

using namespace cadabra;

NotebookWindow::NotebookWindow()
	: DocumentThread(this),
	  b_help(Gtk::Stock::HELP), b_stop(Gtk::Stock::STOP), b_undo(Gtk::Stock::UNDO), b_redo(Gtk::Stock::REDO),
	  modified(false)
	{
	// Connect the dispatcher.
	dispatcher.connect(sigc::mem_fun(*this, &NotebookWindow::process_todo_queue));

	// Setup menu.
	actiongroup=Gtk::ActionGroup::create();
	actiongroup->add( Gtk::Action::create("MenuFile", "_File") );
	actiongroup->add( Gtk::Action::create("MenuEdit", "_Edit") );
	actiongroup->add( Gtk::Action::create("MenuView", "_View") );
	actiongroup->add( Gtk::Action::create("MenuSettings", "_Settings") );
	actiongroup->add( Gtk::Action::create("MenuTutorial", "_Tutorial") );
	actiongroup->add( Gtk::Action::create("MenuFontSize", "Font size") );
	actiongroup->add( Gtk::Action::create("MenuBrainWired", "Brain wired for") );
	actiongroup->add( Gtk::Action::create("MenuHelp", "_Help") );

	add(topbox);
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
	kernel_string = "network error";
	dispatcher.emit();
	}

void NotebookWindow::process_todo_queue()
	{
	// Update the status/kernel messages into the corresponding widgets.
	std::lock_guard<std::mutex> guard(status_mutex);
	kernel_label.set_text(kernel_string);
	status_label.set_text(status_string);

	// Perform any ActionBase actions.

	}

void NotebookWindow::add_cell(DTree& tr, DTree::iterator it)
	{
	// Add a visual cell corresponding to this document cell in 
	// every canvas.
	
	Glib::RefPtr<Gtk::TextBuffer> global_buffer;

	for(unsigned int i=0; i<canvasses.size(); ++i) {

		// Create a cell of the appropriate type.

		VisualCell newcell;
		Gtk::Widget *w=0;
		switch(it->cell_type) {
			case DataCell::CellType::output:
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

				ci->edit.content_changed.connect( sigc::mem_fun(this, &NotebookWindow::cell_content_changed) );
				ci->edit.content_execute.connect( sigc::bind( sigc::mem_fun(this, &NotebookWindow::cell_content_execute),
																			 it ) );
				newcell.inbox = manage( ci );
				w=newcell.inbox;
				break;
				}
			default:
				throw std::logic_error("Unimplemented datacell type");
			}
		
		
		canvasses[i]->visualcells[&(*it)]=newcell;
		
		// Figure out where to store this new VisualCell in the GUI widget
		// tree by exploring the DTree near the new DataCell.
		
		DTree::iterator prev = DTree::previous_sibling(it);
		canvasses[i]->scrollbox.pack_start(*w, false, false);

		if(tr.is_valid(prev.node)==false) {
			// This node has no previous sibling.
			DTree::iterator parent = DTree::parent(it);
			if(tr.is_valid(parent)==false) {
				// This node has no parent either, it is a top-level cell.
				} 
			else {
				// Add as first child of parent.
				canvasses[i]->scrollbox.reorder_child(*w, 0);
				}
			}
		else {
			// Add in an existing sibling range.
			unsigned int index=tr.index(it);
			canvasses[i]->scrollbox.reorder_child(*w, index);
			}
		}
	}

void NotebookWindow::remove_cell(DTree&, DTree::iterator)
	{
	std::cout << "request to remove gui cell" << std::endl;
	}

void NotebookWindow::update_cell(DTree&, DTree::iterator)
	{
	std::cout << "request to update gui cell" << std::endl;
	}

bool NotebookWindow::cell_content_changed()
	{
	std::cerr << "canvas received content changed" << std::endl;
	return false;
	}

bool NotebookWindow::cell_content_execute(std::string content, DTree::iterator it)
	{
	std::cerr << "canvas received content exec " << content << std::endl;

	it->textbuf=content;

	return true;
	}

