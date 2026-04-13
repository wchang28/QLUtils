#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/indexes/term-ois.hpp>

namespace QuantLib {
    class USDTermSofr : public TermOISIndex {
    public:
        USDTermSofr(
            Frequency frequency,    // Annual (12M), Semiannual (6M), Quarterly (3M), Monthly (1M)
            const Handle<YieldTermStructure>& h = {}       // index estimating term structure
        )
        :TermOISIndex(
            "USDTermSofr",  // familyName
            frequency,
            2,  // T+2 settlement on the fixing calendar
            USDCurrency(),  // currency
            UnitedStates(UnitedStates::GovernmentBond), // fixingCalendar
            Actual360(),    // dayCounter
            h   // index estimating term structure
        )
        {}
    };
	template <
        Frequency FREQ
    >
    class TermSofr : public USDTermSofr {
    public:
        TermSofr(
            const Handle<YieldTermStructure>& h = {}    // index estimating term structure
        ) : USDTermSofr(FREQ, h)
        {}
    };
    using TermSofr1M = TermSofr<Monthly>;
    using TermSofr3M = TermSofr<Quarterly>;
    using TermSofr6M = TermSofr<Semiannual>;
    using TermSofr12M = TermSofr<Annual>;
}
