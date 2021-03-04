#include <internal/difflib.h>
#include <internal/string_tools.h>

#include "DiffViewer.hh"
#include "nlohmann/json.hpp"

const Gdk::RGBA color_insert("rgb(200, 255, 200)");
const Gdk::RGBA color_delete("rgb(255, 200, 200)");
const Gdk::RGBA color_offwhite("rgb(240, 240, 240)");
const Gdk::RGBA color_white("rgb(255, 255, 255)");

DiffTextView::DiffTextView()
	{
	auto buffer = get_buffer();
	buffer->create_tag("insert")->property_background_rgba() = color_insert;
	buffer->create_tag("delete")->property_background_rgba() = color_delete;

	set_border_width(5);
	set_editable(false);
	set_hexpand(true);
	set_monospace(true);
	}

CellDiff::CellDiff(const std::string& a, const std::string& b)
	{
	set_border_width(5);
	set_margin_top(5);
	set_margin_bottom(5);
	set_margin_left(5);
	set_margin_right(5);

	grid.set_hexpand(true);
	grid.attach(sw_lhs, 1, 0, 1, 1);
	grid.attach(sw_rhs, 3, 0, 1, 1);
	grid.set_column_homogeneous(true);
	add(grid);

	sw_lhs.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_NEVER);
	sw_rhs.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_NEVER);
	sw_lhs.add(tv_lhs);
	sw_rhs.add(tv_rhs);

	tv_lhs.override_background_color(color_offwhite);
	tv_rhs.override_background_color(color_white);

	compare(a, b);

	show_all_children();
	}

void CellDiff::compare(const std::string& a_, const std::string& b_)
	{
	auto a = string_to_vec(a_);
	auto b = string_to_vec(b_);

	auto buf_lhs = tv_lhs.get_buffer();
	auto buf_rhs = tv_rhs.get_buffer();

	if (a_.empty() && !b_.empty()) {
		// Insert only
		for (size_t i = 0; i < b.size(); ++i) {
			tv_rhs.override_background_color(color_insert);
			buf_rhs->insert(buf_rhs->end(), b[i]);
			}
		}
	else if (b_.empty() && !a_.empty()) {
		// Delete only
		for (size_t i = 0; i < a.size(); ++i) {
			tv_lhs.override_background_color(color_delete);
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

DiffViewer::DiffViewer(std::istream& a, std::istream& b, Gtk::Window& parent)
	: Gtk::Dialog("Comparing notebooks", parent)
	, box(Gtk::ORIENTATION_VERTICAL)
	{
	set_border_width(5);
	set_default_size(1000, 600);

	get_content_area()->add(scrolled_window);

	scrolled_window.add(box);
	scrolled_window.set_vexpand(true);
	scrolled_window.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_ALWAYS);
	add_button("Close", Gtk::RESPONSE_CLOSE);

	populate(a, b);

	show_all();
	}

void DiffViewer::run_noblock()
	{
	signal_response().connect([this](int /* response_id */) {
		hide();
		});
	show();
	present();
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

	for (auto& cell : cells)
		box.add(cell);
	}

DiffViewer::Cells DiffViewer::make_cells(std::istream& stream)
	{
	Cells ret;

	nlohmann::json nb;
	stream >> nb;
	nb = nb["cells"];
	for (auto it = nb.begin(); it != nb.end(); ++it) {
		ret.first.push_back(std::to_string((*it)["cell_id"].get<uint64_t>()));
		ret.second.push_back((*it)["source"].get<std::string>());
		}

	return ret;
	}
