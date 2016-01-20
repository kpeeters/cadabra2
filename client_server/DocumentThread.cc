
#include "Actions.hh"
#include "DocumentThread.hh"
#include "GUIBase.hh"

#include <iostream>
#include <set>
#include <string>
#include <fstream>

#include <boost/config.hpp>
#include <boost/program_options/detail/config_file.hpp>
#include <boost/program_options/parsers.hpp>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <snoop/Snoop.hh>

using namespace cadabra;

DocumentThread::DocumentThread(GUIBase* g)
	: gui(g), compute(0)
	{
	// Setup logging.
	snoop::log.init("Cadabra", "2.00", "/tmp/cadabra_log.sql", "log.cadabra.science");
	snoop::log.set_sync_immediately(true);
	snoop::log(snoop::warn) << "Program started" << snoop::flush;	

	struct passwd *pw = getpwuid(getuid());
	const char *homedir = pw->pw_dir;

	std::ifstream config(homedir + std::string("/.config/cadabra.conf"));
	if(config) {
		std::cerr << "cadabra-client: reading config file" << std::endl;
		std::set<std::string> options;
		options.insert("registered");

		for(boost::program_options::detail::config_file_iterator i(config, options), e ; i != e; ++i) {
			// FIXME: http://stackoverflow.com/questions/24701547/how-to-parse-boolean-option-in-config-file
			if(i->string_key=="registered") registered=(i->value[0]=="true");
			}
		}
	else {
		// First time run; create config file.
		std::ofstream config(homedir + std::string("/.config/cadabra.conf"));		
		registered=false;
		if(config) {
			// config file written.
			}
		else {
			std::cerr << "cadabra-client: cannot write ~/.config/cadabra.conf" << std::endl;
			}
		}
	}

void DocumentThread::set_compute_thread(ComputeThread *cl)
	{
	compute = cl;
	}

void DocumentThread::new_document()
	{
	// Setup a single-cell document. This operation itself cannot be undone,
	// so we do it directly on the doc, not using Actions.

	DataCell top(DataCell::CellType::document);
	DTree::iterator doc_it = doc.set_head(top);
	gui->add_cell(doc, doc_it, false);

	// One Python input cell in the empty document.

	DataCell one(DataCell::CellType::python, "");
	DTree::iterator one_it = doc.append_child(doc_it, one);
	gui->add_cell(doc, one_it, false);

	// Put a 'position cursor' action on the stack to be executed as
	// soon as the GUI is up.

	std::shared_ptr<ActionBase> actionpos =
		std::make_shared<ActionPositionCursor>(one_it, ActionPositionCursor::Position::in);
	queue_action(actionpos);
	}

void DocumentThread::build_visual_representation()
	{
	// Because the add_cell method figures out by itself where to generate the VisualCell,
	// we only have feed all cells in turn.

	DTree::iterator doc_it=doc.begin();
	while(doc_it!=doc.end()) {
//		std::cout << doc_it->textbuf << std::endl;
		gui->add_cell(doc, doc_it, false);
		++doc_it;
		}
	}

//const DTree& DocumentThread::dtree() 
//	{
//	return doc;
//	}

void DocumentThread::queue_action(std::shared_ptr<ActionBase> ab) 
	{
	std::lock_guard<std::mutex> guard(stack_mutex);
	pending_actions.push(ab);
	}


void DocumentThread::process_action_queue()
	{
	// FIXME: we certainly do not want any two threads to run this at the same time,
	// but that is not guaranteed.

	stack_mutex.lock();
	while(pending_actions.size()>0) {
		std::shared_ptr<ActionBase> ab = pending_actions.front();
		// Unlock the queue while we are processing this particular action.
		stack_mutex.unlock();
		ab->pre_execute(*this);
		ab->update_gui(doc, *gui);
		ab->post_execute(*this);
		// Lock the queue to remove the running action.
		stack_mutex.lock();
		pending_actions.pop();
		}
	stack_mutex.unlock();
	}

bool DocumentThread::is_registered() const
	{
	return registered;
	}

void DocumentThread::set_email(const std::string& email) 
	{
	std::cerr << "Received email " << email << std::endl;
	snoop::log(snoop::info) << email << snoop::flush;	
	}
