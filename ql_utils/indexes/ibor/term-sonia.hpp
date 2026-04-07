#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/indexes/term-ois.hpp>

namespace QuantLib {
    // Term Sonia, tenorMonths = 1, 3, 6, 12
    class GBPTermSonia : public TermOISIndex {
    public:
        GBPTermSonia(
            Natural tenorMonths,    // tenor in months
            const Handle<YieldTermStructure>& h = {}   // index estimating term structure
        )
        :TermOISIndex(
            "GBPTermSonia", // familyName
            tenorMonths,
            0,	// T+0 settlement on the fixing calendar
            GBPCurrency(),  // currency
            UnitedKingdom(UnitedKingdom::Exchange),	// fixingCalendar
            Actual365Fixed(),   // // dayCounter
            h   // index estimating term structure
        )
        {}
    };
    template <
        Natural TenorMonths
    >
    class TermSonia : public GBPTermSonia {
    public:
        TermSonia(
            const Handle<YieldTermStructure>& h = {}    // index estimating term structure
        ) : GBPTermSonia(TenorMonths, h)
        {}
    };
    using TermSonia1M = TermSonia<1>;
    using TermSonia3M = TermSonia<3>;
    using TermSonia6M = TermSonia<6>;
    using TermSonia12M = TermSonia<12>;
}
