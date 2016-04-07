
#include "DataCell.hh"
#include <sstream>
#include <boost/regex.hpp>
//#include <regex>
#include <iostream>

using namespace cadabra;

uint64_t DataCell::max_serial_number=0;
std::mutex DataCell::serial_mutex;

bool DataCell::id_t::operator<(const DataCell::id_t& other) const
	{
	if(created_by_client != other.created_by_client) return created_by_client;

	return (id < other.id);
	}

DataCell::id_t::id_t()
	{
	std::lock_guard<std::mutex> guard(serial_mutex);
	id=max_serial_number++;
	created_by_client=true;
	}

DataCell::DataCell(CellType t, const std::string& str, bool cell_hidden) 
	{
	cell_type = t;
	textbuf = str;
	hidden = cell_hidden;
	running=false;
	}

DataCell::DataCell(id_t id_, CellType t, const std::string& str, bool cell_hidden) 
	{
	cell_type = t;
	textbuf = str;
	hidden = cell_hidden;
	running=false;
	serial_number=id_;
	}

DataCell::DataCell(const DataCell& other)
	{
	running = other.running;
	cell_type = other.cell_type;
	textbuf = other.textbuf;
	hidden = other.hidden;
	sensitive = other.sensitive;
	serial_number = other.serial_number;
	}

std::string cadabra::export_as_HTML(const DTree& doc, bool for_embedding)
	{
	std::ostringstream str;
	HTML_recurse(doc, doc.begin(), str, for_embedding);

	return str.str();
	}

std::string cadabra::latex_to_html(const std::string& str)
	{
	boost::regex section(R"(\\section\*\{([^\}]*)\})");
	boost::regex subsection(R"(\\subsection\*\{([^\}]*)\})");
	boost::regex verb(R"(\\verb\|([^\|]*)\|)");
	boost::regex url(R"(\\url\{([^\}]*)\})");
	boost::regex href(R"(\\href\{([^\}]*)\}\{([^\}]*)\})");
	boost::regex begin_verbatim(R"(\\begin\{verbatim\})");
	boost::regex end_verbatim(R"(\\end\{verbatim\})");
	boost::regex begin_dmath(R"(\\begin\{dmath\*\})");
	boost::regex end_dmath(R"(\\end\{dmath\*\})");
	boost::regex tilde("~");
	boost::regex less("<");
	boost::regex greater(">");
	boost::regex latex(R"(\\LaTeX\{\})");
	boost::regex tex(R"(\\TeX\{\})");
	boost::regex algorithm(R"(\\algorithm\{([^\}]*)\}\{([^\}]*)\})");
	boost::regex property(R"(\\property\{([^\}]*)\}\{([^\}]*)\})");
	boost::regex algo(R"(\\algo\{([^\}]*)\})");
	boost::regex prop(R"(\\prop\{([^\}]*)\})");
	boost::regex underscore(R"(\\_)");
	boost::regex e_aigu(R"(\\'e)");

	std::string res;

	try {
		res = boost::regex_replace(str, begin_dmath, R"(\\[)");
		res = boost::regex_replace(res, end_dmath, R"(\\])");
		res = boost::regex_replace(res, tilde, " ");
		res = boost::regex_replace(res, less, "&lt;");
		res = boost::regex_replace(res, tilde, "&gt;");
		res = boost::regex_replace(res, begin_verbatim, "<pre class='output'>");
		res = boost::regex_replace(res, end_verbatim, "</pre>");
		res = boost::regex_replace(res, section, "<h1>$1</h1>");
		res = boost::regex_replace(res, subsection, "<h2>$1</h2>");
		res = boost::regex_replace(res, verb, "<code>$1</code>");
		res = boost::regex_replace(res, url, "<a href=\"$1\">$1</a>");
		res = boost::regex_replace(res, href, "<a href=\"$1\">$2</a>");
		res = boost::regex_replace(res, algorithm, "<h1>$1</h1><div class=\"summary\">$2</div>");
		res = boost::regex_replace(res, property, "<h1>$1</h1><div class=\"summary\">$2</div>");
		res = boost::regex_replace(res, algo, "<a href=\"$1.html\"><code>$1</code></a>");
		res = boost::regex_replace(res, prop, "<a href=\"$1.html\"><code>$1</code></a>");
		res = boost::regex_replace(res, underscore, "_");
		res = boost::regex_replace(res, latex, "LaTeX");
		res = boost::regex_replace(res, tex, "TeX");
		res = boost::regex_replace(res, e_aigu, "é");
		}
	catch(boost::regex_error& ex) {
		std::cerr << "regex error on " << str << std::endl;
		}

	return res;
	}

