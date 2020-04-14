#include <vector>
#include <numeric>
#include "properties/Trace.hh"
#include "properties/TableauBase.hh"
#include "Cleanup.hh"
#include "Hash.hh"
#include "meld.hh"
#include "collect_terms.hh"
#include "DisplayTerminal.hh"

#include <boost/numeric/ublas/matrix.hpp> 
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/numeric/ublas/lu.hpp>

using namespace cadabra;

std::string ex_to_string(const Kernel& kernel, const Ex& ex)
{
	std::ostringstream ss;
	DisplayTerminal dt(kernel, ex, true);
	dt.output(ss);
	return "$" + ss.str() + "$";
}

std::string ex_to_string(const Kernel& kernel, Ex::iterator it)
{
	return ex_to_string(kernel, Ex(it));
}

//-------------------------------------------------
// generic useful routines

// Solve Mx = y for x
template <typename T>
struct LinearSolver
{
public:
	using matrix_type = boost::numeric::ublas::matrix<T>;
	using vector_type = boost::numeric::ublas::vector<T>;

	bool factorize(const matrix_type& A_)
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

			for (size_t j = i + 1; j < N; ++j) {
				if (A(i, i) == 0) 
					return false;
				A(j, i) /= A(i, i);
				for (size_t k = i + 1; k < N; ++k)
					A(j, k) -= A(j, i) * A(i, k);
			}
		}

		return true;
	}

	vector_type solve(const vector_type& y)
	{
		for (size_t i = 0; i < N; ++i) {
			x(i) = y(P(i));
			for (int k = 0; k < i; ++k)
				x(i) -= A(i, k) * x(k);
		}

		for (size_t i = N; i > 0; --i) {
			for (size_t k = i; k < N; ++k)
				x(i - 1) -= A(i - 1, k) * x(k);
			x(i - 1) = x(i - 1) / A(i - 1, i - 1);
		}

		return x;
	}

private:
	matrix_type A;
	boost::numeric::ublas::vector<size_t> P;
	vector_type x;
	size_t N;
};

std::vector<Ex::iterator> split_ex(Ex::iterator it, const std::string& delim)
{
	if (*it->name == delim) {
		// Loop over children creating a list
		std::vector<Ex::iterator> res;
		Ex::sibling_iterator beg = it.begin(), end = it.end();
		while (beg != end) {
			res.push_back(beg);
			++beg;
		}
		return res;
	}
	else {
		// Return a list containing only 'it'
		return std::vector<Ex::iterator>(1, it);
	}
}

bool has_TableauBase(const Kernel& kernel, Ex::iterator it)
{
	if (*it->name == "\\prod" || *it->name == "\\sum") {
		for (Ex::sibling_iterator beg = it.begin(), end = it.end(); beg != end; ++beg)
			if (has_TableauBase(beg))
				return true;
		return false;
	}
	else {
		return (kernel.properties.get_composite<TableauBase>(it) != nullptr);
	}
}

bool has_Trace(const Kernel& kernel, Ex::iterator it)
{
	auto p = kernel.properties.get_composite<Trace>(it);
	if (p)
		return true;
	else
		return false;
}

//-------------------------------------------------
// meld stuff

meld::meld(const Kernel& kernel, Ex& ex)
	 : Algorithm(kernel, ex)
{

}

meld::~meld()
{

}

bool meld::can_apply(iterator it)
{
	return
		can_apply_traces(it) ||
		can_apply_tableaux(it);
}

#define APPLY_ROUTINE(name)						\
	if (can_apply_##name (it)) {					\
		switch (apply_##name (it)) {				\
			case  result_t::l_applied:				\
				res = result_t::l_applied;			\
				break;									\
			case result_t::l_error:					\
				return result_t::l_error;			\
				break;									\
			default:										\
				break;									\
		}													\
	}														//end of macro

meld::result_t meld::apply(iterator& it) 
{
	 result_t res = result_t::l_no_action;

	 APPLY_ROUTINE(traces);
	 APPLY_ROUTINE(tableaux);

	 cleanup_dispatch(kernel, tr, it);
	 cleanup_traces(it);
	 cleanup_like_terms(it);

	 return res;
}

