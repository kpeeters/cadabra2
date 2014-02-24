
#pragma once


class ImplicitIndex : virtual public property {
	public:
		virtual bool parse(exptree&, exptree::iterator, exptree::iterator, keyval_t&);
		virtual std::string name() const;
		virtual std::string unnamed_argument() const { return "name"; };
		virtual void display(std::ostream& str) const;

		std::vector<std::string> set_names;
};

