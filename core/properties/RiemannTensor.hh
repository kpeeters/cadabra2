
#include "properties/TableauSymmetry.hh"

class RiemannTensor : public TableauSymmetry, virtual public property {
	public:
		RiemannTensor();
		virtual std::string name() const override;
		virtual void        validate(const Kernel&, const Ex&) const override;
};
