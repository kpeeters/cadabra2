
#include <iostream>
#include "NotebookWindow.hh"

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

	set_size_request(800,800);
	update_title();
	show_all();

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

void NotebookWindow::add_cell(DTree::iterator)
	{
	}

void NotebookWindow::remove_cell(DTree::iterator)
	{
	}

void NotebookWindow::update_cell(DTree::iterator)
	{
	}
