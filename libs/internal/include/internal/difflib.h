//---------------------------------------------------------------------------------
// C++ port of the Python difflib -- helpers for computing deltas between objects.
//
// Function get_close_matches(word, possibilities, n = 3, cutoff = 0.6) :
//	 Use SequenceMatcher to return list of the best "good enough" matches.
// Function context_diff(a, b) :
//	 For two lists of strings, return a delta in context diff format.
// Function ndiff(a, b) :
//	 Return a delta : the difference between `a` and `b` (lists of strings).
// Function restore(delta, which) :
//	 Return one of the two sequences that generated an ndiff delta.
// Function unified_diff(a, b) :
//	 For two lists of strings, return a delta in unified diff format.
// Class SequenceMatcher :
//   A flexible class for comparing pairs of sequences of any type.
// Class Differ :
//   For producing human - readable deltas from sequences of lines of text.
// Class HtmlDiff :
//   For producing HTML side by side comparison with change highlights.
//---------------------------------------------------------------------------------

#ifndef DIFFLIB_HPP
#define DIFFLIB_HPP

#include <tuple> // std::pair
#include <vector> // std::vector
#include <map> // std::map
#include <set> // std::set
#include <algorithm> // std::sort, std::max, std::min
#include <iosfwd> // std::ostream
#include <locale> // std::isspace

namespace difflib
{
	namespace detail
	{
		inline double calculate_ratio(size_t matches, size_t length)
		{
			if (length == 0)
				return 1;
			return	(2. * matches) / length;
		}

		template <typename string_type>
		inline size_t count_leading(const string_type& line, const typename string_type::value_type& ch)
		{
			size_t i = 0;
			size_t n = line.size();
			while (i < n && line[i] == ch)
				++i;
			return i;
		}

		template <typename string_type>
		inline void rtrim(string_type& s)
		{
			s.erase(
				std::find_if(
					s.rbegin(),
					s.rend(),
					[](const typename string_type::value_type& ch) { return !std::isspace(ch, std::locale{}); })
				.base(),
				s.end());
		}
	}

	template <typename CharT>
	bool NO_JUNK(const CharT&)
	{
		return false;
	}

	enum class tag_t
	{
		t_replace,
		t_delete,
		t_insert,
		t_equal,
		t_unknown
	};

	inline std::ostream& operator << (std::ostream& stream, tag_t tag)
	{
		switch (tag) {
		case tag_t::t_replace: return stream << "replace";
		case tag_t::t_delete: return stream << "delete";
		case tag_t::t_insert: return stream << "insert";
		case tag_t::t_equal: return stream << "equal";
		default: return stream << "unknown";
		}
	}

	struct Block
	{
		Block(size_t i, size_t j, size_t length) : i(i), j(j), length(length) {}
		size_t i, j, length;
	};

	inline std::ostream& operator << (std::ostream& stream, const Block& rhs)
	{
		return stream << "Block(" << rhs.i << ", " << rhs.j << ", " << rhs.length << ")";
	}

	struct OpCode
	{
		OpCode(tag_t tag, size_t i1, size_t i2, size_t j1, size_t j2) : tag(tag), i1(i1), i2(i2), j1(j1), j2(j2) {}
		tag_t tag;
		size_t i1, i2, j1, j2;
	};

	inline std::ostream& operator << (std::ostream& stream, const OpCode& rhs)
	{
		return stream << "OpCode(tag_t::t_" << rhs.tag << ", " << rhs.i1 << ", " << rhs.i2 << ", " << rhs.j1 << ", " << rhs.j2 << ")";
	}

	template <typename StringT>
	struct Delta
	{
		Delta(tag_t tag, const StringT& a, const StringT& b, const std::vector<OpCode>& opcodes = {}) : tag(tag), a(a), b(b), opcodes(opcodes) {}
		tag_t tag;
		StringT a, b;
		std::vector<OpCode> opcodes;
	};

