
#include "InstallPrefix.hh"
#include "Config.hh"
#include "DataCell.hh"
#include <sstream>
#include <fstream>
#include <regex>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>

using namespace cadabra;

uint64_t DataCell::max_serial_number=1; // 0 is intended to mean 'not set'
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

std::string cadabra::export_as_HTML(const DTree& doc, bool for_embedding, bool strip_code, std::string title)
	{
	// Load the pre-amble from file.
	std::string pname = cadabra::install_prefix()+"/share/cadabra2/notebook.html";
	std::ifstream preamble(pname);
	if(!preamble)
		throw std::logic_error("Cannot open HTML preamble at "+pname);
	std::stringstream buffer;
	buffer << preamble.rdbuf();
	// std::cerr << "Using preamble at " << pname << std::endl;
	std::string preamble_string = buffer.str();

	std::ostringstream str;
	HTML_recurse(doc, doc.begin(), str, preamble_string, for_embedding, strip_code, title);

	return str.str();
	}

std::string cadabra::latex_to_html(const std::string& str)
	{
	std::regex section(R"(\\section\*\{([^\}]*)\})");
	std::regex discretionary(R"(\\discretionary\{\}\{\}\{\})");
	std::regex subsection(R"(\\subsection\*\{([^\}]*)\})");
	std::regex subsubsection(R"(\\subsubsection\*\{([^\}]*)\})");	
	std::regex verb(R"(\\verb\|([^\|]*)\|)");
	std::regex url(R"(\\url\{([^\}]*)\})");
	std::regex href(R"(\\href\{([^\}]*)\}\{([^\}]*)\})");
	std::regex begin_verbatim(R"(\\begin\{verbatim\})");
	std::regex end_verbatim(R"(\\end\{verbatim\})");
	std::regex begin_dmath(R"(\\begin\{dmath\*\})");
	std::regex end_dmath(R"(\\end\{dmath\*\})");
	std::regex tilde("~");
	std::regex less("<");
	std::regex greater(">");
	std::regex latex(R"(\\LaTeX\{\})");
	std::regex tex(R"(\\TeX\{\})");
	std::regex algorithm(R"(\\algorithm\{([^\}]*)\}\{([^\}]*)\})");
	std::regex property(R"(\\property\{([^\}]*)\}\{([^\}]*)\})");
	std::regex package(R"(\\package\{([^\}]*)\}\{([^\}]*)\})");
	std::regex algo(R"(\\algo\{([^\}]*)\})");
	std::regex prop(R"(\\prop\{([^\}]*)\})");
	std::regex underscore(R"(\\_)");
	std::regex e_aigu(R"(\\'e)");
	std::regex ldots(R"(\\ldots)");
	std::regex dquote(R"(``([^']*)'')");
	std::regex squote(R"(`([^']*)')");
	std::regex linebreak(R"(\\linebreak\[0\])");
	std::regex tableau(R"(\\tableau\{(\{[^\}]*\})*\})");
	std::regex ftableau(R"(\\ftableau\{(\{[^\}]*\}[,]?)*\})");		
	std::string res;

	try {
		res = std::regex_replace(str, begin_dmath, R"(\(\displaystyle)");
		res = std::regex_replace(res, discretionary, " ");
		res = std::regex_replace(res, end_dmath, R"(\))");
		res = std::regex_replace(res, tilde, " ");
		res = std::regex_replace(res, less, "&lt;");
		res = std::regex_replace(res, tilde, "&gt;");
		res = std::regex_replace(res, begin_verbatim, "<pre class='output'>");
		res = std::regex_replace(res, end_verbatim, "</pre>");
		res = std::regex_replace(res, section, "<h1>$1</h1>");
		res = std::regex_replace(res, subsection, "<h2>$1</h2>");
		res = std::regex_replace(res, subsubsection, "<h3>$1</h3>");		
		res = std::regex_replace(res, verb, "<code>$1</code>");
		res = std::regex_replace(res, url, "<a href=\"$1\">$1</a>");
		res = std::regex_replace(res, href, "<a href=\"$1\">$2</a>");
		res = std::regex_replace(res, algorithm, "<h2>$1</h2><div class=\"summary\">$2</div>");
		res = std::regex_replace(res, property, "<h2>$1</h2><div class=\"summary\">$2</div>");
		res = std::regex_replace(res, package, "<h1>$1</h1><div class=\"summary\">$2</div>");
		res = std::regex_replace(res, algo, "<a href=\"/manual/$1.html\"><code>$1</code></a>");
		res = std::regex_replace(res, prop, "<a href=\"/manual/$1.html\"><code>$1</code></a>");
		res = std::regex_replace(res, underscore, "_");
		res = std::regex_replace(res, latex, "LaTeX");
		res = std::regex_replace(res, tex, "TeX");
		res = std::regex_replace(res, e_aigu, "é");
		res = std::regex_replace(res, ldots, "...");
		res = std::regex_replace(res, dquote, "\"$1\"");
		res = std::regex_replace(res, squote,  "'$1'");
		res = std::regex_replace(res, linebreak, "\\mmlToken{mo}[linebreak=\"goodbreak\"]{}");
		res = std::regex_replace(res, tableau, "\\)<div class=\"young_box\"></div>\\(\\displaystyle");
		res = std::regex_replace(res, ftableau, "\\)<div class=\"young_box filled\"></div>\\(\\displaystyle");
		res = std::regex_replace(res, std::regex(R"(\{\\tt ([^\}]*)\})"), "<tt>$1</tt>");
		}
	catch(std::regex_error& ex) {
		std::cerr << "regex error on " << str << std::endl;
		}

	return res;
	}

