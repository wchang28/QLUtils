#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/indexes/term-ois.hpp>

namespace QuantLib {
	// Term Sonia, tenorMonths = 1, 3, 6, 12
	class GBPTermSonia : public TermOISIndex {
	public:
		GBPTermSonia(
			Natural tenorMonths,
			const Handle<YieldTermStructure>& h = {}
		)
		:TermOISIndex(
			"GBPTermSonia",
			tenorMonths,
			0,	// T+0 settlement, confirmed
			GBPCurrency(),
			UnitedKingdom(UnitedKingdom::Exchange),	// fixingCalendar, confirmed
			Actual365Fixed(),
			h
		)
		{}
	};
}