	// SequenceMatcher is a flexible class for comparing pairs of sequences of
	// any type, so long as the sequence elements are hashable.The basic
	// algorithm predates, and is a little fancier than, an algorithm
	// published in the late 1980's by Ratcliff and Obershelp under the
	// hyperbolic name "gestalt pattern matching".The basic idea is to find
	// the longest contiguous matching subsequence that contains no "junk"
	// elements(R - O doesn't address junk).  The same idea is then applied
	// recursively to the pieces of the sequences to the left and to the right
	// of the matching subsequence.This does not yield minimal edit
	// sequences, but does tend to yield matches that "look right" to people.
	//
	// SequenceMatcher tries to compute a "human-friendly diff" between two
	// sequences.Unlike e.g.UNIX(tm) diff, the fundamental notion is the
	// longest *contiguous* & junk - free matching subsequence.That's what
	// catches peoples' eyes.  The Windows(tm) windiff has another interesting
	// notion, pairing up elements that appear uniquely in each sequence.
	// That, and the method here, appear to yield more intuitive difference
	// reports than does diff.This method appears to be the least vulnerable
	// to synching up on blocks of "junk lines", though(like blank lines in
	// ordinary text files, or maybe "<P>" lines in HTML files).That may be
	// because this is the only method of the 3 that has a *concept* of
	// "junk" <wink>.
	template <typename StringT>
	class SequenceMatcher
	{
	public:
		using string_type = StringT;
		using char_type = typename StringT::value_type;
		using JunkPred = bool(*)(const char_type&);

		// Construct a SequenceMatcher
		//
		// Optional arg isjunk is No_JUNK (the default), or a one - argument
		// function that takes a sequence element and returns true iff the
		// element is junk. NO_JUNK is equivalent to passing "return false", i.e.
		// no elements are considered to be junk.
		//
		// Optional arg a is the first of two sequences to be compared. By
		// default, an empty string. The elements of a must overload operator <.
		//
		// Optional arg b is the second of two sequences to be compared.By
		// default, an empty string. The elements of b must overload operator <.
		//
		// Optional arg autojunk should be set to false to disable the
		// "automatic junk heuristic" that treats popular elements as junk
		// (see module documentation for more information).
		SequenceMatcher(
			JunkPred is_junk = NO_JUNK,
			const string_type& a = "",
			const string_type& b = "",
			bool auto_junk = true
		)
			: is_junk(is_junk)
			, a_(nullptr)
			, b_(nullptr)
			, auto_junk(auto_junk)
		{
			set_seqs(a, b);
		}

		// Construct a SequenceMatcher with no junk
		SequenceMatcher(const string_type& a, const string_type& b)
			: SequenceMatcher(NO_JUNK, a, b, false)
		{

		}

		// Set the two sequences to be compared
		void set_seqs(const string_type& a, const string_type& b)
		{
			set_seq1(a);
			set_seq2(b);
		}

		// Set the first sequence to be compared
		//
		// The second sequence to be compared is not changed.
		//
		// SequenceMatcher computes and caches detailed information about the
		// second sequence, so if you want to compare one sequence S against
		// many sequences, use.set_seq2(S) once and call.set_seq1(x)
		// repeatedly for each of the other sequences.
		void set_seq1(const string_type& a)
		{
			if (a_ == &a)
				return;

			a_ = &a;
			matching_blocks.clear();
			opcodes.clear();
		}

		// Set the second sequence to be compared
		//
		// The first sequence to be compared is not changed.
		//
		// SequenceMatcher computes and caches detailed information about the
		// second sequence, so if you want to compare one sequence S against
		// many sequences, use.set_seq2(S) once and call.set_seq1(x)
		// repeatedly for each of the other sequences.
		void set_seq2(const string_type& b)
		{
			if (b_ == &b)
				return;

			b_ = &b;
			matching_blocks.clear();
			opcodes.clear();
			full_b_count.clear();;
			chain_b();
		}

