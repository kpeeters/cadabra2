
#include "Actions.hh"
#include "DocumentThread.hh"
#include "GUIBase.hh"

#include <typeinfo>
#include <iostream>
#include <set>
#include <string>
#include <fstream>

//#include <boost/config.hpp>

#include <internal/unistd.h>
#include <sys/types.h>
#include <json/json.h>
#ifndef EMSCRIPTEN
#include <glibmm/miscutils.h>
#include "Snoop.hh"
#endif
#include "Config.hh"

using namespace cadabra;

DocumentThread::DocumentThread(GUIBase* g)
	: gui(g), compute(0), disable_stacks(false)
	{
	// Setup logging.
	std::string version=std::string(CADABRA_VERSION_MAJOR)+"."+CADABRA_VERSION_MINOR+"."+CADABRA_VERSION_PATCH;
#ifndef EMSCRIPTEN
	snoop::log.init("Cadabra", version, "log.cadabra.science");
	snoop::log.set_sync_immediately(true);
#endif
	//	snoop::log(snoop::warn) << "Starting" << snoop::flush;

	}

void DocumentThread::on_interactive_output(const Json::Value& )
	{

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
	   std::make_shared<ActionPositionCursor>(one_it->id(), ActionPositionCursor::Position::in);
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

void DocumentThread::undo()
	{
	stack_mutex.lock();
	if(undo_stack.size()==0) {
		//std::cerr << "no entries left on the stack" << std::endl;
		stack_mutex.unlock();
		return;
		}

	disable_stacks=true;
	auto ua = undo_stack.top();
	//std::cerr << "Undo action " << typeid(*ua).name() << std::endl;

	redo_stack.push(ua);
	undo_stack.pop();
	ua->revert(*this, *gui);
	disable_stacks=false;

	stack_mutex.unlock();
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
		// std::cerr << "Executing action " << typeid(*ab).name() << " for " << ab->ref_id.id << std::endl;
		// Execute the action; this will run synchronously, so after
		// this returns the doc and visual representation have both been
		// updated.
		ab->execute(*this, *gui);
		// Lock the queue to remove the action just executed, and
		// add it to the undo stack.
		stack_mutex.lock();
		if(ab->undoable())
			undo_stack.push(ab);
		pending_actions.pop();
		}
	stack_mutex.unlock();
	}


DocumentThread::Prefs::Prefs(bool use_defaults)
	{
#ifndef EMSCRIPTEN
	config_path=std::string(Glib::get_user_config_dir()) + "/cadabra2.conf";
	if (!use_defaults) {
		std::ifstream f(config_path);
		if (f)
			f >> data;
		else {
			// Backwards compatibility, check to see if cadabra.conf exists
			// and if so take the is_registered variable from there
			std::ifstream old_f(std::string(Glib::get_user_config_dir()) + "/cadabra.conf");
			if (old_f) {
				std::string line;
				while (old_f.good()) {
					std::getline(old_f, line);
					if (line.find("registered=true") != std::string::npos) {
						data["is_registered"] = true;
						break;
						}
					}
				}
			}
		}
	font_step = data.get("font_step", 0).asInt();
	highlight = data.get("highlight", false).asBool();
	is_registered = data.get("is_registered", false).asBool();
	is_anonymous = data.get("is_anonymous", false).asBool();
	git_path = data.get("git_path", "").asString();
	python_path = data.get("python_path", "").asString();
	move_into_new_cell = data.get("move_into_new_cell", false).asBool();

	if(git_path=="")
		git_path="/usr/bin/git";
	// Get the colours for syntax highlighting.
	auto python_colours = data.get("colours", Json::Value()).get("python", Json::Value());
	colours["python"]["keyword"] = (python_colours.get("keyword", "RoyalBlue").asString());
	colours["python"]["operator"] = (python_colours.get("operator", "SlateGray").asString());
	colours["python"]["brace"] = (python_colours.get("brace", "SlateGray").asString());
	colours["python"]["string"] = (python_colours.get("string", "ForestGreen").asString());
	colours["python"]["comment"] = (python_colours.get("comment", "Silver").asString());
	colours["python"]["object"] = (python_colours.get("object", "DarkGray").asString());
	colours["python"]["number"] = (python_colours.get("number", "Sienna").asString());
	colours["python"]["maths"] = (python_colours.get("maths", "Olive").asString());
	colours["python"]["function"] = (python_colours.get("function", "FireBrick").asString());
	colours["python"]["decorator"] = (python_colours.get("decorator", "DarkViolet").asString());
	colours["python"]["class"] = (python_colours.get("class", "MediumOrchid").asString());

	auto latex_colours = data.get("colours", Json::Value()).get("latex", Json::Value());
	colours["latex"]["command"] = (latex_colours.get("command", "rgb(52,101,164)").asString());
	colours["latex"]["parameter"] = (latex_colours.get("brace", "rgb(245,121,0)").asString());
	colours["latex"]["comment"] = (latex_colours.get("comment", "Silver").asString());
	colours["latex"]["maths"] = (latex_colours.get("maths", "Sienna").asString());
#endif
	}

