#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/indexes/term-ois.hpp>

namespace QLUtils {
	class GBPTermSonia : public TermOISIndex {
	public:
		GBPTermSonia(
			QuantLib::Natural tenorMonths,
			const QuantLib::Handle<QuantLib::YieldTermStructure>& h = {}
		)
		:TermOISIndex(
				"GBPTermSonia",
				QuantLib::Period(tenorMonths, QuantLib::Months),
				0,
				QuantLib::GBPCurrency(),
				QuantLib::UnitedKingdom(QuantLib::UnitedKingdom::Exchange),	// confirmed
				QuantLib::Actual365Fixed(),
				h
			)
		{}
	};
}