void meld::cleanup_traces(iterator it)
{
	 // Remove empty traces    
	 if (has_Trace(kernel, it) && it.number_of_children() == 0) {
		node_zero(it);
	 }
	 else if (*it->name == "\\sum" || *it->name == "\\prod") {
		for (Ex::sibling_iterator beg = it.begin(), end = it.end(); beg != end; ++beg) {
				Ex::iterator newit = beg;
			cleanup_traces(newit);
		}
	}
}

void meld::cleanup_like_terms(iterator it)
{
	collect_terms ct(kernel, tr);
	ct.apply_generic(it, true, false, 0);
}

//-------------------------------------------------
// do_tableaux stuff

AdjformEx meld::symmetrize(Ex::iterator it)
{
	AdjformEx sym(it, index_map, kernel);
	Ex_hasher hasher(HashFlags::HASH_IGNORE_INDEX_ORDER);
	auto terms = split_ex(sym.get_tensor_ex().begin(), "\\prod");

	// Symmetrize in identical tensors
	// Map holding hash of tensor -> { number of indices, {pos1, pos2, ...} }
	std::map<Ex_hasher::result_t, std::pair<size_t, std::vector<size_t>>> idents;
	size_t pos = 0;
	for (const auto& term : split_ex(sym.get_tensor_ex().begin(), "\\prod")) {
		idents[hasher(term)].second.push_back(pos);
		for (Ex::iterator beg = term.begin(), end = term.end(); beg != end; ++beg) 
			idents[hasher(term)].first += is_index(kernel, beg);			
	}
	for (const auto& ident : idents) {
		if (ident.second.second.size() != 1)
			sym.apply_ident_symmetry(ident.second.second, ident.second.first);
	}

	// Young project antisymmetric components
	pos = 0;
	for (auto& it : terms) {
		auto tb = kernel.properties.get<TableauBase>(it);
		if(tb) {
			int siz = tb->size(kernel.properties, tr, it);
			if(siz>0) {
				// FIXME: need to handle all tableaux, not just '0'.
				auto tab = tb->get_tab(kernel.properties, tr, it, 0);
				for (size_t col = 0; col < tab.row_size(0); ++col) {
					if (tab.column_size(col) > 1) {
						std::vector<size_t> indices;
						for (auto beg = tab.begin_column(col), end = tab.end_column(col); beg != end; ++beg)
							indices.push_back(*beg + pos);
						std::sort(indices.begin(), indices.end());
						sym.apply_young_symmetry(indices, true);
						}
					}
				}
			}
		pos += it.number_of_children();
	}

	// Young project symmetric components
	pos = 0;
	for (auto& it : terms) {
		// Apply the symmetries
		auto tb = kernel.properties.get<TableauBase>(it);
		if(tb) {
			int siz = tb->size(kernel.properties, tr, it);
			if(siz>0) {
				// FIXME: need to handle all tableaux, not just '0'.
				auto tab = tb->get_tab(kernel.properties, tr, it, 0);
				for (size_t row = 0; row < tab.number_of_rows(); ++row) {
					if (tab.row_size(row) > 1) {
						std::vector<size_t> indices;
						for (auto beg = tab.begin_row(row), end = tab.end_row(row); beg != end; ++beg)
							indices.push_back(*beg + pos);
						std::sort(indices.begin(), indices.end());
						sym.apply_young_symmetry(indices, false);
						}
					}
				}
			}
		pos += it.number_of_children();
		}
	return sym;
}

bool meld::can_apply_tableaux(iterator it)
{
	return
		*it->name == "\\sum" &&
		has_TableauBase(kernel, it); 
}

bool meld::can_apply_traces(iterator it)
{
	if (has_Trace(kernel, it))
		return true;

	if (*it->name == "\\sum") {
		for (Ex::sibling_iterator beg = it.begin(), end = it.end(); beg != end; ++beg) {
			if (has_Trace(kernel, beg))
				return true;
		}
	}
	return false;
}

