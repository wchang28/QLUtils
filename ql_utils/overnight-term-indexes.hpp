#pragma once
#include <ql/quantlib.hpp>
#include <string>

namespace QuantLib {
    // overnight compounded average in arrears index
    template
    <
        typename OVERNIGHTINDEX // OvernightIndex can be Sofr, FedFunds, Sonia, Estr, Eonia
    >
    class OvernightCompoundedAverageInArrearsIndex: public IborIndex {
    public:
        typedef typename OVERNIGHTINDEX OvernightIndex;
    public:
        OvernightCompoundedAverageInArrearsIndex(
            Integer tenorInMonth,   // usually 1m, 3m, 6m, 12m
            const Handle<YieldTermStructure>& h = Handle<YieldTermStructure>()
        ) :
            IborIndex
            (
                std::string("OvernightCompoundedAverageInArrearsIndex<<") + OvernightIndex().name() + ">>",
                tenorInMonth * Months,
                OvernightIndex().fixingDays(),
                OvernightIndex().currency(),
                OvernightIndex().fixingCalendar(),
                ModifiedFollowing,
                true,   // endOfMonth is always true
                OvernightIndex().dayCounter(),
                h
            )
        {}
    };
}