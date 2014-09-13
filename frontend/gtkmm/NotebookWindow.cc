
#include "NotebookWindow.hh"

cadabra::NotebookWindow::NotebookWindow()
	: modified(false)
	{
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

	// The three main widgets
	mainbox.pack_start(buttonbox, Gtk::PACK_SHRINK, 0);
//	buttonbox.pack_start(statusbox, Gtk::PACK_EXPAND_WIDGET, 0);

	update_title();
	show_all();
	}

cadabra::NotebookWindow::~NotebookWindow()
	{
	}

void cadabra::NotebookWindow::update_title()
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
