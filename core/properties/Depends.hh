
#include "properties/DependsBase.hh"

class Depends : public DependsBase, virtual public property {
	public:
		virtual std::string name() const override;
		virtual bool parse(const Properties&, keyval_t&) override;
		virtual exptree dependencies(exptree::iterator) const;
		virtual std::string unnamed_argument() const { return "dependants"; };
	private:
		exptree dependencies_;
};