void cadabra::HTML_recurse(const DTree& doc, DTree::iterator it, std::ostringstream& str, bool for_embedding)
	{
	switch(it->cell_type) {
		case DataCell::CellType::document:
			if(!for_embedding) {
				// FIXME: this needs to be read from a text file.
				str << "<html>\n";
				str << "<head>\n";
				str << "<link rel=\"stylesheet\" href=\"http://cadabra.science/static/fonts/Serif/cmun-serif.css\"></link>\n";
				str << "<script type=\"text/javascript\" src=\"http://beta.mathjax.org/mathjax/latest/MathJax.js?config=TeX-AMS-MML_HTMLorMML\"></script>\n";
				str << "<style>\n";
				str << "div.image_png { width: 400px; }\n"
					 << "div.output { font-family: monospace; }\n"
					 << "h1, h2, h3 { font-family: 'STIXGENERAL'; }\n"
					 << "div.latex_view { font-family: 'STIXGENERAL'; color: black; font-size: 16px; line-height: 23px; margin-left: 40px; margin-right: 40px; padding-left: 10px; margin-bottom: 10px; }\n"
					 << "div.image_png img { width: 100%; }\n"
					 << "div.python { font-family: monospace; padding-left: 10px; margin-left: 40px; margin-right; 40px; margin-bottom: 10px; margin-top: 10px; white-space: pre; color: blue; }\n"
					 << "pre.output { color: black; }\n";
				str << "</style>\n";
				str << "</head>\n";
				str << "<body>\n";
				}
			else {
				str << "{% extends \"notebook_layout.html\" %}\n"
					 << "{% block head %}{%- endblock %}\n"
					 << "{% block main %}\n"
					 << "{% raw %}\n";
				}
			break;
		case DataCell::CellType::python:
			str << "<div class='python'>";
			break;
		case DataCell::CellType::output:
			str << "<div class='output'>";
			break;
		case DataCell::CellType::verbatim:
			str << "<div class='verbatim'>";
			break;
		case DataCell::CellType::latex:
			str << "<div class='latex'>";
			break;
		case DataCell::CellType::latex_view:
			str << "<div class='latex_view'>";
			break;
		case DataCell::CellType::error:
			str << "<div class='error'>";
			break;
		case DataCell::CellType::image_png:
			str << "<div class='image_png'><img src='data:image/png;base64,";
			break;
		}	

	try {
		if(it->textbuf.size()>0) {
			if(it->cell_type==DataCell::CellType::image_png)
				str << it->textbuf;
			else if(it->cell_type!=DataCell::CellType::document && it->cell_type!=DataCell::CellType::latex) {
				std::string out;
				if(it->cell_type==DataCell::CellType::python)
					out=it->textbuf;
				else
					out=latex_to_html(it->textbuf);
				if(out.size()>0)
					str << "<div class=\"source\">"+out+"</div>";
				}
			}
		}
	catch(boost::regex_error& ex) {
		std::cerr << "regex error doing latex_to_html on " << it->textbuf << std::endl;
		throw;
		}

	if(doc.number_of_children(it)>0) {
		DTree::sibling_iterator sib=doc.begin(it);
		while(sib!=doc.end(it)) {
			HTML_recurse(doc, sib, str);
			++sib;
			}
		}

	switch(it->cell_type) {
		case DataCell::CellType::document:
			if(!for_embedding) {
				str << "</body>\n";
				str << "</html>\n";
				}
			else {
				str << "{% endraw %}\n"
					 << "{%- endblock %}\n"
					 << "{% block title %}Cadabra manual{% endblock %}\n";
				}
			break;
		case DataCell::CellType::python:
			str << "</div>\n";
			break;
		case DataCell::CellType::output:
			str << "</div>\n";
			break;
		case DataCell::CellType::verbatim:
			str << "</div>\n";
			break;
		case DataCell::CellType::latex:
			str << "</div>\n";
			break;
		case DataCell::CellType::latex_view:
			str << "</div>\n";
			break;
		case DataCell::CellType::error:
			str << "</div>\n";
			break;
		case DataCell::CellType::image_png:
			str << "' /></div>\n";
			break;
		}	
	}

