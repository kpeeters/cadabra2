#include <internal/difflib.h>
#include <internal/string_tools.h>
#include <internal/uuid.h>
#include <iostream>
#include <iomanip>
#include <fstream>

#ifdef _MSC_VER
#include <Windows.h>
#endif

#include "process.hpp"
#include "nlohmann/json.hpp"

// Global colour constants

const char* colour_insert = "\033[32m";
const char* colour_delete = "\033[31m";
const char* colour_insert_bg = "\033[42m";
const char* colour_delete_bg = "\033[41m";
const char* colour_info = "\033[36m";
const char* colour_modified = "\033[33m";
const char* colour_reset = "\033[0m";

void strip_newline(std::string& s)
{
	while (!s.empty() && (s.back() == '\n' || s.back() == '\r'))
		s.pop_back();
}

// Run git commands

std::string git_path;

std::string run_command(std::string command)
{
	using namespace TinyProcessLib;

	std::string sout;
	std::string serr;
	Process proc(command, "",
		[&sout](const char* bytes, size_t n) {
			sout += std::string(bytes, n);
		},
		[&serr](const char* bytes, size_t n) {
			serr += std::string(bytes, n);
		}
		);
	if (proc.get_exit_status())
		throw std::runtime_error(serr);
	return sout;
}

std::string run_git_command(std::string command)
{
	if (git_path.empty()) {
#ifdef _MSC_VER
		LPSTR lpFilePart;
		char filename[MAX_PATH];
		if (!SearchPath(NULL, "git", ".exe", MAX_PATH, filename, &lpFilePart)) {
			std::cerr << "Could not find git executable\n";
			exit(1);
		}
		git_path = std::string("\"") + filename + std::string("\"");
#else
		std::string temp;
		TinyProcessLib::Process proc("/usr/bin/which git", "", [&temp](const char* bytes, size_t n) {
			temp += std::string(bytes, n);
			}
		);
		if (proc.get_exit_status()) {
			std::cerr << "Could not find git executable in path\n";
			exit(1);
		}
		trim(temp);
		strip_newline(temp);
		git_path = temp;
#endif
	}
	return run_command(git_path + " " + command);
}


// Diffing tools

using Cells = std::pair<std::vector<std::string>, std::vector<std::string>>;

std::string diff_substr(const std::string& s, size_t i1, size_t i2, difflib::tag_t tag)
{
	using namespace difflib;
	auto substr = s.substr(i1, i2 - i1);

	if (tag == tag_t::t_delete) {
		replace_all(substr, " ", colour_delete_bg + std::string(" ") + std::string(colour_reset) + colour_delete);
		replace_all(substr, "\t", colour_delete_bg + std::string(" ") + std::string(colour_reset) + colour_delete);
		return colour_delete + substr;
	}
	else if (tag == tag_t::t_insert) {
		replace_all(substr, " ", colour_insert_bg + std::string(" ") + std::string(colour_reset) + colour_insert);
		replace_all(substr, "\t", colour_insert_bg + std::string(" \b\t") + std::string(colour_reset) + colour_insert);
		return colour_insert + substr;
	}
	else {
		return colour_reset + substr;
	}
}

