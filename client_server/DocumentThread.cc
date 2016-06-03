
#include "Actions.hh"
#include "DocumentThread.hh"
#include "GUIBase.hh"

#include <typeinfo>
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

#include "Snoop.hh"

using namespace cadabra;

DocumentThread::DocumentThread(GUIBase* g)
	: gui(g), compute(0)
	{
	// Setup logging.
	snoop::log.init("Cadabra", "2.0", "log.cadabra.science");
	snoop::log.set_sync_immediately(true);
//	snoop::log(snoop::warn) << "Starting" << snoop::flush;	

	struct passwd *pw = getpwuid(getuid());
	const char *homedir = pw->pw_dir;

	std::ifstream config(homedir + std::string("/.config/cadabra.conf"));
	if(config) {
		// std::cerr << "cadabra-client: reading config file" << std::endl;
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
			throw std::logic_error("cadabra-client: Cannot write ~/.config/cadabra.conf");
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

void DocumentThread::load_from_string(const std::string& json)
	{
	std::lock_guard<std::mutex> guard(stack_mutex);
	pending_actions=std::queue<std::shared_ptr<ActionBase> >(); // clear queue
	doc.clear();
	JSON_deserialise(json, doc);
	gui->remove_all_cells();
	build_visual_representation();
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
	// but that is not guaranteed. Actions should always be run on the GUI thread.
	// This absolutely has to be run on the main GUI thread.
//	assert(main_thread_id==std::this_thread::get_id());


	stack_mutex.lock();
	while(pending_actions.size()>0) {
		std::shared_ptr<ActionBase> ab = pending_actions.front();
		// Unlock the action queue while we are processing this particular action,
		// so that other actions can be added which we run.
		stack_mutex.unlock();
//		std::cerr << "Executing action " << typeid(*ab).name() << std::endl;
		ab->execute(*this, *gui);
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

void DocumentThread::set_user_details(const std::string& name, const std::string& email, const std::string& affiliation) 
	{
	snoop::log("name") << name << snoop::flush;	
	snoop::log("email") << email << snoop::flush;	
	snoop::log("affiliation") << affiliation << snoop::flush;	

	// FIXME: make this a generic function to write all our config data.
	struct passwd *pw = getpwuid(getuid());
	const char *homedir = pw->pw_dir;
	std::ofstream config(homedir + std::string("/.config/cadabra.conf"));
	config << "registered=true" << std::endl;
	}