		// Find the longest matching block in a[i1:i2] and b[j1:j2]
		//
		// Of all maximal matching blocks, return one that starts earliest in a, and 
		// of all those maximal matching blocks that start earliest in a, return the one
		// that starts earliest in b
		//
		// If is_junk is defined, first the longest matching block is
		// determined as above, but with the additional restriction that no
		// junk element appears in the block.Then that block is extended as
		// far as possible by matching(only) junk elements on both sides.So
		// the resulting block never matches on junk except as identical junk
		// happens to be adjacent to an "interesting" match.
		//
		// If no blocks match, return (i1, j1, 0)
		Block find_longest_match(size_t i1, size_t i2, size_t j1, size_t j2)
		{
			const string_type& a = *a_;
			const string_type& b = *b_;

			size_t best_i = i1;
			size_t best_j = j1;
			size_t best_size = 0;

			std::map<size_t, size_t> j2len;

			for (size_t i = i1; i < i2; ++i) {
				std::map<size_t, size_t> newj2len;
				for (size_t j : b2j[a[i]]) {
					if (j < j1)
						continue;
					if (j >= j2)
						break;
					size_t k = newj2len[j] = j2len[j - 1] + 1;
					if (k > best_size) {
						best_i = i - k + 1;
						best_j = j - k + 1;
						best_size = k;
					}
				}
				j2len = newj2len;
			}

			while (
				best_i > i1 &&
				best_j > j1 &&
				!is_b_junk(b[best_j - 1]) &&
				a[best_i - 1] == b[best_j - 1]
				) {
				--best_i;
				--best_j;
				++best_size;
			}

			while (
				best_i + best_size < i2 &&
				best_j + best_size < j2 &&
				!is_b_junk(b[best_j + best_size]) &&
				a[best_i + best_size] == b[best_j + best_size]
				) {
				++best_size;
			}

			while (
				best_i > i1 &&
				best_j > j1 &&
				is_b_junk(b[best_j - 1]) &&
				a[best_i - 1] == b[best_j - 1]
				) {
				--best_i;
				--best_j;
				++best_size;
			}

			while (
				best_i + best_size < i2 &&
				best_j + best_size < j2 &&
				is_b_junk(b[best_j + best_size]) &&
				a[best_i + best_size] == b[best_j + best_size]
				) {
				++best_size;
			}

			return Block{ best_i, best_j, best_size };
		}

		// Return list of Blocks describing matching subsequences
		const std::vector<Block>& get_matching_blocks()
		{
			if (!matching_blocks.empty())
				return matching_blocks;

			size_t len_a = a_->size();
			size_t len_b = b_->size();

			std::vector<OpCode> queue{ 1,{ tag_t::t_unknown, 0, len_a, 0, len_b } };

			while (!queue.empty()) {
				OpCode op = queue.back();
				queue.pop_back();
				Block b = find_longest_match(op.i1, op.i2, op.j1, op.j2);
				if (b.length != 0) {
					matching_blocks.push_back(b);
					if (op.i1 < b.i && op.j1 < b.j)
						queue.emplace_back(tag_t::t_unknown, op.i1, b.i, op.j1, b.j);
					if (b.i + b.length < op.i2 && b.j + b.length < op.j2)
						queue.emplace_back(tag_t::t_unknown, b.i + b.length, op.i2, b.j + b.length, op.j2);
				}
			}
			std::sort(matching_blocks.begin(), matching_blocks.end(), [](const Block& lhs, const Block& rhs) { return lhs.i == rhs.i ? lhs.j < rhs.j : lhs.i < rhs.i; });

			Block b1{ 0, 0, 0 };
			std::vector<Block> non_adjacent;
			for (const Block& b2 : matching_blocks) {
				if (b1.i + b1.length == b2.i && b1.j + b1.length == b2.j)
					b1.length += b2.length;
				else {
					if (b1.length != 0)
						non_adjacent.push_back(b1);
					b1 = b2;
				}

			}
			if (b1.length != 0)
				non_adjacent.push_back(b1);
			non_adjacent.emplace_back(len_a, len_b, 0);
			matching_blocks = non_adjacent;
			return matching_blocks;
		}

		// Return list of opcodes describing how to turn a into b
		const std::vector<OpCode>& get_opcodes()
		{
			if (!opcodes.empty())
				return opcodes;

			size_t i = 0;
			size_t j = 0;

			for (const Block& b : get_matching_blocks()) {
				tag_t tag = tag_t::t_unknown;
				if (i < b.i && j < b.j)
					tag = tag_t::t_replace;
				else if (i < b.i)
					tag = tag_t::t_delete;
				else if (j < b.j)
					tag = tag_t::t_insert;

				if (tag != tag_t::t_unknown)
					opcodes.emplace_back(tag, i, b.i, j, b.j);

				i = b.i + b.length;
				j = b.j + b.length;

				if (b.length != 0)
					opcodes.emplace_back(tag_t::t_equal, b.i, i, b.j, j);
			}

			return opcodes;
		}

