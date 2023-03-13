
#include "ExNode.hh"
#include "Cleanup.hh"
#include "Exceptions.hh"
#include "Algorithm.hh"
#include "pythoncdb/py_kernel.hh"
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>
#include "DisplayTeX.hh"
#include "DisplaySympy.hh"
#include "DisplayTerminal.hh"
#include "pythoncdb/py_ex.hh"
#include <sstream>

//#define DEBUG 1

using namespace cadabra;

ExNode::ExNode(const Kernel& k, std::shared_ptr<Ex> ex_)
	: IndexClassifier(k), ex(ex_), indices_only(false), args_only(false), terms_only(false), factors_only(false),
	  indnxtit(get_kernel_from_scope()->properties), use_sibling_iterator(false), use_index_iterator(false)
	{
	}

ExNode ExNode::copy() const
	{
	ExNode ret(kernel, ex);

	ret.it=it;
	ret.tag=tag;
	ret.indices_only=indices_only;
	ret.args_only=args_only;
	ret.terms_only=terms_only;
	ret.factors_only=factors_only;

	ret.nxtit=nxtit;
	ret.sibnxtit=sibnxtit;
	ret.indnxtit=indnxtit;

	ret.use_sibling_iterator=use_sibling_iterator;
	ret.use_index_iterator=use_index_iterator;

	ret.topit=topit;
	ret.stopit=stopit;

	ret.ind_free=ind_free;
	ret.ind_dummy=ind_dummy;
	ret.ind_pos_dummy=ind_pos_dummy;

	return ret;
	}

std::string ExNode::input_form() const
   {
		// FIXME: this is a duplicate of Ex_as_string; unify the logic.
		std::ostringstream str;

		DisplayTerminal dt(*get_kernel_from_scope(), get_ex(), false);
		dt.output(str);

		return str.str();
   }

ExNode ExNode::getitem_string(std::string tag)
	{
	ExNode ret(kernel, ex);
	ret.tag=tag;
	ret.ex=ex;
	ret.topit=it;
	ret.stopit=it;
	ret.stopit.skip_children();
	++ret.stopit;
	ret.update(true);
	return ret;
	}

ExNode ExNode::getitem_iterator(ExNode it)
	{
	if(it.ex!=ex) {

		std::cerr << "Need to convert iterator" << std::endl;
		}

	ExNode ret=it;
	return ret;
	}

void ExNode::setitem_string(std::string, std::shared_ptr<Ex>)
	{
	//   ExNode ret(ex);
	//   ret.tag=tag;
	//   ret.ex=ex;
	//   ret.topit=it;
	//   ret.stopit=it;
	//   ret.stopit.skip_children();
	//   ++ret.stopit;
	//   ret.update(true);
	//
	std::cerr << "will set iterator range to value" << std::endl;
	////	Ex::iterator it=ex->begin();
	////
	////	size_t num=ex->number_of_children(it);
	////	if(index>=0 && (size_t)index<num)
	////		ex->replace(ex->child(it, index), val.begin());
	////	else
	////		throw ArgumentException("index "+std::to_string(index)+" out of range, must be smaller than "+std::to_string(num));
	//
	//   return ret;
	}

void ExNode::setitem_iterator(ExNode en, std::shared_ptr<Ex> val)
	{
	std::cerr << "Setitem iterator" << std::endl;

	Ex::iterator use;
	if(en.ex!=ex) {
		std::cerr << "Setitem need to convert iterator" << std::endl;
		auto path=en.ex->path_from_iterator(en.it, en.topit);
		use=ex->iterator_from_path(path, topit);
		}
	else use=en.it;

	Ex::iterator top=val->begin();
	if(*top->name=="") {
		// std::cerr << "top is empty" << std::endl;
		top=val->begin(top);
		}
	ex->replace(use, top);
	}

ExNode ExNode::terms()
	{
	ExNode ret(kernel, ex);
	ret.topit=it;
	ret.terms_only=true;
	ret.factors_only=false;
	ret.use_sibling_iterator=true;
	ret.update(true);
	return ret;
	}

ExNode ExNode::factors()
	{
	ExNode ret(kernel, ex);
	ret.topit=it;
	ret.terms_only=false;
	ret.factors_only=true;
	ret.use_sibling_iterator=true;
	ret.update(true);
	return ret;
	}