void compare_cell(const std::string& a_, const std::string& b_, const std::string& id)
{
	auto a = string_to_vec(a_);
	auto b = string_to_vec(b_);
	std::for_each(a.begin(), a.end(), strip_newline);
	std::for_each(b.begin(), b.end(), strip_newline);

	if (a_.empty() && !b_.empty()) {
		// Insert only
		std::cout << colour_info << "## Inserted cell " << id << "\n";
		for (const auto& line : b)
			std::cout << colour_insert << "+  " << line << '\n';
		std::cout << colour_reset << '\n';
	}
	else if (b_.empty() && !a_.empty()) {
		// Delete only
		std::cout << colour_info << "## Deleted cell " << id << "\n";
		for (const auto& line : a)
			std::cout << colour_delete << "-  " << line << '\n';
		std::cout << colour_reset << '\n';
	}
	else {
		// Compare both
		using namespace difflib;
		Differ<std::string> d;
		auto deltas = d.get_deltas(a, b);

		bool changed = std::any_of(deltas.begin(), deltas.end(), [](const Delta<std::string>& d) { return d.tag != tag_t::t_equal; });
		if (!changed)
			return;

		std::cout << colour_info << "## Modified cell " << id << '\n';
		size_t next;
		for (size_t i = 0; i < deltas.size(); ++i) {
			auto delta = deltas[i];
			switch (delta.tag) {
			case tag_t::t_delete:
				std::cout << colour_delete << "-  " << delta.a << '\n';
				break;
			case tag_t::t_insert:
				std::cout << colour_insert << "+  " << delta.b << '\n';
				break;
			case tag_t::t_equal:
				// Find next line which isnt equal
				next = deltas.size();
				for (size_t j = i + 1; j < deltas.size(); ++j) {
					if (deltas[j].tag != tag_t::t_equal) {
						next = j;
						break;
					}
				}
				// If there is a gap of more than two lines, replace it with ellipsis
				if (next - i > 3) {
					if (i == 0) {
						std::cout
							<< colour_info << ".  (skipping " << (next - i - 1) << " lines)\n"
							<< colour_reset << "=  " << deltas[next - 1].a << '\n';
					}
					else {
						std::cout
							<< colour_reset << "=  " << deltas[i].a << '\n'
							<< colour_info << ".  (skipping " << (next - i - 2) << " lines)\n"
							<< colour_reset << "=  " << deltas[next - 1].a << '\n';
					}
					i = next - 1;
				}
				else {
					std::cout << colour_reset << "=  " << delta.a << '\n';
				}
				break;
			case tag_t::t_replace:
				std::cout << colour_modified << "*  ";
				for (const auto& opcode : delta.opcodes) {
					switch (opcode.tag) {
					case tag_t::t_delete:
						std::cout << diff_substr(delta.a, opcode.i1, opcode.i2, tag_t::t_delete);
						break;
					case tag_t::t_insert:
						std::cout << diff_substr(delta.b, opcode.j1, opcode.j2, tag_t::t_insert);
						break;
					case tag_t::t_replace:
						std::cout << diff_substr(delta.a, opcode.i1, opcode.i2, tag_t::t_delete);
						std::cout << diff_substr(delta.b, opcode.j1, opcode.j2, tag_t::t_insert);
						break;
					case tag_t::t_equal:
						std::cout << diff_substr(delta.a, opcode.i1, opcode.i2, tag_t::t_equal);
						break;
					default:
						throw std::runtime_error("Unexpected tag encountered in differ");
					}
				}
				std::cout << '\n';
				break;
			default:
				throw std::runtime_error("Unexpected tag encountered in differ");
			}
		}
	}
	std::cout << colour_reset << "\n\n";
}

Cells cnb_to_cells(std::istream& stream)
{
	Cells ret;
	if (stream.eof())
		return ret;
	try {
		nlohmann::json nb;
		stream >> nb;
		nb = nb["cells"];
		for (auto it = nb.begin(); it != nb.end(); ++it) {
			if (!it->contains("cell_id")) {
				std::cerr << "Notebook version is too old for diff tool, please update by resaving the notebook\n";
				exit(1);
			}
			ret.first.push_back(std::to_string((*it)["cell_id"].get<uint64_t>()));
			ret.second.push_back((*it)["source"].get<std::string>());
		}
	}
	catch (nlohmann::json::exception& e) {
		throw std::runtime_error("Not a valid Cadabra notebook.");
	}
	return ret;
}

