
#include "DataCell.hh"

using namespace cadabra;

DataCell::DataCell(CellType t, const std::string& str, bool texhidden) 
	{
	cell_type = t;
	textbuf = str;
	tex_hidden = texhidden;
	}

