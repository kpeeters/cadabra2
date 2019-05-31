#include <sstream>
#include "py_tableau.hh"
#include "../YoungTab.hh"
#include "../properties/TableauBase.hh"
namespace cadabra {

	namespace py = pybind11;
	using tab_t = TableauBase::tab_t;

	std::string tab_str(const tab_t& tab)
	{
		std::stringstream ss;
		ss << "( ";
		for (size_t row = 0; row < tab.number_of_rows(); ++row) {
			ss << "(";
			auto beg = tab.begin_row(row), next = std::next(tab.begin_row(row)), end = tab.end_row(row);
			while (next != end) {
				ss << *beg << " ";
				++beg, ++next;
			}
			ss << *beg << ") ";
		}
		ss << ")";
		return ss.str();
	}

	void init_tableau(pybind11::module& m)
	{
		auto py_tab = py::class_<tab_t>(m, "TableauObserver")
		              .def("number_of_rows", &tab_t::number_of_rows)
		              .def("row_size", &tab_t::row_size)
		              .def("find", &tab_t::find)
		              .def("__getitem__", &tab_t::operator[])
		              .def("compare_without_multiplicity", &tab_t::compare_without_multiplicity)
		              .def("has_nullifying_trace", &tab_t::has_nullifying_trace)
		              .def("nonstandard_loc", &tab_t::nonstandard_loc)
		.def("__iter__", [](const tab_t & tab) {
			return py::make_iterator(tab.begin(), tab.end());
		})
		.def("row", [](const tab_t & tab, unsigned int row) {
			return py::make_iterator(tab.begin_row(row), tab.end_row(row));
		})
		.def("column", [](const tab_t & tab, unsigned int col) {
			return py::make_iterator(tab.begin_column(col), tab.end_column(col));
		})
		.def("__str__", &tab_str);
	}

}