void cnb_diff(std::istream& a, std::istream& b)
{
	using namespace difflib;

	auto lhs_cells = cnb_to_cells(a);
	auto rhs_cells = cnb_to_cells(b);

	Differ<std::string> d(nullptr, nullptr, 1.);
	for (const auto& delta : d.get_deltas(lhs_cells.first, rhs_cells.first)) {
		if (delta.tag == tag_t::t_insert) {
			auto pos = std::find(rhs_cells.first.begin(), rhs_cells.first.end(), delta.b);
			compare_cell("", rhs_cells.second[std::distance(rhs_cells.first.begin(), pos)], delta.b);
		}
		else if (delta.tag == tag_t::t_delete) {
			auto pos = std::find(lhs_cells.first.begin(), lhs_cells.first.end(), delta.a);
			compare_cell(lhs_cells.second[std::distance(lhs_cells.first.begin(), pos)], "", delta.a);
		}
		else {
			auto pos_a = std::find(lhs_cells.first.begin(), lhs_cells.first.end(), delta.a);
			auto pos_b = std::find(rhs_cells.first.begin(), rhs_cells.first.end(), delta.a);
			compare_cell(lhs_cells.second[std::distance(lhs_cells.first.begin(), pos_a)], rhs_cells.second[std::distance(rhs_cells.first.begin(), pos_b)], delta.a);
		}
	}
}


// View functions
std::vector<std::string> split_to_maxlength(std::string s, int maxlength)
{
	std::vector<std::string> res;
	while ((int)s.size() > maxlength) {
		res.push_back(s.substr(0, maxlength));
		s = "  " + s.substr(maxlength);
	}
	res.push_back(s);
	return res;
}

// Commands 

void help()
{
	std::cout
		<< "cdb-nbtool: Command line tools for working with Cadabra notebooks. It is recommended to\n"
		<< "pass the output of these tools to a program like `less` or `more` for better viewing.\n"
		<< "  -- cdb-nbtool view <file>\n"
		<< "       View a notebook in the console\n"
		<< "  -- cdb-nbtool diff <file1> <file2>\n"
		<< "       Compute differences between two Cadabra notebooks in a CLI friendly way\n"
		<< "  -- cdb-nbtool gitdiff\n"
		<< "       Specialised version of the diff tool to integrate with git. To setup git to\n"
		<< "       use this tool with .cnb files, edit your .gitconfig file by adding the two lines:\n"
		<< "         |  [diff \"cadabranotebook\"]\n"
		<< "         |      command = cdb-nbtool gitdiff\n"
		<< "       and add the following line to your .gitattributes file:\n"
		<< "         |  *.cnb diff=cadabranotebook\n"
		<< "  -- cdb-nbtool merge <file1> <file2>\n"
		<< "       Interactively merge changes from <file1> into <file2>\n"
		<< "  -- cdb-nbtool gitmerge\n"
		<< "       Specialised version of the merge tool to integrate with git. To setup git to\n"
		<< "       use this tool with .cnb files, edit your .gitconfig file by adding the two lines:\n"
		<< "         |  [merge \"cadabranotebook\"]\n"
		<< "         |    command = cdb-nbtool gitmerge\n"
		<< "       and add the following line to your .gitattributes file:\n"
		<< "         |  *.cnb merge=cadabranotebook\n"
		<< "  -- cdb-nbtool clean <file>\n"
		<< "       Ensures that all input cells are visible and deletes all output cells. This can fix\n"
		<< "       problems with corrupt notebooks. The original notebook is saved as <file>~\n";
}

