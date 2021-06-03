
#pragma once

#include <numeric>
#include <vector>
#include <boost/numeric/ublas/matrix.hpp> 
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/numeric/ublas/lu.hpp>
#include "Storage.hh"

namespace linear {
	//	bool gaussian_elimination(const std::vector<std::vector<multiplier_t> >&, const std::vector<multiplier_t>& );
	bool gaussian_elimination_inplace(std::vector<std::vector<cadabra::multiplier_t> >&, std::vector<cadabra::multiplier_t>& );

	// Class for solving equations of the form Ax = y for x. First call
	// factorize with the matrix A and then call solve as many times as needed
	template <typename T>
	struct Solver
	{
	public:
		using matrix_type = boost::numeric::ublas::matrix<T>;
		using vector_type = boost::numeric::ublas::vector<T>;

		// Initialise the solver with the matrix A
		bool factorize(const matrix_type& A_);

		// Solve for Ax = y
		vector_type solve(const vector_type& y);

	private:
		matrix_type A;
		boost::numeric::ublas::vector<size_t> P;
		vector_type x;
		size_t N;
	};

	template <typename T>
	bool Solver<T>::factorize(const matrix_type& A_)
	{
		assert(A_.size1() == A_.size2());
		N = A_.size1();
		A = A_;
		P.resize(N);

		// Bring swap and abs into namespace
		using namespace std;
		using namespace boost::numeric::ublas;

		std::iota(P.begin(), P.end(), 0);

		for (size_t i = 0; i < N; ++i) {
			T maxA = 0;
			size_t imax = i;
			for (size_t k = i; k < N; ++k) {
				T absA = abs(A(k, i));
				if (absA > maxA) {
					maxA = absA;
					imax = k;
				}
			}

			if (imax != i) {
				swap(P(i), P(imax)); //pivoting P
				swap(row(A, i), row(A, imax)); //pivoting rows of A
			}

			if (A(i, i) == 0)
				return false;

			for (size_t j = i + 1; j < N; ++j) {
				A(j, i) /= A(i, i);
				for (size_t k = i + 1; k < N; ++k)
					A(j, k) -= A(j, i) * A(i, k);
			}
		}

		return true;
	}


	template <typename T>
	typename Solver<T>::vector_type Solver<T>::solve(const vector_type& y)
	{
		x.resize(y.size());
		for (size_t i = 0; i < N; ++i) {
			x(i) = y(P(i));
			for (size_t k = 0; k < i; ++k)
				x(i) -= A(i, k) * x(k);
		}

		for (size_t i = N; i > 0; --i) {
			for (size_t k = i; k < N; ++k)
				x(i - 1) -= A(i - 1, k) * x(k);
			x(i - 1) = x(i - 1) / A(i - 1, i - 1);
		}

		return x;
	}

	}

