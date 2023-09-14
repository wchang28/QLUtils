#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/indexes/term-ois.hpp>

namespace QuantLib {
	// Term Sofr, tenorMonths = 1, 3, 6, 12
	class USDTermSofr : public TermOISIndex {
	public:
		USDTermSofr(
			Natural tenorMonths,
			const Handle<YieldTermStructure>& h = {}
		)
		:TermOISIndex(
			"USDTermSofr",
			tenorMonths,
			2,		// T+2 settlement, confirmed
			USDCurrency(),
			UnitedStates(UnitedStates::GovernmentBond),	// fixingCalendar, confirmed
			Actual360(),	// dayCounter
			h
		)
		{}
	};
}