std::string cadabra::JSON_serialise(const DTree& doc)
	{
	Json::Value json;
	JSON_recurse(doc, doc.begin(), json);

	std::ostringstream str;
	str << json;

	return str.str();
	}

void cadabra::JSON_recurse(const DTree& doc, DTree::iterator it, Json::Value& json)
	{
	switch(it->cell_type) {
		case DataCell::CellType::document:
			json["description"]="Cadabra JSON notebook format";
			json["version"]=1.0;
			break;
		case DataCell::CellType::python:
			json["cell_type"]="input";
			break;
		case DataCell::CellType::output:
			json["cell_type"]="output";
			break;
		case DataCell::CellType::verbatim:
			json["cell_type"]="verbatim";
			break;
		case DataCell::CellType::latex:
			json["cell_type"]="latex";
			break;
		case DataCell::CellType::latex_view:
			json["cell_type"]="latex_view";
			break;
		case DataCell::CellType::error:
			json["cell_type"]="error";
			break;
		case DataCell::CellType::image_png:
			json["cell_type"]="image_png";
			break;
//		case DataCell::CellType::section: {
//			assert(1==0);
//			// NOT YET FUNCTIONAL
//			json["cell_type"]="section";
//			Json::Value child;
//			child["content"]="test";
//			json.append(child);
//			break;
//			}
		}
	if(it->hidden)
		json["hidden"]=true;

	if(it->cell_type!=DataCell::CellType::document) {
		json["source"]  =it->textbuf;
		if(it->id().created_by_client)
			json["cell_origin"] = "client";
		else
			json["cell_origin"] = "server";
		//json["cell_id"]       =(Json::UInt64)it->id().id;
		}

	if(doc.number_of_children(it)>0) {
		Json::Value cells;
		DTree::sibling_iterator sib=doc.begin(it);
		while(sib!=doc.end(it)) {
			Json::Value thiscell;
			JSON_recurse(doc, sib, thiscell);
			cells.append(thiscell);
			++sib;
			}
		json["cells"]=cells;
		}
	}

void cadabra::JSON_deserialise(const std::string& cj, DTree& doc) 
	{
	Json::Reader reader;
	Json::Value  root;

	bool ret = reader.parse( cj, root );
	if ( !ret ) {
		std::cerr << "cannot parse json file" << std::endl;
		return;
		}

	// Setup main document.
	DataCell top(DataCell::CellType::document);
	DTree::iterator doc_it = doc.set_head(top);

	// Scan through json file.
	const Json::Value cells = root["cells"];
	JSON_in_recurse(doc, doc_it, cells);
	}

void cadabra::JSON_in_recurse(DTree& doc, DTree::iterator loc, const Json::Value& cells)
	{
	try {
		for(unsigned int c=0; c<cells.size(); ++c) {
			const Json::Value celltype    = cells[c]["cell_type"];
			//const Json::Value cell_id     = cells[c]["cell_id"];
			const Json::Value cell_origin = cells[c]["cell_origin"];
			const Json::Value textbuf     = cells[c]["source"];
			const Json::Value hidden      = cells[c]["hidden"];
			
			DTree::iterator last=doc.end();
			DataCell::id_t id;
			//id.id=cell_id.asUInt64();
			if(cell_origin=="server")
				id.created_by_client=false;
			else
				id.created_by_client=true;
			
			bool hide=false;
			if(hidden.asBool()) 
				hide=true;
			
			if(celltype.asString()=="input" || celltype.asString()=="code") {
				std::string res;
				if(textbuf.isArray()) {
					for(auto& el: textbuf) 
						res+=el.asString();
					}
				else {
					res=textbuf.asString();
					}
				DataCell dc(id, cadabra::DataCell::CellType::python, res, hide);
				last=doc.append_child(loc, dc);
				}
			else if(celltype.asString()=="output") {
				DataCell dc(id, cadabra::DataCell::CellType::output, textbuf.asString(), hide);
				last=doc.append_child(loc, dc);
				}
			else if(celltype.asString()=="error") {
				DataCell dc(id, cadabra::DataCell::CellType::error, textbuf.asString(), hide);
				last=doc.append_child(loc, dc);
				}
			else if(celltype.asString()=="verbatim") {
				DataCell dc(id, cadabra::DataCell::CellType::verbatim, textbuf.asString(), hide);
				last=doc.append_child(loc, dc);
				}
			else if(celltype.asString()=="latex_view") {
				std::string res;
				if(textbuf.isArray()) {
					for(auto& el: textbuf) 
						res+=el.asString();
					}
				else {
					res=textbuf.asString();
					}
				DataCell dc(id, cadabra::DataCell::CellType::latex_view, res, false);
				last=doc.append_child(loc, dc);
				}
			else if(celltype.asString()=="latex" || celltype.asString()=="markdown") {
				std::string res;
				if(textbuf.isArray()) {
					for(auto& el: textbuf) 
						res+=el.asString();
					}
				else {
					res=textbuf.asString();
					}
				bool hide_jupyter=hide;
				if(cells[c].isMember("cells")==false) hide_jupyter=true;

				DataCell dc(id, cadabra::DataCell::CellType::latex, res, hide_jupyter);
				last=doc.append_child(loc, dc);
				
				// IPython/Jupyter notebooks only have the input LaTeX cell, not the output cell,
				// which we need. 
				if(cells[c].isMember("cells")==false) {
					DataCell dc(id, cadabra::DataCell::CellType::latex_view, res, hide);
					doc.append_child(last, dc);
					}
				}
			else if(celltype.asString()=="image_png") {
				DataCell dc(id, cadabra::DataCell::CellType::image_png, textbuf.asString(), hide);
				last=doc.append_child(loc, dc);
				}
			else {
				std::cerr << "cadabra-client: found unknown cell type '"+celltype.asString()+"', ignoring" << std::endl;
				continue;
				}
			
			if(last!=doc.end()) {
				if(cells[c].isMember("cells")) {
					const Json::Value subcells = cells[c]["cells"];
					JSON_in_recurse(doc, last, subcells);
					}
				}
			}
		}
	catch(std::exception& ex) {
		std::cerr << "cadabra-client: exception reading notebook: " << ex.what() << std::endl;
		}
	}

