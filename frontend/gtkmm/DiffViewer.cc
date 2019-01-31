#include <internal/difflib.h>
#include <internal/string_tools.h>
#include <json/json.h>

#include "DiffViewer.hh"

const Gdk::RGBA color_insert("rgb(200, 255, 200)");
const Gdk::RGBA color_delete("rgb(255, 200, 200)");
const Gdk::RGBA color_blank("rgb(150, 150, 150)");

DiffTextView::DiffTextView()
{
	auto buffer = get_buffer();
	buffer->create_tag("insert")->property_background_rgba() = color_insert;
	buffer->create_tag("delete")->property_background_rgba() = color_delete;
	buffer->create_tag("blank")->property_background_rgba() = color_blank;

	set_margin_left(5);
	set_margin_right(5);
	set_margin_bottom(5);
	set_margin_top(5);

	set_editable(false);
	set_hexpand(true);
	set_monospace(true);
}

bool DiffTextView::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	Glib::RefPtr<Gdk::Window> win = Gtk::TextView::get_window(Gtk::TEXT_WINDOW_TEXT);

//	std::cerr << "on draw for " << get_buffer()->get_text() << std::endl;
	
	bool ret=Gtk::TextView::on_draw(cr);

	int w, h, x, y;
	win->get_geometry(x,y,w,h);

	// paint the background
	cr->set_source_rgba(1.0, 1.0, 1.0, 1.0);
//	cr->rectangle(5,3,8,h-3);
//	cr->fill();

	cr->set_source_rgba(.2, .7, .2, 1.0);
	double line_width=5.0;
	cr->set_line_width(line_width);
	cr->set_antialias(Cairo::ANTIALIAS_NONE);
	int hor=5;
	cr->move_to(5+hor,line_width);
	cr->line_to(5,line_width);
	cr->line_to(5,h-line_width); 
	cr->line_to(5+hor,h-line_width); 
	cr->stroke();

	return ret;
}

CellDiff::CellDiff(const std::string& a, const std::string& b)
{
	grid.set_column_homogeneous(true);
	add(grid);

	set_border_width(5);
	set_margin_top(5);
	set_margin_bottom(5);
	set_margin_left(5);
	set_margin_right(5);
	override_background_color(color_blank);
	grid.set_hexpand(true);
	set_homogeneous(true);

	grid.attach(sw_lhs, 1, 0, 1, 1);
	grid.attach(sw_rhs, 3, 0, 1, 1);

	sw_lhs.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_NEVER);
	sw_rhs.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_NEVER);

	sw_lhs.add(tv_lhs);
	sw_rhs.add(tv_rhs);

	compare(a, b);

	show_all_children();
}

void CellDiff::compare(const std::string& a_, const std::string& b_)
{
	auto a = string_to_vec(a_);
	auto b = string_to_vec(b_);

	if (a_.empty() && !b_.empty()) {
		// Insert only
		for (size_t i = 0; i < b.size(); ++i) {
			buf_rhs->insert(buf_rhs->end(), b[i]);
		}
	}
	else if (b_.empty() && !a_.empty()) {
		// Delete only
		for (size_t i = 0; i < a.size(); ++i) {
			buf_lhs->insert(buf_lhs->end(), a[i]);
		}
	}
	else {
		// Compare both
		using namespace difflib;

		Differ<std::string> d;

		auto deltas = d.get_deltas(a, b);

		for (const auto& delta : deltas) {
			switch (delta.tag) {
			case tag_t::t_delete:
				buf_lhs->insert_with_tag(buf_lhs->end(), delta.a, "delete");
				buf_rhs->insert(buf_rhs->end(), "\n");
				break;
			case tag_t::t_insert:
				buf_lhs->insert(buf_lhs->end(), "\n");
				buf_rhs->insert_with_tag(buf_rhs->end(), delta.b, "insert");
				break;
			case tag_t::t_equal:
				buf_lhs->insert(buf_lhs->end(), delta.a);
				buf_rhs->insert(buf_rhs->end(), delta.a);
				break;
			case tag_t::t_replace:
				for (const auto& opcode : delta.opcodes) {
					switch (opcode.tag) {
					case tag_t::t_delete:
						buf_lhs->insert_with_tag(buf_lhs->end(), delta.a.substr(opcode.i1, opcode.i2 - opcode.i1), "delete");
						break;
					case tag_t::t_insert:
						buf_rhs->insert_with_tag(buf_rhs->end(), delta.b.substr(opcode.j1, opcode.j2 - opcode.j1), "insert");
						break;
					case tag_t::t_replace:
						buf_lhs->insert_with_tag(buf_lhs->end(), delta.a.substr(opcode.i1, opcode.i2 - opcode.i1), "delete");
						buf_rhs->insert_with_tag(buf_rhs->end(), delta.b.substr(opcode.j1, opcode.j2 - opcode.j1), "insert");
						break;
					case tag_t::t_equal:
						buf_lhs->insert(buf_lhs->end(), delta.a.substr(opcode.i1, opcode.i2 - opcode.i1));
						buf_rhs->insert(buf_rhs->end(), delta.a.substr(opcode.i1, opcode.i2 - opcode.i1));
						break;
					default:
						throw std::runtime_error("Unexpected tag encountered in differ");
					}
				}
				break;
			default:
				throw std::runtime_error("Unexpected tag encountered in differ");
			}
		}
	}

}

DiffViewer::DiffViewer(std::istream& a, std::istream& b)
	: box(Gtk::ORIENTATION_VERTICAL)
{
	set_title("Comparing Notebook");
	set_border_width(5);
	set_default_size(1000, 600);

	get_content_area()->add(scrolled_window);

	scrolled_window.add(box);
	scrolled_window.set_vexpand(true);
	scrolled_window.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);

	populate(a, b);

	for (auto& cell : cells)
		box.add(cell);

	show_all();
}

void DiffViewer::populate(std::istream& a, std::istream& b)
{
	using namespace difflib;

	auto lhs_cells = make_cells(a);
	auto rhs_cells = make_cells(b);

	Differ<std::string> d(nullptr, nullptr, 1.);
	for (const auto& delta : d.get_deltas(lhs_cells.first, rhs_cells.first)) {
		if (delta.tag == tag_t::t_insert) {
			auto pos = std::find(rhs_cells.first.begin(), rhs_cells.first.end(), delta.b);
			cells.emplace_back("", rhs_cells.second[std::distance(rhs_cells.first.begin(), pos)]);
		}
		else if (delta.tag == tag_t::t_delete) {
			auto pos = std::find(lhs_cells.first.begin(), lhs_cells.first.end(), delta.a);
			cells.emplace_back(lhs_cells.second[std::distance(lhs_cells.first.begin(), pos)], "");
		}
		else {
			auto pos_a = std::find(lhs_cells.first.begin(), lhs_cells.first.end(), delta.a);
			auto pos_b = std::find(rhs_cells.first.begin(), rhs_cells.first.end(), delta.a);
			cells.emplace_back(lhs_cells.second[std::distance(lhs_cells.first.begin(), pos_a)], rhs_cells.second[std::distance(rhs_cells.first.begin(), pos_b)]);
		}
	}
}

DiffViewer::Cells DiffViewer::make_cells(std::istream& stream)
{
	Cells ret;

	Json::Value nb;
	stream >> nb;
	nb = nb["cells"];
	for (auto it = nb.begin(); it != nb.end(); ++it) {
		ret.first.push_back((*it)["cell_id"].asString());
		ret.second.push_back((*it)["source"].asString());
	}

	return ret;
}