ExNode ExNode::own_indices()
	{
	ExNode ret(kernel, ex);
	ret.topit=it;
	ret.indices_only=true;
	ret.use_sibling_iterator=true;
	ret.update(true);
	return ret;
	}

ExNode ExNode::indices()
	{
	ExNode ret(kernel, ex);
	ret.topit=it;
	ret.ind_free.clear();
	ret.ind_dummy.clear();
	ret.indices_only=true;
	ret.use_index_iterator=true;
	ret.update(true);
	return ret;
	}

ExNode ExNode::free_indices()
	{
	ExNode ret(kernel, ex);
	ret.topit=it;
	ret.ind_free.clear();
	ret.ind_dummy.clear();
	classify_indices(it, ret.ind_free, ret.ind_dummy);
	fill_index_position_map(it, ret.ind_dummy, ret.ind_pos_dummy);
	ret.indices_only=true;
	ret.use_index_iterator=true;
	ret.update(true);
	return ret;
	}

ExNode ExNode::args()
	{
	ExNode ret(kernel, ex);
	ret.topit=it;
	ret.args_only=true;
	ret.use_sibling_iterator=true;
	ret.update(true);
	return ret;
	}

ExNode ExNode::children()
	{
	ExNode ret(kernel, ex);
	ret.topit=it;
	ret.use_sibling_iterator=true;
	ret.update(true);
	return ret;
	}

void ExNode::replace(std::shared_ptr<Ex> rep)
	{
#ifdef DEBUG
	std::cerr << "next at " << nxtit << std::endl;
#endif
	while(nxtit!=stopit && ex->is_in_subtree(nxtit, it))
		update(false);
#ifdef DEBUG
	std::cerr << nxtit.node << std::endl;
	std::cerr << stopit.node << std::endl;
	if(nxtit==stopit)
		std::cerr << "updated next to be at stopit" << std::endl;
	else
		std::cerr << "updated next at " << nxtit << std::endl;
#endif
	it=ex->replace(it, rep->begin());
	}

ExNode ExNode::insert(std::shared_ptr<Ex> rep)
	{
	ExNode ret(kernel, ex);
	ret.it=ex->insert_subtree(it, rep->begin());
	return ret;
	}

ExNode ExNode::insert_it(ExNode rep)
	{
	ExNode ret(kernel, ex);
	ret.it=ex->insert_subtree(it, rep.it);
	return ret;
	}

ExNode ExNode::append_child(std::shared_ptr<Ex> rep)
	{
	ExNode ret(kernel, ex);
	ret.it=ex->append_child(it, rep->begin());
	return ret;
	}

ExNode ExNode::append_child_it(ExNode rep)
	{
	ExNode ret(kernel, ex);
	ret.it=ex->append_child(it, rep.it);
	return ret;
	}

ExNode ExNode::add_ex(std::shared_ptr<cadabra::Ex> other)
	{
	// std::cerr << it << std::endl;
	// std::cerr << "- - - " << std::endl;
	// std::cerr << ex->begin() << std::endl;
	if(ex->is_head(it) || *(ex->parent(it)->name)!="\\sum")
		ex->wrap(it, str_node("\\sum"));
	auto sumnode=ex->parent(it);
	ExNode ret(kernel, ex);
	// std::cerr << ex->begin() << std::endl;
	ret.it=ex->insert_subtree_after(it, other->begin());
	cleanup_dispatch(*get_kernel_from_scope(), *ex, sumnode);
	// std::cerr << "----" << std::endl;
	// std::cerr << ex->begin() << std::endl;
	// std::cerr << "====" << std::endl;
	return *this;
	}

void ExNode::erase()
	{
	ex->erase(it);
	}

std::string ExNode::get_name() const
	{
	if(!ex->is_valid(it))
		throw ConsistencyException("Cannot print the value of an iterator before the first 'next'.");
	return *it->name;
	}

void ExNode::set_name(std::string nm)
	{
	if(!ex->is_valid(it))
		throw ConsistencyException("Cannot set the value of an iterator before the first 'next'.");
	it->name = name_set.insert(nm).first;
	}

cadabra::Ex ExNode::get_ex() const
	{
	return Ex(it);
	}

