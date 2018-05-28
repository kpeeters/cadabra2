
#include "Linear.hh"

typedef cadabra::multiplier_t multiplier_t;

bool linear::gaussian_elimination_inplace(std::vector<std::vector<multiplier_t> >& a, 
														std::vector<multiplier_t>& b)
	{
	assert(a.size() == b.size());

	// If there are more equations than unknowns, we first reduce the form to
	//  xxx = x
	//   xx = x
   //    x = x
   //  000 = 0
   //  000 = 0

	unsigned int number_of_eqs = a.size();
	unsigned int number_of_unk = a[0].size();
	unsigned int mineu=std::min(number_of_eqs, number_of_unk);

	// Loop over rows, creating upper-triangular matrix 'a'
	for(unsigned row=0; row<mineu; ++row) {
		multiplier_t pivot = a[row][row];
		if(pivot == 0) {
			unsigned int nrow;
			for(nrow=row+1; nrow<number_of_eqs; ++nrow)
				if((pivot = a[nrow][row]) != 0) 
					break;
			if(pivot == 0) 
				return true; // undetermined system FIXME: still minimalise
			std::swap(a[nrow],a[row]);
			std::swap(b[nrow],b[row]);
			}
		
      // Gaussian elimination of column
		for(unsigned int nrow=row+1; nrow<number_of_eqs; ++nrow) {
			multiplier_t tmp = a[nrow][row]/pivot;
			a[nrow][row]=0;
			for(unsigned int col=row+1; col<number_of_unk; ++col)
				a[nrow][col] -= tmp*a[row][col];
			b[nrow] -= tmp*b[row];
			}
		}
//	for(unsigned int i=0; i<a.size(); ++i) {
//		for(unsigned int j=0; j<a[i].size(); ++j)
//			txtout << a[i][j] << " ";
//		txtout <<  " = " << b[i] << std::endl;
//		}

	// Check that there are no inconsistencies
	for(unsigned int i=number_of_unk; i<number_of_eqs; ++i)
		if(b[i]!=0)
			return false;

//	txtout << "consistent" << std::endl;

   // Back substitution
	for(int row=mineu-1; row>=0; --row) {
		assert(a[row][row]!=0);
		for(int col=mineu-1; col>row; --col) {
			b[row]      -= a[row][col]*b[col];
			for(unsigned allcol=col; allcol<number_of_unk; ++allcol)
				a[row][allcol] -= a[row][allcol]*a[row][row];
			}
		assert(a[row][row]!=0);
		b[row]      /= a[row][row];
		a[row][row]  = (a[row][row]!=0?1:0);
		}
	return true;
	}