void DocumentThread::Prefs::save()
	{
	std::ofstream f(config_path);
	if (f) {
		data["font_step"] = font_step;
		data["highlight"] = highlight;
		data["is_registered"] = is_registered;
		data["is_anonymous"] = is_anonymous;
		data["python_path"] = python_path;
		data["move_into_new_cell"] = move_into_new_cell;
		for (const auto& lang : colours) {
			for (const auto& kw : lang.second)
				data["colours"][lang.first][kw.first] = kw.second;
			}
		data["git_path"] = git_path;
		f << data << '\n';
		}
	else
		std::cerr << "Warning: could not write to config file\n";
	}

void DocumentThread::set_user_details(const std::string& name, const std::string& email, const std::string& affiliation)
	{
#ifndef EMSCRIPTEN
	snoop::log("name") << name << snoop::flush;
	snoop::log("email") << email << snoop::flush;
	snoop::log("affiliation") << affiliation << snoop::flush;
#endif
	}

bool DocumentThread::help_type_and_topic(const std::string& before, const std::string& after,
      help_t& help_type, std::string& help_topic) const
	{
	help_t objtype=help_t::algorithm;
	if(! (before.size()==0 && after.size()==0) ) {
		// We provide help for properties, algorithms and reserved node
		// names.  Properties are delimited to the left by '::' and to
		// the right by anything non-alnum. Algorithms are delimited to
		// the left by non-alnum except '_' and to the right by '('. Reserved node
		// names are TeX symbols, starting with '\'.
		//
		// So scan the 'before' string for a left-delimiter and the 'after' string
		// for a right-delimiter.

		int lpos=before.size()-1;
		while(lpos>=0) {
			if(before[lpos]==':' && lpos>0 && before[lpos-1]==':') {
				objtype=help_t::property;
				break;
				}
			if(before[lpos]=='\\') {
				objtype=help_t::latex;
				break;
				}
			if(isalnum(before[lpos])==0 && before[lpos]!='_') {
				objtype=help_t::algorithm;
				break;
				}
			--lpos;
			}
		if(objtype==help_t::none) return false;
		++lpos;

		size_t rpos=0;
		while(rpos<after.size()) {
			if(objtype==help_t::property) {
				if(isalnum(after[rpos])==0)
					break;
				}
			else if(objtype==help_t::algorithm) {
				if(after[rpos]=='(')
					break;
				}
			else if(objtype==help_t::latex) {
				if(isalnum(after[rpos])==0 && after[rpos]!='_')
					break;
				}
			++rpos;
			}
		help_topic=before.substr(lpos)+after.substr(0,rpos);
		}

	help_type=objtype;
	return true;
	}