DataCell::id_t DataCell::id() const
	{
	return serial_number;
	}

std::string cadabra::export_as_LaTeX(const DTree& doc)
	{
	std::ostringstream str;
	LaTeX_recurse(doc, doc.begin(), str);

	return str.str();
	}

void cadabra::LaTeX_recurse(const DTree& doc, DTree::iterator it, std::ostringstream& str)
	{
	switch(it->cell_type) {
		case DataCell::CellType::document:
			str << "\\documentclass[11pt]{article}\n"
				 << "\\usepackage{amsmath}\n"
				 << "\\usepackage{amssymb}\n"
				 << "\\usepackage{inconsolata}\n"
				 << "\\usepackage{color}\n"
				 << "\\usepackage{tableaux}\n"
				 << "\\usepackage{breqn}\n"
				 << "\\begin{document}\n";
			break;
		case DataCell::CellType::python:
			str << "\\begin{verbatim}\n";
			break;
		case DataCell::CellType::output:
			str << "\\begin{verbatim}\n";
			break;
		case DataCell::CellType::verbatim:
			str << "\\begin{verbatim}\n";
			break;
		case DataCell::CellType::latex:
			break;
		case DataCell::CellType::latex_view:
			break;
		case DataCell::CellType::error:
			break;
		case DataCell::CellType::image_png:
			str << "(image)";
			break;
		}	

	if(it->textbuf.size()>0) {
		if(it->cell_type==DataCell::CellType::image_png)
			str << it->textbuf;
		else if(it->cell_type!=DataCell::CellType::document && it->cell_type!=DataCell::CellType::latex) {
			str << it->textbuf << "\n";
			}
		}

	switch(it->cell_type) {
		case DataCell::CellType::python:
		case DataCell::CellType::output:
		case DataCell::CellType::verbatim:
			str << "\\end{verbatim}\n";
			break;
		case DataCell::CellType::document:
		case DataCell::CellType::latex:
		case DataCell::CellType::latex_view:
		case DataCell::CellType::error:
		case DataCell::CellType::image_png:
			break;
		}	

	if(doc.number_of_children(it)>0) {
		DTree::sibling_iterator sib=doc.begin(it);
		while(sib!=doc.end(it)) {
			LaTeX_recurse(doc, sib, str);
			++sib;
			}
		}

	switch(it->cell_type) {
		case DataCell::CellType::document:
			str << "\\end{document}\n";
			break;
		case DataCell::CellType::python:
		case DataCell::CellType::output:
		case DataCell::CellType::verbatim:
		case DataCell::CellType::latex:
		case DataCell::CellType::latex_view:
		case DataCell::CellType::error:
		case DataCell::CellType::image_png:
			break;
		}	

	}
