
#include "NInterpolatingFunction.hh"
#include "Exceptions.hh"

// #define DEBUG 1

#ifdef DEBUG
#warning "DEBUG enabled for NInterpolatingFunction.cc"
static bool debug_stop = false;
#define DEBUGLN(ln) if(!debug_stop) { ln; }
#else
#define DEBUGLN(ln)
#endif


using namespace cadabra;

NInterpolatingFunction::NInterpolatingFunction()
	: var_values(0), fun_values(0), slope_values(0), last_index(0), precomputed(false)
	{
	}

size_t NInterpolatingFunction::find_interval(double v) const
	{
	// Same interval as previously?
	if(v >= var_values.values[last_index].real() && v<= var_values.values[last_index+1].real())
		return last_index;

	// Hope that 'v' has just rolled into the next interval...
	++last_index;
	if(v >= var_values.values[last_index].real() && v<= var_values.values[last_index+1].real())
		return last_index;

	// Exhaustive search.
	for(last_index=0; last_index<var_values.values.size()-1; ++last_index)
		if(v >= var_values.values[last_index].real() && v<= var_values.values[last_index+1].real())
			return last_index;

	throw InternalError("NInterpolatingFunction: internal error, please report a bug.");
	}

void NInterpolatingFunction::compute_slopes() const
	{
	if(var_values.values.size() != fun_values.values.size())
		throw ConsistencyException("NInterpolatingFunction: number of variable values != number of function values");

	slope_values.shape = fun_values.shape;
	slope_values.values.clear();
	slope_values.values.resize(var_values.values.size()-1);
	
	for(size_t i=0; i<var_values.values.size()-1; ++i) {
		double dx               = var_values.values[i+1].real() - var_values.values[i].real();
		std::complex<double> dy = fun_values.values[i+1]        - fun_values.values[i];
		slope_values.values[i] = dy / dx;
		}

	precomputed=true;
	}

std::pair<double, double> NInterpolatingFunction::range() const
	{
	return std::make_pair( var_values.values.front().real(),
								  var_values.values.back().real() );
	}

std::complex<double> NInterpolatingFunction::evaluate(double v) const
	{
	if(!precomputed) {
		compute_slopes();
//		spline = boost::math::cubic_b_spline<std::complex<double>>(
//			fun_values.values.begin(), fun_values.values.end(),
//			var_values.values.front(),
//			[&x](size_t i) {
//			return var_values.values[i+1].real() - var_values.values[i].real();
//			});
		}
	
	if(v < var_values.values.front().real() || v > var_values.values.back().real())
		throw ArgumentException("NInterpolatingFunction: evaluated outside domain.");

	size_t i = find_interval(v);
	auto ret = fun_values.values[i] + (v - var_values.values[i].real()) * slope_values.values[i];

	DEBUGLN( std::cerr << "InterpolatingFunction::evaluate: returning " << ret << std::endl; );
	return ret;
	}

variable_ranges_t cadabra::function_domain(const Ex& ex)
	{
	variable_ranges_t ret;

	// Walk the whole tree, collecting ranges from
	// NInterpolatingFunctions (and possibly others
	// at some later stage).

	auto it = ex.begin();
	while(it!=ex.end()) {
		if(std::holds_alternative<std::shared_ptr<NInterpolatingFunction>>(it->content)) {
			auto nif = std::get<std::shared_ptr<NInterpolatingFunction>>(it->content);
			auto rit = ret.find(nif->var);
			if(rit!=ret.end()) {
				std::pair<double, double> oldrange = rit->second;
				oldrange.first  = std::max(nif->range().first, oldrange.first);
				oldrange.second = std::min(nif->range().second, oldrange.second);
				if(oldrange.first > oldrange.second)
					throw ArgumentException("NInterpolatingFunction: domain intersection is empty.");
				rit->second = oldrange;
				}
			else {
				ret[nif->var] = nif->range();
				}
		}
		
		++it;
		}
	
	return ret;
	}