void view(const char* fname)
{
	std::ifstream f(fname);
	if (!f.is_open()) {
		std::cerr << "Could not open notebook " << fname << '\n';
		exit(1);
	}
	nlohmann::json nb;
	f >> nb;
	nb = nb["cells"];

	// Get terminal width for drawing cell boxes
	int width;
	try {
		width = stoi(run_command("tput cols")) - 1;
	}
	catch (std::runtime_error& e) {
		width = 100;
	}

	// Loop through all cells
	int cell_num = 1;
	for (auto it = nb.begin(); it != nb.end(); ++it) {
		// Top banner with logical cell number
		std::string cell_num_str = std::to_string(cell_num);
		++cell_num;
		std::cout << "┌─Cell " << cell_num_str << " (";
		std::string cell_type = (*it)["cell_type"].get<std::string>();
		int cell_type_len;
		if (cell_type == "latex") {
			std::cout << "LaTeX";
			cell_type_len = 5;
		}
		else if (cell_type == "input") {
			std::cout << "Python";
			cell_type_len = 6;
		}
		else {
			std::cout << cell_type;
			cell_type_len = cell_type.size();
		}
		std::cout << ")";
		for (int i = 0; i < width - (int)cell_num_str.size() - cell_type_len - 11; ++i)
			std::cout << "─";
		std::cout << "┐\n";

		// Cell source
		std::string source = (*it)["source"].get<std::string>();
		// Need to temporarily escape things like the \t in \theta because this is a
		// really bad way of doing this
		replace_all(source, "\t", "    ");

		int line_num = 1;
		for (auto line : string_to_vec(source)) {
			strip_newline(line);
			int maxlength = width - 8;
			auto sublines = split_to_maxlength(line, width - 8);
			if (sublines.empty())
				break;
			std::cout
				<< "│" << std::right << std::setw(3) << line_num
				<< "┆ " << std::left << std::setw(maxlength) << sublines[0] << " │\n";
			++line_num;
			for (size_t i = 1; i < sublines.size(); ++i) {
				std::cout << "│   " << "┆ " << std::left << std::setw(maxlength) << sublines[i] << " │\n";
			}
		}

		// Cell outputs
		if (cell_type == "input" && it->contains("cells")) {
			auto cells = (*it)["cells"];
			for (auto ot = cells.begin(); ot != cells.end(); ++ot) {
				// Divider
				std::cout << "├";
				for (int i = 0; i < width - 2; ++i)
					std::cout << "─";
				std::cout << "┤\n";
				std::string output_type = (*ot)["cell_type"];
				// Sanitize output
				std::string output_str;
				if (output_type == "latex_view") {
					output_str = (*ot)["source"];
					replace_all(output_str, "\\\\", "\\");
					replace_all(output_str, "\\begin{dmath*}", "");
					replace_all(output_str, "\\end{dmath*}", "");
					replace_all(output_str, "\\left", "");
					replace_all(output_str, "\\right", "");
				}
				else if (output_type == "verbatim" || output_type == "output") {
					output_str = (*ot)["source"];
					replace_all(output_str, "\\\\", "\\");
					replace_all(output_str, "\\begin{verbatim}", "");
					replace_all(output_str, "\\end{verbatim}", "");
				}
				else {
					output_str = "[output cell of type " + output_type + "]";
				}
				// Print output
				for (auto line : string_to_vec(output_str)) {
					strip_newline(line);
					for (auto subline : split_to_maxlength(line, width - 4)) {
						std::cout << "│ " << std::left << std::setw(width - 4) << subline << " │\n";
					}
				}

			}
		}

		// Finish off bottom of box
		std::cout << "└";
		for (int i = 0; i < width - 2; ++i)
			std::cout << "─";
		std::cout << "┘\n\n";
	}
}

void diff(const char* a, const char* b)
{
	std::ifstream af(a);
	std::ifstream bf(b);
	if (!af.is_open()) {
		std::cerr << "Could not open file " << a << '\n';
		exit(1);
	}
	if (!bf.is_open()) {
		std::cerr << "Could not open file " << b << '\n';
		exit(1);
	}
	std::cout << "--- " << a << "\n+++ " << b << "\n\n";
	cnb_diff(af, bf);
}

