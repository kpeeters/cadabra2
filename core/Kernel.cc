#include "Kernel.hh"
#include "PreClean.hh"
#include "Cleanup.hh"
#include "Parser.hh"
#include "Config.hh"

// Default properties
#include "properties/Distributable.hh"
#include "properties/IndexInherit.hh"
#include "properties/TableauInherit.hh"
#include "properties/CommutingAsProduct.hh"
#include "properties/DependsInherit.hh"
#include "properties/NumericalFlat.hh"
#include "properties/WeightInherit.hh"
#include "properties/CommutingAsSum.hh"
#include "properties/Derivative.hh"
#include "properties/Accent.hh"
#include "properties/Tableau.hh"
#include "properties/FilledTableau.hh"

using namespace cadabra;

const std::string Kernel::version = CADABRA_VERSION_FULL;
const std::string Kernel::build   = CADABRA_VERSION_BUILD;

Kernel::Kernel(bool inject_defaults)
	: scalar_backend(scalar_backend_t::sympy)
	, call_embedded_python_functions(false)
	, warning_level(warn_t::warning)
	, warning_callback(nullptr)
	, display_fractions(false)
	{
	if (inject_defaults) {
		inject_property(new Distributable(),          ex_from_string("\\prod{#}"), 0);
		inject_property(new IndexInherit(),           ex_from_string("\\prod{#}"), 0);
		inject_property(new TableauInherit(),         ex_from_string("\\prod{#}"), 0);
		inject_property(new CommutingAsProduct(),     ex_from_string("\\prod{#}"), 0);
		inject_property(new DependsInherit(),         ex_from_string("\\prod{#}"), 0);
		inject_property(new NumericalFlat(),          ex_from_string("\\prod{#}"), 0);
		inject_property(new Inherit<Tableau>(),       ex_from_string("\\prod{#}"), 0);
		inject_property(new Inherit<FilledTableau>(), ex_from_string("\\prod{#}"), 0);

		auto wi2 = new WeightInherit();
		wi2->combination_type = WeightInherit::multiplicative;
		auto wa2 = ex_from_string("label=all, type=multiplicative");
		inject_property(wi2,                          ex_from_string("\\prod{#}"), wa2);

		inject_property(new IndexInherit(),           ex_from_string("\\frac{#}"), 0);
		inject_property(new DependsInherit(),         ex_from_string("\\frac{#}"), 0);

		inject_property(new Distributable(),          ex_from_string("\\wedge{#}"), 0);
		inject_property(new IndexInherit(),           ex_from_string("\\wedge{#}"), 0);

		inject_property(new DependsInherit(),         ex_from_string("\\wedge{#}"), 0);
		inject_property(new NumericalFlat(),          ex_from_string("\\wedge{#}"), 0);
		auto wi4 = new WeightInherit();
		wi4->combination_type = WeightInherit::multiplicative;
		auto wa4 = ex_from_string("label=all, type=multiplicative");
		inject_property(wi4,                          ex_from_string("\\wedge{#}"), wa4);

		inject_property(new IndexInherit(),           ex_from_string("\\sum{#}"), 0);
		inject_property(new Inherit<Tableau>(),       ex_from_string("\\sum{#}"), 0);
		inject_property(new Inherit<FilledTableau>(), ex_from_string("\\sum{#}"), 0);
		inject_property(new CommutingAsSum(),         ex_from_string("\\sum{#}"), 0);
		inject_property(new DependsInherit(),         ex_from_string("\\sum{#}"), 0);
		auto wi = new WeightInherit();
		auto wa = ex_from_string("label=all, type=additive");
		inject_property(wi,                           ex_from_string("\\sum{#}"), wa);

		auto d = new Derivative();
		d->hidden(true);
		inject_property(d, ex_from_string("\\cdbDerivative{#}"), 0);

		inject_property(new Derivative(), ex_from_string("\\commutator{#}"), 0);
		inject_property(new IndexInherit(), ex_from_string("\\commutator{#}"), 0);

		inject_property(new Derivative(), ex_from_string("\\anticommutator{#}"), 0);
		inject_property(new IndexInherit(), ex_from_string("\\anticommutator{#}"), 0);

		inject_property(new Distributable(), ex_from_string("\\indexbracket{#}"), 0);
		inject_property(new IndexInherit(), ex_from_string("\\indexbracket{#}"), 0);

		inject_property(new DependsInherit(), ex_from_string("\\pow{#}"), 0);
		auto wi3 = new WeightInherit();
		auto wa3 = ex_from_string("label=all, type=power");
		inject_property(wi3, ex_from_string("\\pow{#}"), wa3);

		inject_property(new NumericalFlat(), ex_from_string("\\int{#}"), 0);
		inject_property(new IndexInherit(), ex_from_string("\\int{#}"), 0);

		// Hidden nodes.
		inject_property(new Accent(), ex_from_string("\\ldots{#}"), 0);

		// Accents, necessary for proper display.
		inject_property(new Accent(), ex_from_string("\\hat{#}"), 0);
		inject_property(new Accent(), ex_from_string("\\bar{#}"), 0);
		inject_property(new Accent(), ex_from_string("\\overline{#}"), 0);
		inject_property(new Accent(), ex_from_string("\\tilde{#}"), 0);
		}
	}

Kernel::~Kernel()
	{
	//	std::cerr << "~Kernel() " << this << std::endl;
	}

void Kernel::inject_property(property *prop, std::shared_ptr<Ex> ex, std::shared_ptr<Ex> param)
	{
	Ex::iterator it=ex->begin();

	if(param) {
		// std::cerr << "property with " << *param << std::endl;
		keyval_t keyvals;
		prop->parse_to_keyvals(*param, keyvals);
		prop->parse(*this, ex, keyvals);
		}
	prop->validate(*this, Ex(it));
	properties.master_insert(Ex(it), prop);
	}

std::shared_ptr<cadabra::Ex> Kernel::ex_from_string(const std::string& s)
	{
	auto ex = std::make_shared<cadabra::Ex>();
	cadabra::Parser parser1(ex, s);

	pre_clean_dispatch_deep(*this, *ex);
	cleanup_dispatch_deep(*this, *ex);
	check_index_consistency(*this, *ex, ex->begin());

	return ex;
	}

void Kernel::warn(const std::string& msg, Kernel::warn_t level) const
	{
	if (warning_callback && warning_level != warn_t::notset && level > warning_level)
		warning_callback(msg);
	}
