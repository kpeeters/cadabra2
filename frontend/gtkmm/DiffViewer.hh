#pragma once

#include <gtkmm.h>
#include <gtkmm/frame.h>
#include <string>
#include <istream>

class DiffTextView : public Gtk::TextView {
	public:
		DiffTextView();
	};

class CellDiff : public Gtk::Frame {
	public:
		CellDiff(const std::string& a, const std::string& b);

	protected:
		void compare(const std::string& a_str, const std::string& b_str);
		Gtk::Grid grid;
		DiffTextView tv_lhs, tv_rhs;
		Gtk::ScrolledWindow sw_lhs, sw_rhs;
	};

class DiffViewer : public Gtk::Dialog {
	public:
		DiffViewer(std::istream& a, std::istream& b, Gtk::Window& parent);
		void run_noblock();
	protected:
		using Cells = std::pair<std::vector<std::string>, std::vector<std::string>>;

		void populate(std::istream& a, std::istream& b);
		Cells make_cells(std::istream& stream);

		Gtk::Box box;
		Gtk::ScrolledWindow  scrolled_window;
		std::vector<CellDiff> cells;
	};
