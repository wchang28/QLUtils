#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/indexes/term-ois.hpp>

namespace QuantLib {
    class GBPTermSonia : public TermOISIndex {
    public:
        GBPTermSonia(
            Frequency frequency,    // Annual (12M), Semiannual (6M), Quarterly (3M), Monthly (1M)
            const Handle<YieldTermStructure>& h = {}   // index estimating term structure
        )
        :TermOISIndex(
            "GBPTermSonia", // familyName
            frequency,
            0,	// T+0 settlement on the fixing calendar
            GBPCurrency(),  // currency
            UnitedKingdom(UnitedKingdom::Exchange),	// fixingCalendar
            Actual365Fixed(),   // // dayCounter
            h   // index estimating term structure
        )
        {}
    };
    template <
        Frequency FREQ
    >
    class TermSonia : public GBPTermSonia {
    public:
        TermSonia(
            const Handle<YieldTermStructure>& h = {}    // index estimating term structure
        ) : GBPTermSonia(FREQ, h)
        {}
    };
    using TermSonia1M = TermSonia<Monthly>;
    using TermSonia3M = TermSonia<Quarterly>;
    using TermSonia6M = TermSonia<Semiannual>;
    using TermSonia12M = TermSonia<Annual>;
}
