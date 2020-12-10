
#include "InstallPrefix.hh"
#include "Config.hh"
#include "DataCell.hh"
#include "Exceptions.hh"
#include <sstream>
#include <fstream>
#include <regex>
//#include <boost/filesystem.hpp>
//#include <boost/algorithm/string.hpp>
#include <iostream>
#include "base64.hh"
#include <internal/uuid.h>
#include <iomanip>


using namespace cadabra;

// General tool to strip spaces from both ends
static std::string trim(const std::string& s)
	{
	if(s.length() == 0)
		return s;
	int b = s.find_first_not_of(" \t\n");
	int e = s.find_last_not_of(" \t\n");
	if(b == -1) // No non-spaces
		return "";
	return std::string(s, b, e - b + 1);
	}

bool DataCell::id_t::operator<(const DataCell::id_t& other) const
	{
	if(created_by_client != other.created_by_client) return created_by_client;

	return (id < other.id);
	}

DataCell::id_t::id_t()
	: id(generate_uuid<uint64_t>())
	, created_by_client(true)
	{
	}

DataCell::DataCell(CellType t, const std::string& str, bool cell_hidden)
	{
	cell_type = t;
	textbuf = str;
	hidden = cell_hidden;
	running=false;
	ignore_on_import=false;
	}

DataCell::DataCell(id_t id_, CellType t, const std::string& str, bool cell_hidden)
	{
	cell_type = t;
	textbuf = str;
	hidden = cell_hidden;
	running=false;
	serial_number=id_;
	ignore_on_import=false;	
	}

DataCell::DataCell(const DataCell& other)
	{
	running = other.running;
	cell_type = other.cell_type;
	textbuf = other.textbuf;
	hidden = other.hidden;
	sensitive = other.sensitive;
	serial_number = other.serial_number;
	ignore_on_import = other.ignore_on_import;
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
	std::regex author(R"(\\author\{([^\}]*)\})");
	std::regex email(R"(\\email\{([^\}]*)\})");
	std::regex discretionary(R"(\\discretionary\{\}\{\}\{\})");
	std::regex subsection(R"(\\subsection\*\{([^\}]*)\})");
	std::regex subsubsection(R"(\\subsubsection\*\{([^\}]*)\})");
	std::regex emph(R"(\\emph\{([^\}]*)\})");	
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
	std::regex begin_tabular(R"(\\begin\{tabular\}\{[^\}]*\})");
	std::regex end_tabular(R"(\\end\{tabular\})");
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
		res = std::regex_replace(res, emph, "<i>$1</i>");
		res = std::regex_replace(res, author, "<div class='author'>$1</div>");
		res = std::regex_replace(res, email, "<div class='email'>$1</div>");
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
		res = std::regex_replace(res, begin_tabular, "<table>");
		res = std::regex_replace(res, end_tabular, "</table>");
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
	nlohmann::json json;
	JSON_recurse(doc, doc.begin(), json);

	std::ostringstream str;
	str << std::setfill('\t') << std::setw(1) << json;

	return str.str();
	}

void cadabra::JSON_recurse(const DTree& doc, DTree::iterator it, nlohmann::json& json)
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
			//			nlohmann::json child;
			//			child["content"]="test";
			//			json.append(child);
			//			break;
			//			}
		}
	if(it->hidden)
		json["hidden"]=true;
	if(it->ignore_on_import)
		json["ignore_on_import"]=true;

	json["cell_id"] = it->id().id;

	if(it->cell_type!=DataCell::CellType::document) {
		json["source"]  =it->textbuf;
		if(it->id().created_by_client)
			json["cell_origin"] = "client";
		else
			json["cell_origin"] = "server";
		//json["cell_id"]       =(Json::UInt64)it->id().id;
		}

	if(doc.number_of_children(it)>0) {
		nlohmann::json cells=nlohmann::json::array();
		DTree::sibling_iterator sib=doc.begin(it);
		while(sib!=doc.end(it)) {
			nlohmann::json thiscell;
			JSON_recurse(doc, sib, thiscell);
			cells.push_back(thiscell);
			++sib;
			}
		json["cells"]=cells;
		}
	}

