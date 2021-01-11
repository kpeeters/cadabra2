#include <vector>
#include <numeric>

#include "Cleanup.hh"
#include "DisplayTerminal.hh"
#include "meld.hh"
#include "properties/Trace.hh"
#include "properties/TableauBase.hh"
#include "properties/SelfNonCommuting.hh"
#include "properties/NonCommuting.hh"

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

template <typename FilterFunc>
std::vector<Ex::iterator> split_ex(Ex::iterator it, const std::string& delim, FilterFunc filter)
{
	if (*it->name == delim) {
		// Loop over children creating a list
		std::vector<Ex::iterator> res;
		Ex::sibling_iterator beg = it.begin(), end = it.end();
		while (beg != end) {
			if (filter(beg))
				res.push_back(beg);
			++beg;
		}
		return res;
	}
	else {
		// Return a list containing only 'it'
		return filter(it) ? std::vector<Ex::iterator>(1, it) : std::vector<Ex::iterator>{};
	}
}



meld::meld(const Kernel& kernel, Ex& ex)
	: Algorithm(kernel, ex)
	, index_map(kernel)
{

}

meld::~meld()
{

}

bool meld::can_apply(iterator it)
{
	return can_apply_traces(it) || can_apply_tableaux(it);
}

meld::result_t meld::apply(iterator& it)
{
	result_t res = result_t::l_no_action;

	// Traces
	if (can_apply_traces(it)) {
		switch (apply_traces(it)) {
		case result_t::l_applied: res = result_t::l_applied; break;
		case result_t::l_error: return result_t::l_error; break;
		default: break;
		}
	}

	// Tableaux
	if (can_apply_tableaux(it)) {
		switch (apply_tableaux(it)) {
		case result_t::l_applied: res = result_t::l_applied; break;
		case result_t::l_error: return result_t::l_error; break;
		default: break;
		}
	}

	cleanup_dispatch(kernel, tr, it);
	return res;
}


// Tableaux routines

bool meld::can_apply_tableaux(iterator it)
{
	// The tableaux routine can be applied for any node which has indices, or
	// a sum which contains nodes which have indices
	if (has_indices(kernel, it))
		return true;
	if (*it->name == "\\sum") {
		for (Ex::sibling_iterator beg = it.begin(), end = it.end(); beg != end; ++beg)
			if (has_indices(kernel, beg))
				return true;
	}

	return false;
}

// 
struct Ident {
	Ident() : n_indices(0) {}
	size_t n_indices;
	std::vector<Ex::iterator> its;
	std::vector<size_t> positions;

	std::vector<std::vector<int>> generate_commutation_matrix(const Kernel& kernel) const
	{
		Ex_comparator comp(kernel.properties);
		std::vector<std::vector<int>> cm(its.size(), std::vector<int>(its.size()));
		for (size_t i = 0; i < its.size(); ++i) {
			for (size_t j = 0; j < its.size(); ++j) {
				if (i == j)
					continue;
				cm[i][j] = comp.can_move_adjacent(Ex::parent(its[i]), its[i], its[j]) * comp.can_swap(its[i], its[j], Ex_comparator::match_t::subtree_match);
			}
		}
		return cm;
	}
};

