
#include "ExNode.hh"
#include "PythonCdb.hh"
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>
#include "DisplayTeX.hh"
#include "DisplaySympy.hh"
#include "DisplayTerminal.hh"
#include <sstream>

using namespace cadabra;

ExNode ExNode::getitem_string(std::string tag)
   {
   ExNode ret(ex);
   ret.tag=tag;
   ret.ex=ex;
   ret.topit=it;
   ret.stopit=it;
   ret.stopit.skip_children();
   ++ret.stopit;
   ret.update(true);
   return ret;
   }

ExNode ExNode::terms()
	{
	ExNode ret(ex);
	ret.topit=it;
	ret.terms_only=true;
	ret.factors_only=false;
	ret.use_sibling_iterator=true;
	ret.update(true);
	return ret;
	}

ExNode ExNode::factors()
	{
	ExNode ret(ex);
	ret.topit=it;
	ret.terms_only=false;
	ret.factors_only=true;
	ret.use_sibling_iterator=true;
	ret.update(true);
	return ret;
	}

ExNode ExNode::indices()
	{
	ExNode ret(ex);
	ret.topit=it;
	ret.indices_only=true;
	ret.use_sibling_iterator=true;
	ret.update(true);
	return ret;
	}

ExNode ExNode::args()
	{
	ExNode ret(ex);
	ret.topit=it;
	ret.args_only=true;
	ret.use_sibling_iterator=true;	
	ret.update(true);
	return ret;
	}

ExNode ExNode::children()
   {
	ExNode ret(ex);
	ret.topit=it;
	ret.use_sibling_iterator=true;	
	ret.update(true);
	return ret;
   }

void ExNode::replace(std::shared_ptr<Ex> rep)
	{
	it=ex->replace(it, rep->begin());
	}

ExNode ExNode::insert(std::shared_ptr<Ex> rep)
	{
	ExNode ret(ex);
	ret.it=ex->insert_subtree(it, rep->begin());
	return ret;
	}

ExNode ExNode::insert_it(ExNode rep)
	{
	ExNode ret(ex);
	ret.it=ex->insert_subtree(it, rep.it);
	return ret;
	}

ExNode ExNode::append_child(std::shared_ptr<Ex> rep)
	{
	ExNode ret(ex);
	ret.it=ex->append_child(it, rep->begin());
	return ret;
	}

ExNode ExNode::append_child_it(ExNode rep)
	{
	ExNode ret(ex);
	ret.it=ex->append_child(it, rep.it);
	return ret;
	}

void ExNode::erase()
	{
	ex->erase(it);
	}

std::string ExNode::get_name() const
   {
   return *it->name;
   }

void ExNode::set_name(std::string nm)
   {
   it->name = name_set.insert(nm).first;
   }

str_node::parent_rel_t ExNode::get_parent_rel() const
   {
   return it->fl.parent_rel;
   }

void ExNode::set_parent_rel(str_node::parent_rel_t pr) 
   {
   it->fl.parent_rel=pr;
   }

pybind11::object ExNode::get_multiplier() const
   {
	pybind11::object mpq = pybind11::module::import("gmpy2").attr("mpq");
	auto m = *it->multiplier;
	pybind11::object mult = mpq(m.get_num().get_si(), m.get_den().get_si());
	return mult;
   }

void ExNode::set_multiplier(pybind11::object obj) 
   {
//	pybind11::object mpq = pybind11::module::import("gmpy2").attr("mpq");
//	auto m = *it->multiplier;
//	pybind11::object mult = mpq(m.get_num().get_si(), m.get_den().get_si());
//	return mult;
   }


ExNode::ExNode(std::shared_ptr<Ex> ex_)
   : ex(ex_), indices_only(false), args_only(false), terms_only(false), factors_only(false), use_sibling_iterator(false)
   {
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
			   if(first) sibnxtit=topit;
			   else      sibnxtit=ex->end(topit);
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
	else {
		if(first) nxtit=topit;
		else      ++nxtit;

		while(nxtit!=stopit) {
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
 	std::ostringstream str;
	DisplayTeX dt(*get_kernel_from_scope(), it);
	dt.output(str, it);
	return str.str();
	}


ExNode Ex_iter(std::shared_ptr<Ex> ex)
   {
   ExNode ret(ex);
   ret.ex=ex;
   ret.topit=ex->begin();
   ret.stopit=ex->end();
   ret.update(true);
   return ret;
   }

ExNode Ex_top(std::shared_ptr<Ex> ex)
   {
   ExNode ret(ex);
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

ExNode Ex_getitem_string(std::shared_ptr<Ex> ex, std::string tag)
	{
	ExNode ret(ex);
	ret.tag=tag;
	ret.ex=ex;
	ret.topit=ex->begin();
	ret.stopit=ex->end();
	ret.update(true);
	return ret;
	}

