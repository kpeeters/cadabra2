#include "NTensor.hh"
#include <stdexcept>
#include "Stopwatch.hh"
#include <cassert>

using namespace cadabra;

NTensor::NTensor(const std::vector<size_t>& shape_, double val)
	: shape(shape_)
	{
	size_t len=1;
	for(auto dim: shape)
		len *= dim;

	values.resize(len);
	for(auto& v: values)
		v=val;
	}

NTensor::NTensor(const std::vector<double>& vals)
	: values(vals)
	{
	shape.push_back(values.size());
	}

NTensor::NTensor(double val)
	{
	values.push_back(val);
	shape.push_back(1);
	}

NTensor::NTensor(const NTensor& other)
	{
	shape=other.shape;
	values=other.values;
	}

NTensor NTensor::linspace(double from, double to, size_t steps)
	{
	NTensor res(std::vector<size_t>({steps}), 0.0);
	assert(steps>1);

	for(size_t i=0; i<steps; ++i) {
		res.values[i] = from + i*(to-from)/(steps-1);
		}
	return res;
	}

NTensor& NTensor::operator=(const NTensor& other)
	{
	shape=other.shape;
	values=other.values;
	return *this;
	}

double NTensor::at(const std::vector<size_t>& indices) const
	{
	if(indices.size()!=shape.size())
		throw std::range_error("NTensor::at: number of indices != shape length.");

	size_t idx = 0;
	size_t stride=1;

	for(size_t p=indices.size(); p-- != 0; ) {
		if(indices[p]>=shape[p])
			throw std::range_error("NTensor::at: index out of range.");
		idx += stride*indices[p];
		stride *= shape[p];
		}

	if(idx >= values.size())
		throw std::range_error("NTensor::at: indices out of range.");

	return values[idx];
	}

double& NTensor::at(const std::vector<size_t>& indices)
	{
	if(indices.size()!=shape.size())
		throw std::range_error("NTensor::at: number of indices != shape length.");

	size_t idx = 0;
	size_t stride=1;

	for(size_t p=indices.size(); p-- != 0; ) {
		if(indices[p]>=shape[p])
			throw std::range_error("NTensor::at: index out of range.");
		idx += stride*indices[p];
		stride *= shape[p];
		}

	if(idx >= values.size())
		throw std::range_error("NTensor::at: indices out of range.");

	return values[idx];
	}

std::ostream& operator<<(std::ostream &str, const NTensor &nt)
	{
	// For an {a,b} tensor, we display as a vector of size 'a', each
	// element of which is a vector of size 'b'. And so on.

	for(size_t p=0; p<nt.shape.size(); ++p)
		str << "[";

	for(size_t i=0; i<nt.values.size(); ++i) {
		str << nt.values[i];

		// Closing/re-opening.
		size_t mult=1;
		for(size_t p=nt.shape.size(); p-- != 0; ) {
			// 2,4,3 -> 2, 8, 24
			mult *= nt.shape[p];
			if((i+1)%mult == 0)
				str << "]";
			}

		if(i+1<nt.values.size()) {
			str << ", ";
			mult=1;
			for(size_t p=nt.shape.size(); p-- != 0; ) {
				mult *= nt.shape[p];
				if((i+1)%mult == 0)
					str << "[";
				}
			}
		}

	return str;
	}

NTensor& NTensor::apply(double (*fun)(double))
	{
	for(auto& v: values)
		v = fun(v);

	return *this;
	}

NTensor& NTensor::operator+=(const NTensor& other)
	{
	for(size_t p=0; p<shape.size(); ++p)
		if(shape[p]!=other.shape[p])
			throw std::range_error("NTensor::operator+=: shapes do not match.");

	for(size_t i=0; i<values.size(); ++i)
		values[i] += other.values[i];

	return *this;
	}

NTensor& NTensor::operator*=(const NTensor& other)
	{
	for(size_t p=0; p<shape.size(); ++p)
		if(shape[p]!=other.shape[p])
			throw std::range_error("NTensor::operator+=: shapes do not match.");

	for(size_t i=0; i<values.size(); ++i)
		values[i] *= other.values[i];

	return *this;
	}

NTensor NTensor::outer_product(const NTensor& a, const NTensor& b)
	{
	// std::cerr << "multiplying " << a << "\n"
	// 			 << "       with " << b << std::endl;
	std::vector<size_t> res_shape;
	res_shape.insert(res_shape.end(), a.shape.begin(), a.shape.end());
	res_shape.insert(res_shape.end(), b.shape.begin(), b.shape.end());

	NTensor res( res_shape, 0.0 );

	for(size_t i=0; i<res.values.size(); ++i) {
		size_t idx_a = i / b.values.size();
		size_t idx_b = i % b.values.size();
		assert(idx_a < a.values.size());
		assert(idx_b < b.values.size());

		res.values[i] = a.values[idx_a] * b.values[idx_b];
		}

	return res;
	}

NTensor NTensor::broadcast(std::vector<size_t> new_shape, size_t pos) const
	{
	// for(auto s: new_shape)
	// 	std::cerr << s << ", ";
	// std::cerr << "\n" << pos << std::endl;
	assert( pos < new_shape.size() );
	assert( shape.size()==1 );
	assert( new_shape[pos]==shape[0] );


	NTensor res(new_shape, 0.);

	size_t lower = 1, higher=1;
	for(size_t s=pos+1; s<new_shape.size(); ++s)
		lower  *= new_shape[s];
	higher = lower * new_shape[pos];

	//  std::cerr << "lower: " << lower << "\nhigher: " << higher << std::endl;

	// Stopwatch sw;
	// sw.start();
	for(size_t i=0; i<res.values.size(); ++i) {
		size_t orig_i = (i % higher) / lower;
		// std::cerr << i << " -> " << orig_i << std::endl;
		assert( orig_i < new_shape[pos] );

		res.values[i] = values[orig_i];
		}

	// sw.stop();
	// std::cerr << "broadcast to " << res.values.size() <<  " took " << sw << std::endl;

	return res;
	}
