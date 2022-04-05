#include <pybind11/pybind11.h>
#include "NTensor.hh"

namespace py = pybind11;

namespace cadabra {
	void init_ntensor(py::module& m) {
   	py::class_<NTensor>(m, "NTensor", py::buffer_protocol())
		.def_buffer([](NTensor &nt) -> py::buffer_info {
						size_t stride=sizeof(double);
						std::vector<size_t> strides(nt.shape.size(), 0);
						for(size_t i=0; i<nt.shape.size(); i++) {
							strides[nt.shape.size()-1-i] = stride;
							stride *= nt.shape[i];
							}
						return py::buffer_info(
							nt.values.data(),                         /* Pointer to buffer */
							sizeof(double),                          /* Size of one scalar */
							py::format_descriptor<double>::format(), /* Python struct-style format descriptor */
							nt.shape.size(),                          /* Number of dimensions */
							nt.shape,                                 /* Buffer dimensions */
							strides                                  /* Strides (in bytes) for each index */
													  );
						});
	};

}
