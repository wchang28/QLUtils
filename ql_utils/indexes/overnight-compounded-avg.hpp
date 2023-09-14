#pragma once

#include <ql/quantlib.hpp>
#include <string>

namespace QuantLib {
    // overnight compounded average in arrears index of 1Yr tenor
    // this make forward OIS swap (annual cf excahnge) priced like a vanilla swap
    // this class represent the floating leg (paid annual) index of a forward OIS swap
    template
    <
        typename OVERNIGHTINDEX // OvernightIndex can be Sofr, FedFunds, Sonia, Estr, Eonia
    >
    class OvernightCompoundedAverageInArrearsIndex : public IborIndex {
    public:
        typedef typename OVERNIGHTINDEX OvernightIndex;
    public:
        OvernightCompoundedAverageInArrearsIndex(
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = Handle<YieldTermStructure>()
        ) :
            IborIndex
            (
                std::string("OvernightCompoundedAverageInArrearsIndex<<") + OvernightIndex().name() + ">>",
                1 * Years,  // tenor is one year/ annual
                OvernightIndex().fixingDays(),  // fixingDays = overnight index's fixingDays which is usually 0
                OvernightIndex().currency(),    // currency =  overnight index's currency
                OvernightIndex().fixingCalendar(),	// fixingCalendar =  overnight index's fixing calendar
                OvernightIndex().businessDayConvention(),	// convention = overnight index's business convention
                OvernightIndex().endOfMonth(),   // endOfMonth = overnight index's end of month
                OvernightIndex().dayCounter(),	// dayCounter = overnight index's day counter
                indexEstimatingTermStructure
            )
        {}
    };
}