str_node::parent_rel_t ExNode::get_parent_rel() const
	{
	if(!ex->is_valid(it))
		throw ConsistencyException("Cannot get the value of an iterator before the first 'next'.");
	return it->fl.parent_rel;
	}

void ExNode::set_parent_rel(str_node::parent_rel_t pr)
	{
	if(!ex->is_valid(it))
		throw ConsistencyException("Cannot set the value of an iterator before the first 'next'.");
	it->fl.parent_rel=pr;
	}

pybind11::object ExNode::get_multiplier() const
	{
	if(!ex->is_valid(it))
		throw ConsistencyException("Cannot get the multiplier of an iterator before the first 'next'.");

	pybind11::object mpq = pybind11::module::import("gmpy2").attr("mpq");
	auto m = *it->multiplier;
	pybind11::object mult = mpq(m.get_num().get_si(), m.get_den().get_si());
	return mult;
	}

void ExNode::set_multiplier(pybind11::object mult)
	{
	if(!ex->is_valid(it))
		throw ConsistencyException("Cannot set the multiplier of an iterator before the first 'next'.");

	pybind11::object mpq = pybind11::module::import("gmpy2").attr("mpq");
	multiply(it->multiplier, pybind11::cast<int>(mult));
	}


ExNode& ExNode::iter()
	{
	return *this;
	}

void ExNode::update(bool first)
	{
	if(use_sibling_iterator) {
		if(terms_only) {
			if(*topit->name=="\\sum") {
				if(first) sibnxtit=ex->begin(topit);
				else      ++sibnxtit;
				}
			else {
				if(Algorithm::is_termlike(topit)) {
					if(Ex::is_head(topit)==false && *Ex::parent(topit)->name=="\\sum")
						sibnxtit=ex->end(topit);
					else {
						if(first) sibnxtit=topit;
						else      sibnxtit=ex->end(topit);
						}
					}
				else sibnxtit=ex->end(topit);
				}
			}
		else if(factors_only) {
			if(*topit->name=="\\prod") {
				if(first) sibnxtit=ex->begin(topit);
				else      ++sibnxtit;
				}
			else {
				if(first) sibnxtit=topit;
				else      sibnxtit=ex->end(topit);
				}
			}
		else {
			if(first) sibnxtit=ex->begin(topit);
			else      ++sibnxtit;
			}

		if(!indices_only && !args_only) return; // any sibling is ok.

		while(sibnxtit!=ex->end(topit)) {
			if(indices_only)
				if(sibnxtit->fl.parent_rel==str_node::p_sub || sibnxtit->fl.parent_rel==str_node::p_super)
					return;
			if(args_only)
				if(sibnxtit->fl.parent_rel==str_node::p_none)
					return;
			++sibnxtit;
			}
		}
	else if(use_index_iterator) {
		if(first) indnxtit=cadabra::index_iterator::begin(get_kernel_from_scope()->properties, topit);
		else      ++indnxtit;
		// Test if this is a dummy index.
		Ex::iterator tofind=indnxtit;
		while(ind_pos_dummy.find(tofind)!=ind_pos_dummy.end()) {
#ifdef DEBUG
			std::cerr << "considered " << tofind << " but that's a dummy" << std::endl;
#endif
			++indnxtit;
			tofind=indnxtit;
			if(indnxtit==cadabra::index_iterator::end(get_kernel_from_scope()->properties, topit))
				break;
			}
		}
	else {
#ifdef DEBUG
		std::cerr << "update normal iterator" << std::endl;
#endif
		if(first) nxtit=topit;
		else      ++nxtit;

#ifdef DEBUG
		if(nxtit==stopit)
			std::cerr << "update reached end" << std::endl;
#endif
		while(nxtit!=stopit) {
#ifdef DEBUG
			std::cerr << "update at " << nxtit << std::endl;
#endif
			if(tag=="" || *nxtit->name==tag)
				return;
			++nxtit;
			}
		}
	}

ExNode& ExNode::next()
	{
	if(use_sibling_iterator) {
		if(sibnxtit==ex->end(topit))
			throw pybind11::stop_iteration();
		it=sibnxtit;
		}
	else if(use_index_iterator) {
		if(indnxtit==cadabra::index_iterator::end(get_kernel_from_scope()->properties, topit))
			throw pybind11::stop_iteration();
		it=indnxtit;
		}
	else {
		if(nxtit==stopit)
			throw pybind11::stop_iteration();
		it=nxtit;
		}

	update(false);
	return *this;
	}

