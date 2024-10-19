
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

void DocumentThread::on_interactive_output(const nlohmann::json& )
	{

	}

void DocumentThread::set_progress(const std::string& msg, int cur_step, int total_steps)
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
	// std::cerr << "DocumentThread::undo: undo_stack.size() == " << undo_stack.size() << std::endl;
	ua->revert(*this, *gui);
	disable_stacks=false;

	stack_mutex.unlock();
	}

void DocumentThread::redo()
	{
	stack_mutex.lock();
	if(redo_stack.size()==0) {
		//std::cerr << "no entries left on the stack" << std::endl;
		stack_mutex.unlock();
		return;
		}

	disable_stacks=true;
	auto ua = redo_stack.top();
	//std::cerr << "Undo action " << typeid(*ua).name() << std::endl;

	undo_stack.push(ua);
	redo_stack.pop();
	ua->execute(*this, *gui);
	disable_stacks=false;

	stack_mutex.unlock();
	}

void DocumentThread::build_visual_representation()
	{
	// Because the add_cell method figures out by itself where to generate the VisualCell,
	// we only have feed all cells in turn.

	DTree::iterator doc_it=doc.begin();
	while(doc_it!=doc.end()) {
		// std::cout << "ADDING:" << doc_it->textbuf << std::endl;
		gui->add_cell(doc, doc_it, false);
		++doc_it;
		}
	}

//const DTree& DocumentThread::dtree()
//	{
//	return doc;
//	}

template<typename charT>
struct ci_equal {
		bool operator()(charT ch1, charT ch2) {
		return std::toupper(ch1) == std::toupper(ch2);
		}
};

template<typename T>
int ci_find_substr( const T& str1, const T& str2, int start_pos )
	{
	auto start=str1.begin();
	start+=start_pos;
	typename T::const_iterator it = std::search( start, str1.end(),
																str2.begin(), str2.end(), ci_equal<typename T::value_type>() );
	if ( it != str1.end() ) return it - str1.begin();
	else return -1;
	}

std::pair<DTree::iterator, size_t> DocumentThread::find_string(DTree::iterator start_it, size_t start_pos, const std::string& f, bool case_ins) const
	{
	// std::cerr << "finding from pos " << start_pos << ", " << &(*start_it) << ": " << start_it->textbuf.substr(0,30) << std::endl;
	DTree::iterator doc_it=start_it;
	while(doc_it!=doc.end()) {
		//		std::cout << doc_it->textbuf << std::endl;
		// FIXME: re-enable searching in output cells.
		if(doc_it->hidden==false && (doc_it->cell_type==DataCell::CellType::python || doc_it->cell_type==DataCell::CellType::latex)) {
			size_t pos;
			if(case_ins)
				pos = ci_find_substr(doc_it->textbuf, f, start_pos);
			else
				pos = doc_it->textbuf.find(f, start_pos);

			if(pos!=std::string::npos)
				return std::make_pair(doc_it, pos);
			}
		start_pos=0; // after one fail, start next cell at zero
		++doc_it;
		}
	return std::make_pair(doc.end(), std::string::npos);
	}

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

	if(main_thread_id != std::this_thread::get_id())
		std::cerr << "INTERNAL ERROR: DocumentThread::process_action_queue not running on main thread."
					 << std::endl;

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
		try {
			ab->execute(*this, *gui);
		}
		catch (const std::exception& err) {
			on_unhandled_error(err);
		}
		// Lock the queue to remove the action just executed, and
		// add it to the undo stack.
		stack_mutex.lock();
		if(ab->undoable())
			undo_stack.push(ab);
		pending_actions.pop();
		}
	stack_mutex.unlock();
	}

bool DocumentThread::on_unhandled_error(const std::exception& err)
	{
	return true;
	}

DocumentThread::Prefs::Prefs(bool use_defaults)
	{
#ifndef EMSCRIPTEN
	config_path=std::string(Glib::get_user_config_dir()) + "/cadabra2.conf";
	try {

		if (!use_defaults) {
			std::ifstream f(config_path);
			if (f) {
				try {
					f >> data;
					}
				catch(nlohmann::json::exception& ex) {
					std::cerr << "Config file " << config_path << " is not JSON; ignoring." << std::endl;
					data = nlohmann::json::object();
					}
				}
			else {
				data = nlohmann::json::object();

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
		}
	catch(std::exception& ex) {
		data = nlohmann::json::object();
		}

	font_step          = data.value("font_step", 0);
	highlight          = data.value("highlight", false);
	is_registered      = data.value("is_registered", false);
	is_anonymous       = data.value("is_anonymous", false);
	git_path           = data.value("git_path", "");
	python_path        = data.value("python_path", "");
	move_into_new_cell = data.value("move_into_new_cell", false);
	tab_completion     = data.value("tab_completion", true);
	microtex           = data.value("microtex", true);

	// Force microtex when this is an AppImage.
	const char *appdir = getenv("APPDIR");
	if(appdir)
		microtex=true;
	// Force microtex when we are on Windows.
#if(_WIN32)
	microtex = true;
#endif

	if(git_path=="")
		git_path="/usr/bin/git";

	// Get the colours for syntax highlighting.
	if(data.count("colours")==0)
		data["colours"]={ {"python", nlohmann::json::object() }, {"latex", nlohmann::json::object() } };

	const auto& python_colours = data["colours"]["python"];

	colours["python"]["keyword"]   = python_colours.value("keyword", "RoyalBlue");
	colours["python"]["operator"]  = python_colours.value("operator", "SlateGray");
	colours["python"]["brace"]     = python_colours.value("brace", "SlateGray");
	colours["python"]["string"]    = python_colours.value("string", "ForestGreen");
	colours["python"]["comment"]   = python_colours.value("comment", "Silver");
	colours["python"]["object"]    = python_colours.value("object", "DarkGray");
	colours["python"]["number"]    = python_colours.value("number", "Sienna");
	colours["python"]["maths"]     = python_colours.value("maths", "Olive");
	colours["python"]["function"]  = python_colours.value("function", "FireBrick");
	colours["python"]["decorator"] = python_colours.value("decorator", "DarkViolet");
	colours["python"]["class"]     = python_colours.value("class", "MediumOrchid");

	const auto& latex_colours = data["colours"]["latex"];
	colours["latex"]["command"]    = latex_colours.value("command", "rgb(52,101,164)");
	colours["latex"]["parameter"]  = latex_colours.value("brace", "rgb(245,121,0)");
	colours["latex"]["comment"]    = latex_colours.value("comment", "Silver");
	colours["latex"]["maths"]      = latex_colours.value("maths", "Sienna");
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
		data["tab_completion"] = tab_completion;
		data["microtex"] = microtex;
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
