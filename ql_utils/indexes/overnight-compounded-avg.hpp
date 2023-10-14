#pragma once

#include <ql/quantlib.hpp>
#include <string>
#include <sstream>

namespace QuantLib {
    // overnight compounded average in arrears index of 1Yr tenor
    // this makes forward OIS swap (annual cf exchange) priced like a vanilla swap
    // this class represents the ibor floating leg (paid annual) of a forward OIS swap
    template
    <
        typename OVERNIGHTINDEX // OvernightIndex can be Sofr, FedFunds, Sonia, Estr, Eonia
    >
    class OvernightCompoundedAverageInArrearsIndex : public IborIndex {
    public:
        typedef typename OVERNIGHTINDEX OvernightIndex;
    private:
        static std::string makeFamilyName(
            Natural fixingDays
        ) {
            std::ostringstream oss;
            oss << "OvernightCompoundedAverageInArrearsIndex<<";
            oss << OvernightIndex().name();
            oss << ">> (T+" << fixingDays << ")";
            return oss.str();
        }
    public:
        OvernightCompoundedAverageInArrearsIndex(
            Natural fixingDays, // fixing days of the index, this usually matches with the settlement days of the vanilla swap that has this index on its floating leg
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = Handle<YieldTermStructure>()
        ) :
            IborIndex
            (
                makeFamilyName(fixingDays),	// fmailyName
                1 * Years,  // tenor of this index is one year. This is because OIS swap's cf exchange is always annual
                fixingDays,  // fixingDays
                OvernightIndex().currency(),    // currency = overnight index's currency
                OvernightIndex().fixingCalendar(),	// fixingCalendar = overnight index's fixing calendar
                OvernightIndex().businessDayConvention(),	// convention = overnight index's business convention
                OvernightIndex().endOfMonth(),   // endOfMonth = overnight index's end of month
                OvernightIndex().dayCounter(),	// dayCounter = overnight index's day counter
                indexEstimatingTermStructure	// index estimating term structure
            )
        {}
    };
}