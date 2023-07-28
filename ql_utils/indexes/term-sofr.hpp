#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/indexes/term-ois.hpp>

namespace QLUtils {
	class USDTermSofr: public TermOISIndex {
	public:
		USDTermSofr(
			QuantLib::Natural tenorMonths,
			const QuantLib::Handle<QuantLib::YieldTermStructure>& h = {}
		)
		:TermOISIndex(
			"USDTermSofr",
			QuantLib::Period(tenorMonths, QuantLib::Months),
			2,		// confirmed
			QuantLib::USDCurrency(),
			QuantLib::UnitedStates(QuantLib::UnitedStates::GovernmentBond),	// confirmed
			QuantLib::Actual360(),
			h
		)
		{}
	};
}