		std::vector<std::vector<OpCode>> get_grouped_opcodes(size_t n = 3)
		{
			std::vector<OpCode> codes = opcodes;
			if (codes.empty())
				codes.emplace_back(tag_t::t_equal, 0, 1, 0, 1);
			if (codes.front().tag == tag_t::t_equal) {
				OpCode cur = codes.front();
				codes.front() = OpCode{ cur.tag, std::max(cur.i1, cur.i2 - n), cur.i2, std::max(cur.j1, cur.j2 - n), cur.j2 };
			}
			if (codes.back().tag == tag_t::t_equal) {
				OpCode cur = codes.back();
				codes.back() = OpCode{ cur.tag, cur.i1, std::min(cur.i2, cur.i1 + n), cur.j1, std::min(cur.j2, cur.j1 + n) };
			}

			size_t nn = 2 * n;

			std::vector<std::vector<OpCode>> all_groups;
			std::vector<OpCode> group;
			for (OpCode cur : codes) {
				if (cur.tag == tag_t::t_equal && cur.i2 - cur.i1 > nn) {
					group.emplace_back(cur.tag, cur.i1, std::min(cur.i2, cur.i1 + n), cur.j1, std::min(cur.j2, cur.j1 + n));
					all_groups.push_back(group);
					group.clear();
					cur.i1 = std::max(cur.i1, cur.i2 - n);
					cur.j1 = std::max(cur.j1, cur.j2 - n);
				}
				group.push_back(cur);
			}
			if (!group.empty() && !(group.size() == 1 && group.front().tag == tag_t::t_equal))
				all_groups.push_back(group);

			return all_groups;
		}

		// Return a measure of the sequences' similarity (double in [0, 1])
		double ratio()
		{
			size_t matches = 0;
			for (const Block& b : get_matching_blocks())
				matches += b.length;

			return detail::calculate_ratio(matches, a_->size() + b_->size());
		}

		// Return an upper bound on .ratio() relatively quickly
		double quick_ratio()
		{
			if (full_b_count.empty()) {
				for (char_type c : *b_)
					++full_b_count[c];
			}

			std::map<char_type, int> avail;
			size_t matches = 0;

			for (char_type c : *a_) {
				int numb;
				if (avail.find(c) != avail.end())
					numb = avail[c];
				else
					numb = static_cast<int>(full_b_count[c]);
				avail[c] = numb - 1;
				if (numb > 0)
					matches++;
			}

			return detail::calculate_ratio(matches, a_->size() + b_->size());
		}

		// Return an upper bound on ratio() very quickly
		double real_quick_ratio()
		{
			return detail::calculate_ratio(std::min(a_->size(), b_->size()), a_->size() + b_->size());
		}
	private:
		void chain_b()
		{
			const string_type& b = *b_; // For convenience

										// Build b2j ignoring junk
			b2j.clear();
			for (int i = 0; i < b.size(); ++i)
				b2j[b[i]].push_back(i);

			// Purge junk elements
			b_junk.clear();
			if (is_junk != NO_JUNK) {
				for (const auto& pair : b2j) {
					if (is_junk(pair.first))
						b_junk.insert(pair.first);
				}
				for (const auto& elt : b_junk)
					b2j.erase(elt);
			}

			// Purge popular elements that are not junk
			b_popular.clear();
			if (auto_junk && b.size() > 200) {
				size_t n_test = (b.size() / 100) + 1;
				for (const auto& pair : b2j) {
					if (pair.second.size() > n_test)
						b_popular.insert(pair.first);
				}
				for (const auto& elt : b_popular)
					b2j.erase(elt);
			}

		}

		bool is_b_junk(char_type s)
		{
			return b_junk.find(s) != b_junk.end();
		}

		const string_type* a_; // First sequence
		const string_type* b_; // Second sequence
		std::map<char_type, std::vector<size_t>> b2j; // List of non-junk indices into b where each element of b appears
		std::map<char_type, size_t> full_b_count; // Number of times each element in b appears
		std::vector<OpCode> opcodes; // List of opcodes
		std::vector<Block> matching_blocks; // List of matching blocks
		JunkPred is_junk; // User supplied function that takes a sequence element and returns true if the element is junk
		std::set<char_type> b_junk; // Items in b for which is_junk returns true
		std::set<char_type> b_popular; // Nonjunk element in b treated as junk by the heuristic (if enabled)
		bool auto_junk; // If true, heuristic junk collection is enabled
	};

