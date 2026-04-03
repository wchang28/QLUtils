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
    class TermSofr1M : public USDTermSofr {
    public:
        TermSofr1M(
            const Handle<YieldTermStructure>& h = {}    // index estimating term structure
        ) : USDTermSofr(1, h)
        {}
    };
    class TermSofr3M : public USDTermSofr {
    public:
        TermSofr3M(
            const Handle<YieldTermStructure>& h = {}    // index estimating term structure
        ) : USDTermSofr(3, h)
        {}
    };
    class TermSofr6M : public USDTermSofr {
    public:
        TermSofr6M(
            const Handle<YieldTermStructure>& h = {}    // index estimating term structure
        ) : USDTermSofr(6, h)
        {}
    };
    class TermSofr12M : public USDTermSofr {
    public:
        TermSofr12M(
            const Handle<YieldTermStructure>& h = {}    // index estimating term structure
        ) : USDTermSofr(12, h)
        {}
    };
}
