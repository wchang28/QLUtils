#pragma once

#include <ql/quantlib.hpp>
#include <string>
#include <sstream>
#include <ql_utils/indexes/ibor/term-sofr.hpp>

namespace QuantLib {
    template <
        Natural COUPON_TENOR_MONTHS
    >
    class UsdTermSofrSwapIsdaFix : public SwapIndex {
    private:
        static std::string makeFamilyName() {
            std::ostringstream oss;
            oss << "UsdTermSofrSwapIsdaFix<<" << COUPON_TENOR_MONTHS << "M>>";
            return oss.str();
        }
    public:
        UsdTermSofrSwapIsdaFix(
            const Period& tenor,	// swap tenor
            const Handle<YieldTermStructure>& h = {}	// index estimating term structure
        ) :
            SwapIndex(
                makeFamilyName(), // familyName
                tenor,	// tenor
                2, // settlementDays
                USDCurrency(),	// curency
                UnitedStates(UnitedStates::GovernmentBond),	// fixingCalendar for both legs
                COUPON_TENOR_MONTHS * Months, // fixedLegTenor
                ModifiedFollowing,	// fixedLegConvention
                USDTermSofr{ COUPON_TENOR_MONTHS }.dayCounter(),	// fixedLegDayCounter, same as index's day counter
                ext::shared_ptr<IborIndex>(new USDTermSofr(COUPON_TENOR_MONTHS, h))	// iborIndex
            )
        {}
    };
}
