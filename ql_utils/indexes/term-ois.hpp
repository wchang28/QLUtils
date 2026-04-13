#pragma once

#include <ql/quantlib.hpp>
#include <string>
#include <ql_utils/utilities/time.hpp>

namespace QuantLib {
    // forward-looking term index similar to the xLibor but in the risk-free OIS space
    class TermOISIndex: public IborIndex {
    public:
        TermOISIndex(
            const std::string& familyName,
            Frequency frequency,    // Annual (12M), Semiannual (6M), Quarterly (3M), Monthly (1M)
            Natural settlementDays,
            const Currency& currency,
            const Calendar& fixingCalendar,
            const DayCounter& dayCounter,
            const Handle<YieldTermStructure>& h = {}   // index estimating term structure
        ) :
            IborIndex
            (
                familyName,
                Period(frequency),    // tenor
                settlementDays,
                currency,
                fixingCalendar,
                ModifiedFollowing,  // convention - value should mimic xLibor with tenor greater than or equal to a month. In this case, convention is ModifiedFollowing
                true,               // endOfMonth - value should mimic xLibor with tenor greater than or equal to a month. In this case, endOfMonth is true
                dayCounter,
                h
            )
        {
            QL_REQUIRE(tenor() <= Period(1, Years), "Term OIS index tenor (" << tenor() << ") cannot be longer than one year");
            auto [multipile, n] = Utils::isMultiple(Period(1, Years), tenor());
            QL_REQUIRE(multipile, "Invalid term OIS index tenor (" << tenor() << "). One year must be the multiple of it");
        }
    };
}
