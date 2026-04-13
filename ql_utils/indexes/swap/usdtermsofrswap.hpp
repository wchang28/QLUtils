#pragma once

#include <ql/quantlib.hpp>
#include <string>
#include <sstream>
#include <ql_utils/indexes/ois-swap-index.hpp>
#include <ql_utils/indexes/ibor/term-sofr.hpp>

namespace QuantLib {
    template <
        Frequency FREQ    // Annual (12M), Semiannual (6M), Quarterly (3M), Monthly (1M)
    >
    class UsdTermSofrSwapIsdaFix : public SwapIndexEx {
    private:
        static std::string makeFamilyName() {
            std::ostringstream oss;
            oss << "UsdTermSofrSwapIsdaFix<<" << Period(FREQ) << ">>";
            return oss.str();
        }
    public:
		typedef TermSofr<FREQ> IBOR_INDEX;
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
                Period(FREQ), // fixedLegTenor
                ModifiedFollowing,	// fixedLegConvention
                IBOR_INDEX{}.dayCounter(),	// fixedLegDayCounter, same as index's day counter
                ext::shared_ptr<IborIndex>(new IBOR_INDEX(h)),	// iborIndex
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
                Period(FREQ), // fixedLegTenor
                ModifiedFollowing,	// fixedLegConvention
                IBOR_INDEX{}.dayCounter(),	// fixedLegDayCounter, same as index's day counter
                ext::shared_ptr<IborIndex>(new IBOR_INDEX(forwarding)),	// iborIndex
                discounting, // discountingTermStructure
                true    // endOfMonth, term sofr swap has endOfMonth=true for both legs
            )
        {}
    };
}
