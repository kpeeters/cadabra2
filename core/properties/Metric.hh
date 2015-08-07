
#include "Props.hh"
#include "properties/TableauSymmetry.hh"

class Metric : public TableauSymmetry, virtual public property {
	public:
		Metric();
		virtual std::string name() const override;
		virtual bool        parse(const Properties&, keyval_t&) override;
		virtual void        validate(const Properties&, const Ex&) const override;

		int signature;
};
