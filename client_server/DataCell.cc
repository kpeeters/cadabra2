
#include "DataCell.hh"
#include <sstream>

using namespace cadabra;

uint64_t DataCell::max_serial_number=0;
std::mutex DataCell::serial_mutex;

DataCell::DataCell(CellType t, const std::string& str, bool texhidden) 
	{
	cell_type = t;
	textbuf = str;
	tex_hidden = texhidden;
	
	std::lock_guard<std::mutex> guard(serial_mutex);
	serial_number = max_serial_number++;
	}

DataCell::DataCell(const DataCell& other)
	{
	cell_type = other.cell_type;
	textbuf = other.textbuf;
	cdbbuf = other.cdbbuf;
	tex_hidden = other.tex_hidden;
	sensitive = other.sensitive;
	running = other.running;
	serial_number = other.serial_number;
	}

std::string cadabra::JSON_serialise(const DTree& doc)
	{
	Json::Value json;
	json["description"]="Cadabra JSON notebook format";
	json["version"]=1.0;
	Json::Value cells;

	DTree::sibling_iterator sib=doc.begin();
	while(sib!=doc.end()) {
		Json::Value thiscell;
		JSON_recurse(doc, sib, thiscell);
		cells.append(thiscell);
		++sib;
		}
	json["cells"]=cells;

	std::ostringstream str;
	str << json;

	return str.str();
	}

void cadabra::JSON_recurse(const DTree& doc, DTree::iterator it, Json::Value& json)
	{
	json["textbuf"]  =it->textbuf;
	json["id"]       =(Json::UInt64)it->id();
	switch(it->cell_type) {
		case DataCell::CellType::input:
			json["cell_type"]="input";
			break;
		case DataCell::CellType::output:
			json["cell_type"]="output";
			break;
		case DataCell::CellType::comment:
			json["cell_type"]="comment";
			break;
		case DataCell::CellType::texcomment:
			json["cell_type"]="texcomment";
			break;
		case DataCell::CellType::tex:
			json["cell_type"]="tex";
			break;
		case DataCell::CellType::error:
			json["cell_type"]="error";
			break;
		case DataCell::CellType::section: {
			assert(1==0);
			// NOT YET FUNCTIONAL
			json["cell_type"]="section";
			Json::Value child;
			child["content"]="test";
			json.append(child);
			break;
			}
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

uint64_t DataCell::id() const
	{
	return serial_number;
	}
