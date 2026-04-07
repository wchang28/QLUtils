#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/indexes/term-ois.hpp>

namespace QuantLib {
    // Term Sofr, tenorMonths = 1, 3, 6, 12
    class USDTermSofr : public TermOISIndex {
    public:
        USDTermSofr(
            Natural tenorMonths,    // tenor in months
            const Handle<YieldTermStructure>& h = {}       // index estimating term structure
        )
        :TermOISIndex(
            "USDTermSofr",  // familyName
            tenorMonths,
            2,  // T+2 settlement on the fixing calendar
            USDCurrency(),  // currency
            UnitedStates(UnitedStates::GovernmentBond), // fixingCalendar
            Actual360(),    // dayCounter
            h   // index estimating term structure
        )
        {}
    };
	template <
        Natural TenorMonths
    >
    class TermSofr : public USDTermSofr {
    public:
        TermSofr(
            const Handle<YieldTermStructure>& h = {}    // index estimating term structure
        ) : USDTermSofr(TenorMonths, h)
        {}
    };
    using TermSofr1M = TermSofr<1>;
    using TermSofr3M = TermSofr<3>;
    using TermSofr6M = TermSofr<6>;
    using TermSofr12M = TermSofr<12>;
}