void cadabra::HTML_recurse(const DTree& doc, DTree::iterator it, std::ostringstream& str,
									const std::string& preamble_string,
									bool for_embedding, bool strip_code,
									std::string title)
	{
	bool strip_this=false;
	switch(it->cell_type) {
		case DataCell::CellType::document:
			if(!for_embedding) {
				str << preamble_string << "\n<body>\n";
				}
			else {
				str << "{% extends \"notebook_layout.html\" %}\n"
					 << "{% block head %}\n"
					 << " <meta name=\"keywords\" content=\"cadabra, manual\"/>\n"
					 << "{%- endblock %}\n"
					 << "{% block main %}\n"
					 << "{% raw %}\n";
				}
			break;
		case DataCell::CellType::python:
			if(strip_code && (it->textbuf.substr(0,4)=="def " || it->textbuf.substr(0,5)=="from "))
				strip_this=true;
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
			str << "<div class='latex_view hyphenate'>";
			break;
		case DataCell::CellType::error:
			str << "<div class='error'>";
			break;
		case DataCell::CellType::image_png:
			str << "<div class='image_png'><img src='data:image/png;base64,";
			break;
		case DataCell::CellType::input_form:
			str << "<div class='input_form'>";
			break;
		}	

	if(!strip_this) {
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
						str << "<div class=\"source donthyphenate\">"+out+"</div>";
					}
				}
			}
		catch(std::regex_error& ex) {
			std::cerr << "regex error doing latex_to_html on " << it->textbuf << std::endl;
			throw;
			}
		}
	
	if(doc.number_of_children(it)>0) {
		DTree::sibling_iterator sib=doc.begin(it);
		while(sib!=doc.end(it)) {
			HTML_recurse(doc, sib, str, preamble_string, false, strip_code);
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
					 << "{% block title %}" << title << "{% endblock %}\n";
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
		case DataCell::CellType::input_form:
			str << "</div>\n";
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
		case DataCell::CellType::input_form:
			json["cell_type"]="input_form";
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
			else if(celltype.asString()=="input_form") {
				DataCell dc(id, cadabra::DataCell::CellType::input_form, textbuf.asString(), hide);
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
	// Load the pre-amble from file.
	std::string pname = cadabra::install_prefix()+"/share/cadabra2/notebook.tex";
	std::ifstream preamble(pname);
	if(!preamble)
		throw std::logic_error("Cannot open LaTeX preamble at "+pname);
	std::stringstream buffer;
	buffer << preamble.rdbuf();
	// std::cerr << "Using preamble at " << pname << std::endl;
	std::string preamble_string = buffer.str();

	// Open the LaTeX file for writing.
	std::ostringstream str;
	LaTeX_recurse(doc, doc.begin(), str, preamble_string);

	return str.str();
	}

void cadabra::LaTeX_recurse(const DTree& doc, DTree::iterator it, std::ostringstream& str, const std::string& preamble_string)
	{
	switch(it->cell_type) {
		case DataCell::CellType::document:
			str << preamble_string;
//			str << "\\documentclass[10pt]{article}\n"
//				 << "\\usepackage[scale=.8]{geometry}\n"
//				 << "\\usepackage[fleqn]{amsmath}\n"
//				 << "\\usepackage{listings}\n"
//				 << "\\usepackage{amssymb}\n"
//				 << "\\usepackage{inconsolata}\n"
//				 << "\\usepackage{color}\n"
//				 << "\\usepackage{tableaux}\n"
//				 << "\\newcommand{\\algorithm}[2]{{\\tt\\Large\\detokenize{#1}}\\\\[1ex]\n{\\emph{#2}}\\\\[-1ex]\n}"
//				 << "\\newcommand{\\property}[2]{{\\tt\\Large\\detokenize{#1}}\\\\[1ex]\n{\\emph{#2}}\\\\[-1ex]\n}"
//				 << "\\newcommand{\\algo}[1]{{\\tt #1}}\n"
//				 << "\\newcommand{\\prop}[1]{{\\tt #1}}\n"
//				 << "\\setlength{\\mathindent}{1em}\n"
//				 << "\\lstnewenvironment{python}[1][]\n"
//				 << "{\\lstset{language=Python, columns=fullflexible, xleftmargin=1em, basicstyle=\\small\\ttfamily\\color{blue}, keywordstyle={}}}{}\n"
			str << "\\begin{document}\n";
			break;
		case DataCell::CellType::python:
			str << "\\begin{python}\n";
			break;
		case DataCell::CellType::output:
			str << "\\begin{python}\n";
			break;
		case DataCell::CellType::verbatim:
			str << "\\begin{python}\n";
			break;
		case DataCell::CellType::latex:
			break;
		case DataCell::CellType::latex_view:
			break;
		case DataCell::CellType::error:
			break;
		case DataCell::CellType::input_form:
			break;
		case DataCell::CellType::image_png:
			str << "(image)";
			break;
		}	

	if(it->textbuf.size()>0) {
		if(it->cell_type==DataCell::CellType::image_png)
			str << it->textbuf;
		else if(it->cell_type!=DataCell::CellType::document
		        && it->cell_type!=DataCell::CellType::latex
		        && it->cell_type!=DataCell::CellType::input_form) {
			std::string lr(it->textbuf);
			// Make sure to sync these with the same in TeXEngine.cc !!!
			lr=std::regex_replace(lr, std::regex(R"(\\left\()"),            "\\brwrap{(}{");
			lr=std::regex_replace(lr, std::regex(R"(\\right\))"),           "}{)}");
			lr=std::regex_replace(lr, std::regex(R"(\\left\[)"),            "\\brwrap{[}{");
			lr=std::regex_replace(lr, std::regex(R"(\\right\])"),           "}{]}");
			lr=std::regex_replace(lr, std::regex(R"(\\left\\\{)"),            "\\brwrap{\\{}{");
			lr=std::regex_replace(lr, std::regex(R"(\\right\\\})"),           "}{\\}}");
			lr=std::regex_replace(lr, std::regex(R"(\\right.)"),            "}{.}");
			lr=std::regex_replace(lr, std::regex(R"(\\begin\{dmath\*\})"),  "\\begin{adjustwidth}{1em}{0cm}$");
			lr=std::regex_replace(lr, std::regex(R"(\\end\{dmath\*\})"),    "$\\end{adjustwidth}");
			str << lr << "\n";
			}
		}

	switch(it->cell_type) {
		case DataCell::CellType::python:
		case DataCell::CellType::output:
		case DataCell::CellType::verbatim:
			str << "\\end{python}\n";
			break;
		case DataCell::CellType::document:
		case DataCell::CellType::latex:
		case DataCell::CellType::latex_view:
		case DataCell::CellType::input_form:
		case DataCell::CellType::error:
		case DataCell::CellType::image_png:
			break;
		}	

	if(doc.number_of_children(it)>0) {
		DTree::sibling_iterator sib=doc.begin(it);
		while(sib!=doc.end(it)) {
			LaTeX_recurse(doc, sib, str, preamble_string);
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
		case DataCell::CellType::input_form:
		case DataCell::CellType::error:
		case DataCell::CellType::image_png:
			break;
		}	

	}

std::string cadabra::export_as_python(const DTree& doc)
	{
	std::ostringstream str;
	python_recurse(doc, doc.begin(), str);

	return str.str();
	}

void cadabra::python_recurse(const DTree& doc, DTree::iterator it, std::ostringstream& str)
	{
	if(it->cell_type==DataCell::CellType::document)
		str << "#!/usr/local/bin/cadabra2\n";
	else {
		if(it->cell_type==DataCell::CellType::python) {
			if(it->textbuf.size()>0) {
				str << it->textbuf << "\n";
				}
			}
		}

	if(doc.number_of_children(it)>0) {
		DTree::sibling_iterator sib=doc.begin(it);
		while(sib!=doc.end(it)) {
			python_recurse(doc, sib, str);
			++sib;
			}
		}
	}

// std::string cadabra::replace_all(std::string str, const std::string& old, const std::string& new_s)
//    {
//    if(!old.empty()){
// 	   size_t pos = 0;
// 	   while ((pos = str.find(old, pos)) != std::string::npos) {
// 		   str=str.replace(pos, old.length(), new_s);
// 		   pos += new_s.length();
// 		   }
// 	   }
//    return str;
//    }