AdjformEx meld::symmetrize(Ex::iterator it)
{
	AdjformEx sym(tr, it, index_map, kernel);
	auto prod = sym.get_tensor_ex().begin();
	auto terms = split_ex(prod, "\\prod");

	// Symmetrize in identical tensors
	// Map holding hash of tensor -> { number of indices, {pos1, pos2, ...} }
	std::map<nset_t::iterator, Ident, nset_it_less> idents;
	size_t pos = 0;
	for (const auto& term : split_ex(prod, "\\prod")) {
		auto elem = idents.insert({ term->name, {} });
		auto& ident = elem.first->second;
		if (elem.second) {
			// Insertion took place, count indices
			ident.n_indices = 0;
			for (Ex::iterator beg = term.begin(), end = term.end(); beg != end; ++beg)
				ident.n_indices += is_index(kernel, beg);
		}
		ident.its.push_back(term);
		ident.positions.push_back(pos);
		pos += ident.n_indices;
	}
	for (const auto& ident : idents) {
		if (ident.second.positions.size() != 1) {
			sym.apply_ident_symmetry(
				ident.second.positions, ident.second.n_indices,
				ident.second.generate_commutation_matrix(kernel));
		}
	}

	// Young project antisymmetric components
	pos = 0;
	for (auto& it : terms) {
		auto tb = kernel.properties.get<TableauBase>(it);
		if (tb) {
			int siz = tb->size(kernel.properties, tr, it);
			if (siz > 0) {
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
		if (tb) {
			int siz = tb->size(kernel.properties, tr, it);
			if (siz > 0) {
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

	vector_type solve(const vector_type& y)
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

private:
	matrix_type A;
	boost::numeric::ublas::vector<size_t> P;
	vector_type x;
	size_t N;
};

bool similar_form(const Kernel& kernel, Ex::iterator it1, Ex::iterator it2)
{
	if (it1->name != it2->name || it1->fl.parent_rel != it2->fl.parent_rel)
		return false;

	auto beg1 = it1.begin(), end1 = it1.end();
	auto beg2 = it2.begin(), end2 = it2.end();

	while (beg1 != end1 && beg2 != end2) {
		if (beg1->fl.parent_rel != beg2->fl.parent_rel)
			return false;
		if (is_index(kernel, beg1) && is_index(kernel, beg2)) {
			it1.skip_children();
			it2.skip_children();
		}
		else {
			if (beg1->name != beg2->name)
				return false;
		}
		++beg1, ++beg2;
	}

	return beg1 == end1 && beg2 == end2;
}

bool similar_tensor_form(const Kernel& kernel, Ex& tr, Ex::iterator it1, Ex::iterator it2)
{
	IndexMap im(kernel);
	AdjformEx a1(tr, it1, im, kernel);
	AdjformEx a2(tr, it2, im, kernel);
	return similar_form(kernel, a1.get_tensor_ex().begin(), a2.get_tensor_ex().begin());
}

meld::result_t meld::apply_tableaux(iterator it)
{
	using namespace boost::numeric::ublas;
	using matrix_type = matrix<AdjformEx::rational_type>;
	using vector_type = vector<AdjformEx::rational_type>;

	result_t res = result_t::l_no_action;

	// Split 'it' into lists of terms which have a matching term (and are
	// therefore candidates to be melded)
	std::vector<std::vector<Ex::iterator>> patterns;
	for (const auto& term : split_ex(it, "\\sum", [this](Ex::iterator it) { return has_indices(kernel, it); })) {
		bool found = false;
		for (auto& pattern : patterns) {
			if (similar_tensor_form(kernel, tr, term, pattern[0])) {
				found = true;
				pattern.push_back(term);
				break;
			}
		}
		if (!found)
			patterns.emplace_back(1, term);
	}

	for (auto& terms : patterns) {
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
						for (size_t i = 0; i < adjforms.size(); ++i) {
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
							// Fill in the bottom right element
							coeffs(coeffs.size1() - 1, coeffs.size2() - 1) = cur_adjform.get(kv.first);
							if (solver.factorize(coeffs)) {
								adjforms.push_back(std::move(cur_adjform));
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
	}

	return res;
}


// Trace routiness

bool meld::can_apply_traces(iterator it)
{
	return kernel.properties.get<Trace>(it) != nullptr;
}


bool has_NonCommuting(const Kernel& kernel, Ex::iterator it)
{
	return kernel.properties.get<NonCommuting>(it) != nullptr || kernel.properties.get<SelfNonCommuting>(it) != nullptr;
}

Ex get_noncommuting(const Kernel& kernel, Ex::iterator it)
{
	Ex res("\\prod");
	auto terms = split_ex(it, "\\prod");
	for (const auto& term : terms) {
		if (has_NonCommuting(kernel, term))
			res.append_child(res.begin(), term);
	}
	return res;
}

Ex combine_commuting(const Kernel& kernel, Ex::iterator a, Ex::iterator b)
{
	Ex res("\\prod");
	auto commuting = res.append_child(res.begin(), str_node("\\sum"));
	auto noncommuting = res.begin();

	auto a_commuting = res.append_child(commuting, str_node("\\prod"));
	multiply(a_commuting->multiplier, *a->multiplier);
	for (Ex::sibling_iterator beg = a.begin(), end = a.end(); beg != end; ++beg) {
		if (!has_NonCommuting(kernel, beg))
			res.append_child(a_commuting, (Ex::iterator)beg);
		else
			res.append_child(noncommuting, (Ex::iterator)beg);
	}

	auto b_commuting = res.append_child(commuting, str_node("\\prod"));
	multiply(b_commuting->multiplier, *b->multiplier);
	for (Ex::sibling_iterator beg = b.begin(), end = b.end(); beg != end; ++beg) {
		if (!has_NonCommuting(kernel, beg))
			res.append_child(b_commuting, (Ex::iterator)beg);
	}

	return res;
}

void cycle_ex(Ex& tr, Ex::iterator parent)
{
Ex to_move(parent.begin());
	tr.erase(parent.begin());
	tr.append_child(parent, to_move.begin());
}

meld::result_t meld::apply_traces(iterator it)
{
	result_t res = result_t::l_no_action;
	auto terms = split_ex(it.begin(), "\\sum", [](Ex::iterator it) { return *it->name == "\\prod"; });

	Ex names = get_noncommuting(kernel, terms[0]);
	names.begin()->name = name_set.insert("\\comma").first;
	
	for (size_t i = 0; i < terms.size(); ++i) {
		Ex i_terms = get_noncommuting(kernel, terms[i]);
		for (size_t j = i + 1; j < terms.size(); ++j) {
			Ex j_terms = get_noncommuting(kernel, terms[j]);
			const size_t N = j_terms.begin().number_of_children();
			if (N != i_terms.begin().number_of_children())
				continue;
			for (size_t k = 0; k < N; ++k) {
				if (Adjform::compare(i_terms.begin(), j_terms.begin(), kernel)) {
					res = result_t::l_applied;
					terms[i] = tr.replace(terms[i], combine_commuting(kernel, terms[i], terms[j]).begin());
					tr.erase(terms[j]);
					terms.erase(terms.begin() + j);
					--j;
					break;
				}
				cycle_ex(tr, j_terms.begin());
			}
		}
	}

	return res;
}
