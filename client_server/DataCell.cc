
#include "DataCell.hh"
#include <sstream>
#include <regex>

using namespace cadabra;

uint64_t DataCell::max_serial_number=0;
std::mutex DataCell::serial_mutex;

bool DataCell::id_t::operator<(const DataCell::id_t& other) const
	{
	if(created_by_client != other.created_by_client) return created_by_client;

	return (id < other.id);
	}

DataCell::DataCell(CellType t, const std::string& str, bool cell_hidden) 
	{
	cell_type = t;
	textbuf = str;
	hidden = cell_hidden;
	running=false;
	
	std::lock_guard<std::mutex> guard(serial_mutex);
	serial_number.id = max_serial_number++;
	serial_number.created_by_client = true;
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
	std::regex section(R"(\\section\*\{(.*)\})");
	std::regex subsection(R"(\\subsection\*\{(.*)\})");
	std::regex verb(R"(\\verb\|(.*)\|)");
	std::regex url(R"(\\url\{(.*)\})");
	std::regex href(R"(\\href\{([^\}]*)\}\{([^\}]*)\})");
	std::regex begin_verbatim(R"(\\begin\{verbatim\})");
	std::regex end_verbatim(R"(\\end\{verbatim\})");
	std::regex begin_dmath(R"(\\begin\{dmath\*\})");
	std::regex end_dmath(R"(\\end\{dmath\*\})");
	std::regex tilde("~");

	std::string res;

	res = std::regex_replace(str, section, "<h1>$1</h1>");
	res = std::regex_replace(res, subsection, "<h2>$1</h2>");
	res = std::regex_replace(res, verb, "<code>$1</code>");
	res = std::regex_replace(res, url, "<a href=\"$1\">$1</a>");
	res = std::regex_replace(res, href, "<a href=\"$1\">$2</a>");
	res = std::regex_replace(res, begin_verbatim, "<pre class='output'>");
	res = std::regex_replace(res, end_verbatim, "</pre>");
	res = std::regex_replace(res, begin_dmath, "\\[");
	res = std::regex_replace(res, end_dmath, "\\]");
	res = std::regex_replace(res, tilde, " ");

	return res;
	}

void cadabra::HTML_recurse(const DTree& doc, DTree::iterator it, std::ostringstream& str, bool for_embedding)
	{
	switch(it->cell_type) {
		case DataCell::CellType::document:
			if(!for_embedding) {
				str << "<html>\n";
				str << "<head>\n";
				str << "<link rel=\"stylesheet\" href=\"fonts/Serif/cmun-serif.css\"></link>\n";
				str << "<style>\n";
				str << "div.latex div.output { display: none; }\n div.image_png { width: 400px; }\n"
					 << "div.output { font-family: monospace; }\n"
					 << "h1, h2, h3 { font-family: 'STIXGENERAL'; }\n"
					 << "div.latex { font-family: 'STIXGENERAL'; color: black; font-size: 16px; line-height: 23px; margin-left: 40px; margin-right: 40px; padding-left: 10px; margin-bottom: 10px; }\n"
					 << "div.image_png img { width: 100%; }\n"
					 << "div.python { font-family: monospace; padding-left: 10px; margin-left: 40px; margin-right; 40px; margin-bottom: 10px; margin-top: 10px; white-space: pre; color: blue; }\n"
					 << "pre.output { color: black; }\n";
				str << "</style>\n";
				str << "</head>\n";
				str << "<body>\n";
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

	if(it->cell_type!=DataCell::CellType::document) 
		str << latex_to_html(it->textbuf);

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
		json["cell_id"]       =(Json::UInt64)it->id().id;
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
			const Json::Value cell_id     = cells[c]["cell_id"];
			const Json::Value cell_origin = cells[c]["cell_origin"];
			const Json::Value textbuf     = cells[c]["source"];
			const Json::Value hidden      = cells[c]["hidden"];
			
			DTree::iterator last=doc.end();
			DataCell::id_t id;
			id.id=cell_id.asUInt64();
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