meld::result_t meld::apply_tableaux(iterator it)
{
	using namespace boost::numeric::ublas;
	using matrix_type = matrix<AdjformEx::rational_type>;
	using vector_type = vector<AdjformEx::rational_type>;
	
	result_t res = result_t::l_no_action;

	// 'coeffs' is a square matrix which enough terms of the young projection to
	// ensure that when solving for linear dependence there are as many
	// unknowns as equations
	matrix_type coeffs;
	LinearSolver<AdjformEx::rational_type> solver;

	// The adjform in position 'i' of 'mapping' represents the term corresponding
	// to the 'i'th row of 'coeffs'
	std::vector<Adjform> mapping;

	// A list of all the adjforms encountered so far
	std::vector<AdjformEx> adjforms;
	auto terms = split_ex(it, "\\sum");
	terms.erase(
		std::remove_if(terms.begin(), terms.end(), [this](Ex::iterator it) { return !has_indices(kernel, it); }),
		terms.end()
	);
	for (size_t term_idx = 0; term_idx < terms.size(); ++term_idx) {
		auto cur_adjform = symmetrize(terms[term_idx]);
		if (cur_adjform.empty()) {
			// Empty adjform means that the term is identically
			// equal to 0
			node_zero(terms[term_idx]);
			terms.erase(terms.begin() + term_idx);
			--term_idx;
			res = result_t::l_applied;
		}
		else {
			// See if the current term is a linear combination of
			// terms previously encountered
			bool has_solution = true;

			// Initialize 'y' which contains the coefficients in the
			// young projection
			vector_type x, y;
			y.resize(coeffs.size1());
			for (size_t i = 0; i < mapping.size(); ++i)
				y(i) = cur_adjform.get(mapping[i]);

			if (coeffs.size1() == 0) {
				has_solution = false;
			}
			else {
				// x is guaranteed to be a solution as the 'coeffs' matrix
				// is square. To check whether it is actually a solution, go
				// back over all the adjforms and ensure that the equation
				// holds for each term
				x = solver.solve(y);
				//std::cerr << x.transpose() << '\n';
				std::vector<AdjformEx::const_iterator> lhs_its;
				AdjformEx::const_iterator rhs_it = cur_adjform.begin();
				Adjform cur_term = rhs_it->first;

				// Populate the lhs_its vector and find the smallest term
				for (const auto& adjform : adjforms) {
					auto it = adjform.begin();
					if (it->first < cur_term)
						cur_term = it->first;
					lhs_its.push_back(it);
				}
				
				size_t n_finished = 0;
				while (n_finished < lhs_its.size()) {
					// Ensure that the next term is bigger than any other term to begin with
					Adjform next_term;
					next_term.push_back(std::numeric_limits<Adjform::value_type>::max());
					AdjformEx::rational_type sum = 0;
					for (int i = 0; i < adjforms.size(); ++i) {
						if (lhs_its[i] != adjforms[i].end() && lhs_its[i]->first == cur_term) {
							sum += x(i) * lhs_its[i]->second;
							++lhs_its[i];
							if (lhs_its[i] == adjforms[i].end())
								++n_finished;
						}
						if (lhs_its[i] != adjforms[i].end() && lhs_its[i]->first < next_term)
							next_term = lhs_its[i]->first;
					}

					AdjformEx::rational_type rhs_sum;
					if (rhs_it == cur_adjform.end() || rhs_it->first != cur_term) {
						rhs_sum = 0;
					}
					else {
						rhs_sum = rhs_it->second;
						++rhs_it;
					}

					if (sum != rhs_sum) {
						//std::cerr << "solution failed for " << cur_term << '\n';
						has_solution = false;
						break;
					}
					if (rhs_it != cur_adjform.end() && rhs_it->first < next_term)
						next_term = rhs_it->first;
					cur_term = next_term;
				}
				if (rhs_it != cur_adjform.end())
					has_solution = false;
			}

			if (has_solution) {
				node_zero(terms[term_idx]);
				terms.erase(terms.begin() + term_idx);
				--term_idx;
				for (size_t i = 0; i < adjforms.size(); ++i) {
					if (x(i) != 0) {
						tr.erase(terms[i]);
						terms[i] = tr.append_child(it, str_node("\\prod"));
						auto prefactor = tr.append_child(terms[i], str_node("\\sum"));
						tr.append_child(prefactor, adjforms[i].get_prefactor_ex().begin());
						auto new_term = tr.append_child(prefactor, cur_adjform.get_prefactor_ex().begin());
						multiply(new_term->multiplier, x(i));
						adjforms[i].get_prefactor_ex() = prefactor;
						tr.append_child(terms[i], adjforms[i].get_tensor_ex().begin());
					}
				}
			}
			else {
				// Add a row to the adjform
				coeffs.resize(coeffs.size1() + 1, coeffs.size2() + 1);
				adjforms.push_back(cur_adjform);
				bool found = false;

				for (const auto& kv : cur_adjform) {
					auto pos = std::find(mapping.begin(), mapping.end(), kv.first);
					if (pos == mapping.end()) {
						// Fill in bottom row
						for (size_t i = 0; i < adjforms.size(); ++i)
							coeffs(coeffs.size1() - 1, i) = adjforms[i].get(kv.first);
						// Fill in the righthand column
						for (size_t i = 0; i < mapping.size(); ++i)
							coeffs(i, coeffs.size2() - 1) = cur_adjform.get(mapping[i]);

						if (solver.factorize(coeffs)) {
							mapping.push_back(kv.first);
							found = true;
							break;
						}
					}
				}
				if (!found)
					throw std::runtime_error("Could not find a suitable element to add to the matrix");
			}
		}
	}

	return res;
}