void cadabra::JSON_deserialise(const std::string& cj, DTree& doc)
	{
	nlohmann::json  root;

	try {
		root=nlohmann::json::parse(cj);
		}
	catch(nlohmann::json::exception& e) {
		std::cerr << "cannot parse json file" << std::endl;
		return;
		}

	// Setup main document.
	DataCell::id_t id;
	id.id=root.value("cell_id", generate_uuid<uint64_t>());
	DataCell top(id, DataCell::CellType::document);
	DTree::iterator doc_it = doc.set_head(top);

	// Determine whether this is a Jupyter notebook or a Cadabra
	// notebook.
	if(root.count("description") == 0) {
		root = cadabra::ipynb2cnb(root);
		}
	
	// Scan through json file.
	const nlohmann::json& cells = root["cells"];
	JSON_in_recurse(doc, doc_it, cells);
	}

void cadabra::JSON_in_recurse(DTree& doc, DTree::iterator loc, const nlohmann::json& cells)
	{
	try {
		for(const auto& cell: cells) {
			const nlohmann::json& textbuf = cell["source"];

			DTree::iterator last=doc.end();
			DataCell::id_t id;
			id.id=cell.value("cell_id", generate_uuid<uint64_t>());

			if(cell.value("cell_origin", "")=="server")
				id.created_by_client=false;
			else
				id.created_by_client=true;
			
			bool hide=false;
			if(cell.value("hidden", false))
				hide=true;

			std::string cell_type = cell.value("cell_type", "");
			if(cell_type=="input" || cell_type=="code") {
				std::string res;
				if(textbuf.is_array()) {
					for(const auto& el: textbuf)
						res+=el.get<std::string>();
					}
				else {
					res=textbuf.get<std::string>();
					}
				DataCell dc(id, cadabra::DataCell::CellType::python, res, hide);
				last=doc.append_child(loc, dc);
				}
			else if(cell_type=="output") {
				DataCell dc(id, cadabra::DataCell::CellType::output, textbuf.get<std::string>(), hide);
				last=doc.append_child(loc, dc);
				}
			else if(cell_type=="error") {
				DataCell dc(id, cadabra::DataCell::CellType::error, textbuf.get<std::string>(), hide);
				last=doc.append_child(loc, dc);
				}
			else if(cell_type=="verbatim") {
				DataCell dc(id, cadabra::DataCell::CellType::verbatim, textbuf.get<std::string>(), hide);
				last=doc.append_child(loc, dc);
				}
			else if(cell_type=="input_form") {
				DataCell dc(id, cadabra::DataCell::CellType::input_form, textbuf.get<std::string>(), hide);
				last=doc.append_child(loc, dc);
				}
			else if(cell_type=="latex_view") {
				std::string res;
				if(textbuf.is_array()) {
					for(auto& el: textbuf)
						res+=el.get<std::string>();
					}
				else {
					res=textbuf.get<std::string>();
					}
				DataCell dc(id, cadabra::DataCell::CellType::latex_view, res, false);
				last=doc.append_child(loc, dc);
				}
			else if(cell_type=="latex" || cell_type=="markdown") {
				std::string res;
				if(textbuf.is_array()) {
					for(auto& el: textbuf)
						res+=el.get<std::string>();
					}
				else {
					res=textbuf.get<std::string>();
					}
				bool hide_jupyter=hide;
				if(cell.count("cells")==0) hide_jupyter=true;

				DataCell dc(id, cadabra::DataCell::CellType::latex, res, hide_jupyter);
				last=doc.append_child(loc, dc);

				// IPython/Jupyter notebooks only have the input LaTeX cell, not the output cell,
				// which we need.
				if(cell.count("cells")==0) {
					DataCell dc(id, cadabra::DataCell::CellType::latex_view, res, hide);
					doc.append_child(last, dc);
					}
				}
			else if(cell_type=="image_png") {
				DataCell dc(id, cadabra::DataCell::CellType::image_png, textbuf.get<std::string>(), hide);
				last=doc.append_child(loc, dc);
				}
			else {
				std::cerr << "cadabra-client: found unknown cell type '"+cell_type+"', ignoring" << std::endl;
				continue;
				}

			if(last!=doc.end()) {
				if(cell.value("ignore_on_import", false))
					last->ignore_on_import=true;
				if(cell.count("cells")>0) {
					const nlohmann::json& subcells = cell["cells"];
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

std::string cadabra::export_as_LaTeX(const DTree& doc, const std::string& image_file_base, bool for_embedding)
	{
	// Load the pre-amble from file.
	std::string preamble_string;
	if(!for_embedding) {
		std::string pname = cadabra::install_prefix()+"/share/cadabra2/notebook.tex";
		std::ifstream preamble(pname);
		if(!preamble)
			throw std::logic_error("Cannot open LaTeX preamble at "+pname);
		std::stringstream buffer;
		buffer << preamble.rdbuf();
		// std::cerr << "Using preamble at " << pname << std::endl;
		preamble_string = buffer.str();
		}

	// Open the LaTeX file for writing.
	std::ostringstream str;
	int image_num=0;
	LaTeX_recurse(doc, doc.begin(), str, preamble_string, image_file_base, image_num, for_embedding);

	return str.str();
	}

void cadabra::LaTeX_recurse(const DTree& doc, DTree::iterator it, std::ostringstream& str,
                            const std::string& preamble_string, const std::string& image_file_base,
                            int& image_num, bool for_embedding)
	{
	switch(it->cell_type) {
		case DataCell::CellType::document:
			if(!for_embedding) {
				str << preamble_string;
				str << "\\begin{document}\n";
				}
			break;
		case DataCell::CellType::python:
			str << "\\begin{python}\n";
			break;
		case DataCell::CellType::output:
			str << "\\begin{python}\n";
			break;
		case DataCell::CellType::verbatim:
//			str << "\\begin{verbatim}\n";
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
			std::size_t pos=image_file_base.rfind('/');
			std::string fileonly=image_file_base.substr(pos+1);
			str << "\\begin{center}\n\\includegraphics[width=.6\\textwidth]{"
			    << fileonly+std::to_string(image_num)+"}\n"
			    << "\\end{center}\n";
			break;
		}

	if(it->cell_type==DataCell::CellType::image_png) {
		// Images have to be saved to disk as separate files as
		// LaTeX has no concept of images embedded in the .tex file.
		std::ofstream out(image_file_base+std::to_string(image_num)+".png");
		out << base64_decode(it->textbuf);
		++image_num;
		}
	else {
		if(it->textbuf.size()>0) {
			if(it->cell_type!=DataCell::CellType::document
			      && it->cell_type!=DataCell::CellType::latex
			      && it->cell_type!=DataCell::CellType::input_form) {
				std::string lr(it->textbuf);
				// Make sure to sync these with the same in TeXEngine.cc !!!
				lr=std::regex_replace(lr, std::regex(R"(\\left\()"),            "\\brwrap{(}{");
				lr=std::regex_replace(lr, std::regex(R"(\\right\))"),           "}{)}");
				lr=std::regex_replace(lr, std::regex(R"(\\left\[)"),            "\\brwrap{[}{");
				lr=std::regex_replace(lr, std::regex(R"(\\left\.)"),            "\\brwrap{.}{");
				lr=std::regex_replace(lr, std::regex(R"(\\right\])"),           "}{]}");
				lr=std::regex_replace(lr, std::regex(R"(\\left\\\{)"),            "\\brwrap{\\{}{");
				lr=std::regex_replace(lr, std::regex(R"(\\right\\\})"),           "}{\\}}");
				lr=std::regex_replace(lr, std::regex(R"(\\right\.)"),            "}{.}");
//				lr=std::regex_replace(lr, std::regex(R"(\\begin\{verbatim\})"), "");
//				lr=std::regex_replace(lr, std::regex(R"(\\end\{verbatim\})"),   "");
				lr=std::regex_replace(lr, std::regex(R"(\\begin\{dmath\*\})"),  "\\begin{adjustwidth}{1em}{0cm}$");
				lr=std::regex_replace(lr, std::regex(R"(\\end\{dmath\*\})"),    "$\\end{adjustwidth}");
				lr=std::regex_replace(lr, std::regex(R"(\\algorithm\{(.*)_(.*)\})"), "\\algorithm{$1\\textunderscore{}$2}");
				lr=std::regex_replace(lr, std::regex(R"(\\algorithm\{(.*)_(.*)\})"), "\\algorithm{$1\\textunderscore{}$2}");				
				str << lr << "\n";
				}
			}
		}

	switch(it->cell_type) {
		case DataCell::CellType::python:
		case DataCell::CellType::output:
			str << "\\end{python}\n";
			break;
		case DataCell::CellType::verbatim:
//			str << "\\end{verbatim}\n";
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
			LaTeX_recurse(doc, sib, str, preamble_string, image_file_base, image_num, for_embedding);
			++sib;
			}
		}

	switch(it->cell_type) {
		case DataCell::CellType::document:
			if(!for_embedding) {
				str << "\\end{document}\n";
				}
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

nlohmann::json cadabra::ipynb2cnb(const nlohmann::json& root)
	{
	int nbf = root.value("nbformat", 0);
	nlohmann::json json;

	if(nbf==0)
		throw RuntimeException("Not a Jupyter notebook.");

	json["description"]="Cadabra JSON notebook format";
	json["version"]=1.0;

	nlohmann::json cells=nlohmann::json::array();
	const nlohmann::json& jucells=root["cells"];
	
	// Jupyter notebooks just have a single array of cells; walk
	// through and add to our "cells" array.

	for(unsigned int c=0; c<jucells.size(); ++c) {
		nlohmann::json cell;
		if(jucells[c]["cell_type"].get<std::string>()=="markdown")
			cell["cell_type"]="latex";
		else
			cell["cell_type"]="input";
		cell["hidden"]=false;
		const nlohmann::json& source=jucells[c]["source"];
		// Jupyter stores the source line-by-line in an array 'source'.
		std::string block;
		for(unsigned int l=0; l<source.size(); ++l) {
			std::string line=source[l].get<std::string>();
			if(line.size()>0) {
				if(line[0]=='#') {
					std::string sub="";
					line=line.substr(1);
					int inc=0;
					while(line.size()>0 && line[0]=='#') {
						if(inc<2) {
							sub+="sub";
							inc+=1;
							}
						line=line.substr(1);
						}
					if(line.size()==0)
						continue;
					if(line[line.size()-1]=='\n')
						line=line.substr(0,line.size()-1);
					block+="\\"+sub+"section*{"+trim(line)+"}\n";
					}
				else
					block+=line;
				}
			}
		cell["source"]=block;
		cells.push_back(cell);
		}

	json["cells"] = cells;
	
	return json;
	}

nlohmann::json cadabra::cnb2ipynb(const nlohmann::json& root)
	{
	nlohmann::json ipynb, kernelspec, lang;

	ipynb["nbformat"]=4;
	ipynb["nbformat_minor"]=4;

	kernelspec["display_name"]="Cadabra2";
	kernelspec["language"]="python";
	kernelspec["name"]="cadabra2";

	lang["codemirror_mode"]="cadabra";
	lang["file_extension"]=".ipynb";
	lang["mimetype"]="text/cadabra";
	lang["name"]="cadabra2";
	lang["pygments_lexer"]="cadabra";
	
	ipynb["metadata"] = {
		{"kernelspec", kernelspec},
		{"language_info", lang}
	};

	nlohmann::json cells=nlohmann::json::array();
	
	// Jupyter notebooks just have a single array of cells; walk
	// through and setup our cells array.

	for(const auto& cdb: root["cells"]) {
		nlohmann::json cell;
		if(cdb["cell_type"]=="python") {
			cell["cell_type"]="code";
			cell["source"]=nlohmann::json::array();
			cell["source"].push_back(cdb["source"]);
			cell["metadata"]=nlohmann::json::object();
			cell["outputs"]=nlohmann::json::array();
			cell["execution_count"]={};			
			}
		if(cdb["cell_type"]=="input") {
			cell["cell_type"]="code";
			cell["source"]=nlohmann::json::array();
			cell["source"].push_back(cdb["source"]);
			cell["metadata"]=nlohmann::json::object();
			cell["outputs"]=nlohmann::json::array();
			cell["execution_count"]={};
			}
		else if(cdb["cell_type"]=="latex") {
			cell["cell_type"]="markdown";
			cell["source"]=nlohmann::json::array();
			cell["source"].push_back(cdb["source"]);
			cell["metadata"]=nlohmann::json::object();
			}

		if(cell.is_null()==false)
			cells.push_back(cell);
		
		}

	ipynb["cells"] = cells;
	
	return ipynb;
	}