	template <typename string_type, typename It>
	std::vector<string_type> get_close_matches(const string_type& word, It begin, It end, size_t n = 3, double cutoff = 0.6)
	{
		if (n == 0)
			throw std::invalid_argument("n must be > 0");
		if (cutoff < 0. || cutoff > 1.)
			throw std::invalid_argument("cutoff must be in the range 0.0 < cutoff < 1.0");

		std::vector<std::pair<double, string_type>> result;

		SequenceMatcher s;
		s.set_seq2(word);
		while (begin != end) {
			s.set_seq1(*begin);
			if (s.real_quick_ratio() >= cutoff && s.quick_ratio() >= cutoff && s.ratio() >= cutoff)
				result.emplace_back(s.ratio(), *begin);
			++begin;
		}

		std::vector<string_type> ret;
		if (result.size() > n) {
			std::nth_element(result.begin(), result.end() - n, result.end(), [](const auto& lhs, const auto& rhs) {return lhs.first < rhs.first; });
			result = decltype(result)(result.end() - n, result.end());
		}

		std::sort(result.begin(), result.end(), [](const auto& lhs, const auto& rhs) {return lhs.first > rhs.first; });
		for (auto it = result.begin(), end = result.end(); it != end; ++it)
			ret.push_back(it->second);
		return ret;
	}


	template <typename StringT>
	class Differ
	{
	public:
		using string_type = StringT;
		using delta_type = Delta<StringT>;
		using char_type = typename StringT::value_type;
		using LineJunkPred = bool(*)(const string_type&);
		using CharJunkPred = bool(*)(const char_type&);

		Differ(LineJunkPred line_junk = NO_JUNK, CharJunkPred char_junk = NO_JUNK, double cutoff = 0.75)
			: line_junk(line_junk)
			, char_junk(char_junk)
			, a_ptr(nullptr)
			, b_ptr(nullptr)
			, cutoff(cutoff)
		{

		}

		template <typename SequenceT>
		std::vector<delta_type> get_deltas(const SequenceT& a, const SequenceT& b)
		{
			compare(a, b);
			return deltas;
		}

		template <typename SequenceT>
		const std::vector<string_type>& compare(const SequenceT& a, const SequenceT& b)
		{
			if (set_seqs(a, b) && !diffs.empty())
				return diffs;

			SequenceMatcher<SequenceT> cruncher(line_junk, a, b);
			for (const auto& opcode : cruncher.get_opcodes()) {
				switch (opcode.tag) {
				case tag_t::t_replace:
					fancy_replace(a, opcode.i1, opcode.i2, b, opcode.j1, opcode.j2);
					break;
				case tag_t::t_delete:
					dump(tag_t::t_delete, a, opcode.i1, opcode.i2);
					break;
				case tag_t::t_insert:
					dump(tag_t::t_insert, b, opcode.j1, opcode.j2);
					break;
				case tag_t::t_equal:
					dump(tag_t::t_equal, a, opcode.i1, opcode.i2);
					break;
				default:
					throw std::invalid_argument("unknown tag");
				}
			}
			return diffs;
		}

	private:
		template <typename SequenceT>
		void dump(tag_t tag, const SequenceT& x, size_t i1, size_t i2)
		{
			for (size_t i = i1; i < i2; ++i) {
				switch (tag) {
				case tag_t::t_delete:
					diffs.push_back("- " + x[i]);
					deltas.emplace_back(tag_t::t_delete, x[i], "");
					break;
				case tag_t::t_insert:
					diffs.push_back("+ " + x[i]);
					deltas.emplace_back(tag_t::t_insert, "", x[i]);
					break;
				case tag_t::t_equal:
					diffs.push_back("  " + x[i]);
					deltas.emplace_back(tag_t::t_equal, x[i], "");
					break;
				default:
					throw std::invalid_argument("unknown tag");
				}
			}
		}

		template <typename SequenceT>
		void plain_replace(const SequenceT& a, size_t i1, size_t i2, const SequenceT& b, size_t j1, size_t j2)
		{
			if (j2 - j1 < i2 - i1) {
				dump(tag_t::t_insert, b, j1, j2);
				dump(tag_t::t_delete, a, i1, i2);
			}
			else {
				dump(tag_t::t_delete, a, i1, i2);
				dump(tag_t::t_insert, b, j1, j2);
			}
		}