std::string ExNode::__str__() const
	{
	if(!ex->is_valid(it))
		throw ConsistencyException("Cannot print iterator before the first 'next'.");
	
	std::ostringstream str;
	//
	// //	if(state()==Algorithm::result_t::l_no_action)
	// //		str << "(unchanged)" << std::endl;
	// 	DisplayTeX dt(get_kernel_from_scope()->properties, ex);

	DisplayTerminal dt(*get_kernel_from_scope(), it, true);
	dt.output(str, it);

	return str.str();
	}

std::string ExNode::_latex_() const
	{
	if(!ex->is_valid(it))
		throw ConsistencyException("Cannot print iterator LaTeX form before the first 'next'.");

	std::ostringstream str;
	DisplayTeX dt(*get_kernel_from_scope(), it);
	dt.output(str, it);
	return str.str();
	}


ExNode Ex_iter(std::shared_ptr<Ex> ex)
	{
	ExNode ret(*get_kernel_from_scope(), ex);
	ret.ex=ex;
	ret.topit=ex->begin();
	ret.stopit=ex->end();
	ret.update(true);
	return ret;
	}

ExNode Ex_top(std::shared_ptr<Ex> ex)
	{
	ExNode ret(*get_kernel_from_scope(), ex);
	ret.ex=ex;
	ret.topit=ex->begin();
	ret.stopit=ex->end();
	ret.it=ret.topit;
	return ret;
	}

bool Ex_matches(std::shared_ptr<Ex> ex, ExNode& other)
	{
	Ex_comparator comp(get_kernel_from_scope()->properties);
	auto ret=comp.equal_subtree(ex->begin(), other.it);
	if(ret==Ex_comparator::match_t::no_match_less || ret==Ex_comparator::match_t::no_match_greater) return false;
	return true;
	}

bool Ex_matches_Ex(std::shared_ptr<Ex> ex, std::shared_ptr<Ex> other)
	{
	Ex_comparator comp(get_kernel_from_scope()->properties);
	auto ret=comp.equal_subtree(ex->begin(), other->begin());
	if(ret==Ex_comparator::match_t::no_match_less || ret==Ex_comparator::match_t::no_match_greater) return false;
	return true;
	}

bool ExNode_less(ExNode& one, ExNode& two)
	{
	Ex_comparator comp(get_kernel_from_scope()->properties);
	auto ret=comp.equal_subtree(one.it, two.it);
	if(ret==Ex_comparator::match_t::no_match_less) return true;
	return false;
	}

bool ExNode_greater(ExNode& one, ExNode& two)
	{
	Ex_comparator comp(get_kernel_from_scope()->properties);
	auto ret=comp.equal_subtree(one.it, two.it);
	if(ret==Ex_comparator::match_t::no_match_greater) return true;
	return false;
	}

ExNode Ex_getitem_string(std::shared_ptr<Ex> ex, std::string tag)
	{
	ExNode ret(*get_kernel_from_scope(), ex);
	ret.tag=tag;
	ret.ex=ex;
	ret.topit=ex->begin();
	ret.stopit=ex->end();
	ret.update(true);
	return ret;
	}

ExNode Ex_getitem_iterator(std::shared_ptr<Ex> ex, ExNode en)
	{
	Ex::iterator use;
	if(en.ex!=ex) {
		//		std::cerr << "Need to convert iterator " << en.it << std::endl;
		//		std::cerr << "for " << en.topit << std::endl;
		//		std::cerr << "to one for " << ex->begin() << std::endl;
		auto path=en.ex->path_from_iterator(en.it, en.topit);
		use=ex->iterator_from_path(path, ex->begin());
		}
	else use=en.it;
	//	use=ex->begin();  This does not help

	ExNode ret(*get_kernel_from_scope(), ex);
	ret.ex=ex;
	ret.topit=use;
	ret.it=use;
	use.skip_children();
	++use;
	ret.stopit=use;
	ret.update(true);
	//	std::cerr << "Set to go" << std::endl;
	//	std::cerr << ret.topit << std::endl;
	return ret;
	}

