#include <gtkmm.h>
#include <string>
#include <istream>

class CellDiff : public Gtk::Box
{
public:
	enum Side { lhs, rhs };

	CellDiff(const std::string& a, const std::string& b);

protected:
	void initialize_tv(Gtk::TextView& tv, Glib::RefPtr<Gtk::TextBuffer>& buffer, Side side);
	void compare(const std::string& a_str, const std::string& b_str);

	static const Gdk::RGBA color_blank, color_delete, color_insert;
	Gtk::Grid grid;
	Gtk::TextView tv_lhs, tv_rhs;
	Gtk::Frame frame_lhs, frame_rhs;
	Glib::RefPtr<Gtk::TextBuffer> buf_lhs, buf_rhs;
	Glib::RefPtr<Gtk::CssProvider> css;
	Pango::FontDescription fdesc;
};

class DiffViewer : public Gtk::Dialog
{
public:
	DiffViewer(std::istream& a, std::istream& b);

protected:	
	using Cells = std::pair<std::vector<std::string>, std::vector<std::string>>;

	void populate(std::istream& a, std::istream& b);
	Cells make_cells(std::istream& stream);

	Gtk::Box box;
	Gtk::ScrolledWindow  scrolled_window;
	std::vector<CellDiff> cells;
};