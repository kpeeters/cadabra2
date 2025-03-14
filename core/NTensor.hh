
#pragma once

#include <vector>
#include <iostream>
#include <complex>

/// \ingroup numerical
///
/// Class to hold numerical values structured in tensor form, that is,
/// a multi-dimensional array.

namespace cadabra {

	class NTensor {
		public:
			/// Initialise by giving the dimension for each index.  Storage
			/// is generalised row-major.  Display follows that convention:
			/// we use maths matrix conventions for printing, that is,
			/// earlier indices are more major, and are iterated over in a
			/// more outer loop.
			explicit NTensor(const std::vector<size_t>& shape, std::complex<double> val);

			/// Initialise as a vector of std::complex<double>s; sets shape automatically
			explicit NTensor(const std::vector<std::complex<double>>& vals);
			/// As above, but with doubles instead of complex doubles.
			explicit NTensor(const std::vector<double>& vals);

			/// Helper functions to be able to initialise a tensor with an initialiser list.
			explicit NTensor(std::initializer_list<std::complex<double>> vals);
			explicit NTensor(std::initializer_list<double> vals);

			/// Initialise as a scalar; sets shape automatically.
			explicit NTensor(std::complex<double>);
			explicit NTensor(double);

			/// Copy constructor.
			explicit NTensor(const NTensor&);

			/// Move constructor.
			NTensor(NTensor&&);

			/// Create equally spaced values in a range.
			static NTensor linspace(std::complex<double> from, std::complex<double> to, size_t steps);

			/// Assignment operator.
			NTensor& operator=(const NTensor&);

			/// Move assignment operators.
			NTensor& operator=(NTensor&&) noexcept;
			NTensor& operator=(const NTensor&&) noexcept;

			/// Addition operator. This requires the shapes to match.
			NTensor& operator+=(const NTensor&);

			/// Element-wise multiplication operator. This requires the shapes to match.
			NTensor& operator*=(const NTensor&);

			/// Multiplyall elements with a scalar.
			NTensor& operator*=(const std::complex<double>&);
			NTensor& operator*=(double);

			/// Element-wise pow operator (self**b, or pow(self,b)). Requires the shapes to match.
			NTensor& pow(const NTensor&);

			/// Get the value of a scalar NTensor.
			std::complex<double>  at() const;
			
			/// Get the value of the tensor at the indicated component.
			std::complex<double>  at(const std::vector<size_t>& indices) const;

			/// Get the value of the tensor at the indicated component.
			std::complex<double>& at(const std::vector<size_t>& indices);

			/// Expand the shape of the tensor to the specified shape
			/// by broadcasting to the other dimensions. Effectively,
			///
			///    A_{i} ->  A_{k i l}
			///
			///  shape {2} tensor [3,4] to shape {4,2} pos 1:
			///
			///   -> [[3,4], [3,4]]
			///
			/// For now only works if the original shape is one-dimensional,
			/// that is, a vector (as above).

			NTensor broadcast(std::vector<size_t> new_shape, size_t pos) const;

			/// Outer product of two NTensors. The shape becomes the
			/// concatenation of the two shapes, with the shape of `a`
			/// coming first.
			///
			///    a       b
			///  { 3 } x { 4 } ->  { 3, 4}.

			static NTensor outer_product(const NTensor& a, const NTensor& b);

			/// Apply a scalar function `fun` to all elements, return
			/// a reference to itself.
			NTensor& apply(std::complex<double> (*fun)(const std::complex<double>&));

			/// Test if all values of the tensor are real.
			bool is_real() const;
			
			friend std::ostream& operator<<(std::ostream&, const NTensor&);

			std::vector<size_t> shape;
			std::vector<std::complex<double>> values;
	};

	std::ostream& operator<<(std::ostream &, const NTensor &);

}