		template <typename SequenceT>
		void fancy_replace(const SequenceT& a, size_t i1, size_t i2, const SequenceT& b, size_t j1, size_t j2)
		{
			SequenceMatcher<string_type> cruncher(char_junk);
			double best_ratio = 0.74;
			size_t eq_i;
			size_t eq_j;
			bool eq_found = false;
			size_t best_i;
			size_t best_j;

			for (size_t j = j1; j < j2; ++j) {
				const string_type& b_j = b[j];
				cruncher.set_seq2(b_j);
				for (size_t i = i1; i < i2; ++i) {
					const string_type& a_i = a[i];
					if (a_i == b_j) {
						if (!eq_found) {
							eq_found = true;
							eq_i = i;
							eq_j = j;
						}
						continue;
					}
					cruncher.set_seq1(a_i);
					if (
						cruncher.real_quick_ratio() > best_ratio &&
						cruncher.quick_ratio() > best_ratio &&
						cruncher.ratio() > best_ratio
						) {
						best_ratio = cruncher.ratio();
						best_i = i;
						best_j = j;
					}
				}
			}

			if (best_ratio < cutoff) {
				if (!eq_found)
					return plain_replace(a, i1, i2, b, j1, j2);
				best_i = eq_i;
				best_j = eq_j;
				best_ratio = 1.0;
			}
			else {
				eq_found = false;
			}

			fancy_helper(a, i1, best_i, b, j1, best_j);

			const string_type& a_elt = a[best_i];
			const string_type& b_elt = b[best_j];
			if (!eq_found) {
				string_type a_tags, b_tags;
				cruncher.set_seqs(a_elt, b_elt);
				deltas.emplace_back(tag_t::t_replace, a_elt, b_elt, cruncher.get_opcodes());
				for (const auto& opcode : cruncher.get_opcodes()) {
					size_t len_a = opcode.i2 - opcode.i1;
					size_t len_b = opcode.j2 - opcode.j1;
					switch (opcode.tag) {
					case tag_t::t_replace:
						a_tags += string_type(len_a, '^');
						b_tags += string_type(len_b, '^');
						break;
					case tag_t::t_delete:
						a_tags += string_type(len_a, '-');
						break;
					case tag_t::t_insert:
						b_tags += string_type(len_b, '+');
						break;
					case tag_t::t_equal:
						a_tags += string_type(len_a, ' ');
						b_tags += string_type(len_b, ' ');
						break;
					default:
						throw std::invalid_argument("unknown tag");
					}
				}
				qformat(a_elt, b_elt, a_tags, b_tags);
			}
			else {
				diffs.push_back("  " + a_elt);
				deltas.emplace_back(tag_t::t_equal, a_elt, "");
			}

			fancy_helper(a, best_i + 1, i2, b, best_j + 1, j2);
		}

		template <typename SequenceT>
		void fancy_helper(const SequenceT& a, size_t i1, size_t i2, const SequenceT& b, size_t j1, size_t j2)
		{
			if (i1 < i2) {
				if (j1 < j2)
					fancy_replace(a, i1, i2, b, j1, j2);
				else
					dump(tag_t::t_delete, a, i1, i2);
			}
			else if (j1 < j2) {
				dump(tag_t::t_insert, b, j1, j2);
			}
		}

		void qformat(const string_type& a_line, const string_type& b_line, string_type a_tags, string_type b_tags)
		{
			using detail::count_leading;
			using detail::rtrim;

			size_t common = std::min(count_leading(a_line, '\t'), count_leading(b_line, '\t'));
			common = std::min(common, count_leading(a_tags.substr(0, common), ' '));
			common = std::min(common, count_leading(b_tags.substr(0, common), ' '));

			a_tags = a_tags.substr(common);
			b_tags = b_tags.substr(common);
			rtrim(a_tags);
			rtrim(b_tags);

			diffs.push_back("- " + a_line);
			if (!a_tags.empty())
				diffs.push_back("? " + string_type(common, '\t') + a_tags + "\n");

			diffs.push_back("+ " + b_line);
			if (!b_tags.empty())
				diffs.push_back("? " + string_type(common, '\t') + b_tags + "\n");
		}

		template <typename SequenceT>
		bool set_seqs(const SequenceT& a, const SequenceT& b)
		{
			if (reinterpret_cast<const void*>(std::addressof(a)) == a_ptr && reinterpret_cast<const void*>(std::addressof(b)) == b_ptr) {
				return true;
			}

			a_ptr = reinterpret_cast<const void*>(std::addressof(a));
			b_ptr = reinterpret_cast<const void*>(std::addressof(b));
			deltas.clear();
			diffs.clear();

			return false;
		}

		LineJunkPred line_junk;
		CharJunkPred char_junk;
		std::vector<string_type> diffs;
		std::vector<delta_type> deltas;
		const void* a_ptr;
		const void* b_ptr;
		const double cutoff;
	};
}

#endif