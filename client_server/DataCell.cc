
#include "DataCell.hh"

using namespace cadabra;

long DataCell::max_serial_number=0;
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