//-------------------------------------------------
// do_trace stuff

void cycle_vec(std::vector<size_t>& vec, size_t n)
{
	n %= vec.size();
	
}

struct TraceTerm
{ 
	 TraceTerm(Ex::iterator it, mpq_class parent_multiplier, IndexMap& index_map);
	 Ex::iterator it;
	 Adjform names, indices; 
	 mpq_class parent_multiplier;
	 std::vector<size_t> pushes;
};

TraceTerm::TraceTerm(Ex::iterator it, mpq_class parent_multiplier, IndexMap& index_map)
	: it(it)
	, parent_multiplier(parent_multiplier)
{
	Ex_hasher hasher(HashFlags::HASH_IGNORE_TOP_MULTIPLIER | HashFlags::HASH_IGNORE_INDEX_ORDER);
	 auto terms = split_ex(it, "\\prod");
	 for (const auto& term : terms) {
		  names.push_back(index_map.get_free_index(hasher(term)));
		  pushes.push_back(0);
		  for (Ex::sibling_iterator beg = term.begin(), end = term.end(); beg != end; ++beg) {
				if (beg->is_index()) {
					indices.push_back(index_map.get_free_index(hasher(term)));
					++pushes.back();
				}
		  }
	 }
}

std::vector<TraceTerm> collect_trace_terms(Ex::iterator it, const Kernel& kernel, IndexMap& index_map)
{
	 // If a trace node just return all the children
	if (has_Trace(kernel, it)) {
		Ex::sibling_iterator beg = it.begin(), end = it.end();
		if (beg == end)
			return {};
		if (*beg->name != "\\sum")
			return { TraceTerm(beg, *it->multiplier, index_map) };
		std::vector<TraceTerm> ret;
		for (Ex::sibling_iterator a = beg.begin(), b = beg.end(); a != b; ++a)
			ret.emplace_back(a, *it->multiplier, index_map);
		return ret;
	}
	
	// If a sum node, collect all trace nodes
	if (*it->name == "\\sum") {
		std::vector<TraceTerm> ret;
		for (Ex::sibling_iterator a = it.begin(), b = it.end(); a != b; ++a) {
			if (has_Trace(kernel, a)) {
				auto nodes = collect_trace_terms(a, kernel, index_map);
				ret.insert(ret.end(), nodes.begin(), nodes.end());
			}
		}
		return ret;
	}

	// Else return nothing
	return {};
}

meld::result_t meld::apply_traces(iterator it)
{
	auto terms = collect_trace_terms(it, kernel, index_map);
	if (terms.empty())
		return result_t::l_no_action;

	auto res = result_t::l_no_action;
	for (size_t i = 0; i < terms.size() - 1; ++i) {
		for (size_t j = i + 1; j < terms.size(); ++j) {
			auto perm = terms[j];
			do {
				if (terms[i].names == perm.names && terms[i].indices == perm.indices) {
					multiply(terms[i].it->multiplier, 1 + ((terms[j].parent_multiplier * *terms[j].it->multiplier) / (terms[i].parent_multiplier * *terms[i].it->multiplier)));
					tr.erase(terms[j].it);
					terms.erase(terms.begin() + j);
					--j;
					res= result_t::l_applied;
					break;
				}

				perm.names.rotate(1);
				perm.indices.rotate(perm.pushes.back());
				std::rotate(perm.pushes.begin(), perm.pushes.end() - 1, perm.pushes.end());
			} while (perm.names != terms[j].names || perm.indices != terms[j].indices);
		}
	}
	return res;
}
