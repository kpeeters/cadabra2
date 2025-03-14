#include <pybind11/pybind11.h>
#include "NTensor.hh"
#include <sstream>

namespace py = pybind11;

// #define DEBUG 1

namespace cadabra {
	void init_ntensor(py::module& m) {
	py::class_<NTensor>(m, "NTensor", py::buffer_protocol())
		.def("is_real", &NTensor::is_real)
		.def("__str__", [](NTensor& nt) {
			std::ostringstream str;
			str << nt;
			return str.str();
			})
		.def_buffer([](NTensor &nt) -> py::buffer_info {
			if(nt.is_real()) {
				size_t stride=sizeof(std::complex<double>);
				std::vector<size_t> strides(nt.shape.size(), 0);
				for(size_t i=0; i<nt.shape.size(); i++) {
					strides[nt.shape.size()-1-i] = stride;
					stride *= nt.shape[nt.shape.size()-1-i];
					}
#ifdef DEBUG
				for(size_t i=0; i<nt.shape.size(); ++i)
					std::cerr << "shape " << i << " = " << nt.shape[i] << std::endl;
				for(size_t i=0; i<strides.size(); ++i)
					std::cerr << "stride " << i << " = " << strides[i] << std::endl;
#endif
				return py::buffer_info(
					nt.values.data(),                         /* Pointer to buffer */
					sizeof(double),                          /* Size of one scalar */
					"d", /* Python struct-style format descriptor */
					nt.shape.size(),                          /* Number of dimensions */
					nt.shape,                                 /* Buffer dimensions */
					strides                                  /* Strides (in bytes) for each index */
											  );
				}
			else {
#ifdef DEBUG
				//std::cerr << "at(10,20) = " << nt.at({10,20}) << std::endl;
#endif
				size_t stride=sizeof(std::complex<double>);
				std::vector<size_t> strides(nt.shape.size(), 0);
				for(size_t i=0; i<nt.shape.size(); i++) {
					strides[nt.shape.size()-1-i] = stride;
					stride *= nt.shape[nt.shape.size()-1-i];
					}
#ifdef DEBUG
				for(size_t i=0; i<nt.shape.size(); ++i)
					std::cerr << "shape " << i << " = " << nt.shape[i] << std::endl;
				for(size_t i=0; i<strides.size(); ++i)
					std::cerr << "stride " << i << " = " << strides[i] << std::endl;
#endif
				return py::buffer_info(
					nt.values.data(),                         /* Pointer to buffer */
					sizeof(std::complex<double>),                          /* Size of one scalar */
					"Zd", /* Python struct-style format descriptor */
					nt.shape.size(),                          /* Number of dimensions */
					nt.shape,                                 /* Buffer dimensions */
					strides                                  /* Strides (in bytes) for each index */
											  );
				}
			});
	}
}
