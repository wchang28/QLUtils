#pragma once

#include <ql/quantlib.hpp>

namespace QLUtils {
	class TermOISIndex: public QuantLib::IborIndex {
	public:
		TermOISIndex(
			const std::string& familyName,
			const QuantLib::Period& tenor,
			QuantLib::Natural settlementDays,
			const QuantLib::Currency& currency,
			const QuantLib::Calendar& fixingCalendar,
			const QuantLib::DayCounter& dayCounter,
			const QuantLib::Handle<QuantLib::YieldTermStructure>& h = {}
		) : QuantLib::IborIndex(familyName, tenor, settlementDays, currency, fixingCalendar, QuantLib::ModifiedFollowing, true, dayCounter, h)
		{
			QL_REQUIRE(this->tenor().units() == QuantLib::Months || this->tenor().units() == QuantLib::Years, "the tenor unit must be either in months or years");
		}
	};
}