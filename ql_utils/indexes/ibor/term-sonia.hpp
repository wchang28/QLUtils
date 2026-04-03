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
    class TermSonia1M : public GBPTermSonia {
    public:
        TermSonia1M(
            const Handle<YieldTermStructure>& h = {}    // index estimating term structure
        ) : GBPTermSonia(1, h)
        {}
    };
    class TermSonia3M : public GBPTermSonia {
    public:
        TermSonia3M(
            const Handle<YieldTermStructure>& h = {}    // index estimating term structure
        ) : GBPTermSonia(3, h)
        {}
    };
    class TermSonia6M : public GBPTermSonia {
    public:
        TermSonia6M(
            const Handle<YieldTermStructure>& h = {}    // index estimating term structure
        ) : GBPTermSonia(6, h)
        {}
    };
    class TermSonia12M : public GBPTermSonia {
    public:
        TermSonia12M(
            const Handle<YieldTermStructure>& h = {}    // index estimating term structure
        ) : GBPTermSonia(12, h)
        {}
    };
}
