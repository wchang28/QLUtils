#pragma once

#include <ql/quantlib.hpp>
#include <string>
#include <sstream>
#include <ql_utils/indexes/ois-swap-index.hpp>
#include <ql_utils/indexes/ibor/term-sofr.hpp>

namespace QuantLib {
    template <
        Natural COUPON_TENOR_MONTHS // coupon/cashflow tenor in months, usually 1, 3, 6, 12
    >
    class UsdTermSofrSwapIsdaFix : public SwapIndexEx {
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
            SwapIndexEx(
                makeFamilyName(), // familyName
                tenor,	// tenor
                2, // settlementDays
                USDCurrency(),	// curency
                UnitedStates(UnitedStates::GovernmentBond),	// fixingCalendar for both legs
                COUPON_TENOR_MONTHS * Months, // fixedLegTenor
                ModifiedFollowing,	// fixedLegConvention
                USDTermSofr{ COUPON_TENOR_MONTHS }.dayCounter(),	// fixedLegDayCounter, same as index's day counter
                ext::shared_ptr<IborIndex>(new USDTermSofr(COUPON_TENOR_MONTHS, h)),	// iborIndex
                true    // endOfMonth, term sofr swap has endOfMonth=true for both legs
            )
        {}
        UsdTermSofrSwapIsdaFix(
            const Period& tenor,	// swap tenor
            const Handle<YieldTermStructure>& forwarding,	// index estimating term structure
            const Handle<YieldTermStructure>& discounting // exogenous discount term structure
        ) :
            SwapIndexEx(
                makeFamilyName(), // familyName
                tenor,	// tenor
                2, // settlementDays
                USDCurrency(),	// curency
                UnitedStates(UnitedStates::GovernmentBond),	// fixingCalendar for both legs
                COUPON_TENOR_MONTHS * Months, // fixedLegTenor
                ModifiedFollowing,	// fixedLegConvention
                USDTermSofr{ COUPON_TENOR_MONTHS }.dayCounter(),	// fixedLegDayCounter, same as index's day counter
                ext::shared_ptr<IborIndex>(new USDTermSofr(COUPON_TENOR_MONTHS, forwarding)),	// iborIndex
                discounting, // discountingTermStructure
                true    // endOfMonth, term sofr swap has endOfMonth=true for both legs
            )
        {}
    };
}
