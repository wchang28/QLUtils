#pragma once

#include <ql/quantlib.hpp>
#include <string>

namespace QuantLib {
	class TermOISIndex: public IborIndex {
	public:
		TermOISIndex(
			const std::string& familyName,
			Natural tenorMonths,
			Natural settlementDays,
			const Currency& currency,
			const Calendar& fixingCalendar,
			const DayCounter& dayCounter,
			const Handle<YieldTermStructure>& h = {}
		) :
			IborIndex
			(
				familyName,
				Period(tenorMonths, Months),	// tenor
				settlementDays,
				currency,
				fixingCalendar,
				ModifiedFollowing,	// convention - confirmed
				true,				// endOfMonth - confirmed
				dayCounter,
				h
			)
		{}
	};
}