void gitdiff(const char* a, const char* b, const char* relpath)
{
	std::ifstream af(a);
	std::ifstream bf(b);

	std::cout << "--- a/" << relpath;
	if (!af.is_open()) {
		std::cout << " [does not exist]";
		af.setstate(std::ios::eofbit);
	}
	std::cout << "\n+++ b/" << relpath;
	if (!bf.is_open()) {
		std::cout << " [does not exist]";
		bf.setstate(std::ios::eofbit);
	}
	std::cout << "\n\n";
	cnb_diff(af, bf);
}

void clean(const char* a)
{
	std::ifstream ifs(a);
	if (!ifs.is_open()) {
		std::cerr << "Could not open file " << a << "\n";
		exit(1);
	}

	// Copy the file to a~
	{
		std::ofstream ofs(std::string(a) + "~");
		if (!ofs.is_open()) {
			std::cerr << "Could not create backup at " << a << "~\n";
			exit(1);
		}
		ofs << ifs.rdbuf();
	}

	// Return to beginning of file and read as JSON
	ifs.clear();
	ifs.seekg(0);
	nlohmann::json nb;
	ifs >> nb;

	// Clean cells
	auto& cell_list = nb["cells"];
	for (auto it = cell_list.begin(), end = cell_list.end(); it != end; ++it) {
		if (it->contains("cells"))
			it->erase("cells");
		if (it->value("cell_type", "") == "latex") {
			(*it)["hidden"] = false;
			std::map<std::string, nlohmann::json> output_cell;
			output_cell["cell_id"] = cadabra::generate_uuid<uint64_t>();
			output_cell["cell_origin"] = "client";
			output_cell["cell_type"] = "latex_view";
			output_cell["source"] = "~";
			(*it)["cells"] = std::vector<nlohmann::json>(1, output_cell);
		}
	}

	// Write file
	ifs.close();
	std::ofstream ofs(a);
	if (!ofs.is_open()) {
		std::cerr << "Could not open " << a << " for writing\n";
		exit(1);
	}
	ofs << std::setw(4) << nb << '\n';
}

int run(int argc, char** argv)
{
#if _MSC_VER
	SetConsoleCP(CP_UTF8);
#endif

	if (argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
		help();
		return 0;
	}
	else if (strcmp(argv[1], "view") == 0) {
		if (argc != 3) {
			std::cerr << "Wrong number of arguments passed to view, 1 filename expected\n";
			return 1;
		}
		try {
			view(argv[2]);
		}
		catch (nlohmann::json::exception& e) {
			throw std::runtime_error("Not a valid Cadabra notebook.");
		}
	}
	else if (strcmp(argv[1], "diff") == 0) {
		if (argc != 4) {
			std::cerr << "Wrong number of arguments passed to diff, 2 filenames expected\n";
			return 1;
		}
		diff(argv[2], argv[3]);
	}
	else if (strcmp(argv[1], "gitdiff") == 0) {
		if (argc != 9) {
			std::cerr << "Wrong number of arguments passed to gitdiff, if you are trying to diff manually use the 'diff' command\n";
			return 1;
		}
		std::string repo_base = trim(run_git_command("rev-parse --show-toplevel")) + "/";
		std::string filename = repo_base + argv[2];
		gitdiff(argv[3], filename.c_str(), argv[2]);
	}
	else if (strcmp(argv[1], "merge") == 0) {
		std::cerr << "Sorry, not yet implemented!\n";
	}
	else if (strcmp(argv[1], "gitmerge") == 0) {
		std::cerr << "Sorry, not yet implemented!\n";
	}
	else if (strcmp(argv[1], "clean") == 0) {
		if (argc != 3) {
			std::cerr << "Wrong number of arguments passed to clean, expected 1 filename\n";
			exit(1);
		}
		clean(argv[2]);
	}
	else {
		std::cerr << "Unrecognised option " << argv[1] << '\n';
		help();
		return 1;
	}

	return 0;
}

int main(int argc, char** argv)
{
	try {
		return run(argc, argv);
	}
	catch (std::exception& e) {
		std::cerr << "cdb-nbtool failed with error: " << e.what() << '\n';
		return 1;
	}
}
