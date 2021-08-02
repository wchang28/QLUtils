#pragma once
#include <vector>
#include <ql/quantlib.hpp>

namespace QLUtils {
    // type alias definitions
    using pQuote = QuantLib::ext::shared_ptr<QuantLib::Quote>;
    using pIndex = QuantLib::ext::shared_ptr<QuantLib::IborIndex>;
    using pHelper = QuantLib::ext::shared_ptr<QuantLib::RateHelper>;
    //
    // T = traits, I = interpolation
    template<typename T, typename I>
    class PiecewiseCurveBuilder
    {
    private:
        std::vector<pHelper> rateHelpers;
    public:
        PiecewiseCurveBuilder() {}
        const std::vector<pHelper>& helpers() const {return rateHelpers;}
        void AddDeposit(const pQuote& quote, const pIndex& index) {
            pHelper rateHelper(new QuantLib::DepositRateHelper(QuantLib::Handle<QuantLib::Quote>(quote), index));
            rateHelpers.push_back(rateHelper);
        }
        void AddFRA(const pQuote& quote, const QuantLib::Period& periodLengthToStart, const pIndex& index) {
            pHelper rateHelper(new QuantLib::FraRateHelper(QuantLib::Handle<QuantLib::Quote>(quote), periodLengthToStart, pIndex(index)));
            rateHelpers.push_back(rateHelper);        
        }
        void AddSwap(
            const pQuote& quote,
            QuantLib::Natural settlementDays,
            const QuantLib::Period& tenor,
            const QuantLib::Calendar& fixedCalendar,
            QuantLib::Frequency fixedFrequency,
            QuantLib::BusinessDayConvention fixedConvention,
            const QuantLib::DayCounter& fixedDayCount,
            const pIndex& floatIndex,
            QuantLib::Handle<QuantLib::YieldTermStructure> discountingCurve = QuantLib::Handle<QuantLib::YieldTermStructure>()  // exogenous discounting curve
        ) {
            pHelper rateHelper(new QuantLib::SwapRateHelper(QuantLib::Handle<QuantLib::Quote>(quote), tenor, fixedCalendar, fixedFrequency, fixedConvention, fixedDayCount, floatIndex, QuantLib::Handle<QuantLib::Quote>(), 0 * QuantLib::Days, discountingCurve, settlementDays));
            rateHelpers.push_back(rateHelper);       
        }
        void AddSwap(
            const pQuote& quote,
            const QuantLib::ext::shared_ptr<QuantLib::SwapIndex>& swapIndex,
            QuantLib::Handle<QuantLib::YieldTermStructure> discountingCurve = QuantLib::Handle<QuantLib::YieldTermStructure>()  // exogenous discounting curve
        ) {
            pHelper rateHelper(new QuantLib::SwapRateHelper(QuantLib::Handle<QuantLib::Quote>(quote), swapIndex, QuantLib::Handle<QuantLib::Quote>(), 0 * QuantLib::Days, discountingCurve));
            rateHelpers.push_back(rateHelper);
        }
        void AddOIS(
            const pQuote& quote,
            QuantLib::Natural settlementDays,
            const QuantLib::Period& tenor,
            const QuantLib::ext::shared_ptr<QuantLib::OvernightIndex>& overnightIndex,
            bool telescopicValueDates,
            QuantLib::Natural paymentLag,
            QuantLib::BusinessDayConvention paymentConvention = QuantLib::ModifiedFollowing
        ) {
            pHelper rateHelper(new QuantLib::OISRateHelper(settlementDays, tenor, QuantLib::Handle<QuantLib::Quote>(quote), overnightIndex, QuantLib::Handle<QuantLib::YieldTermStructure>(), telescopicValueDates, paymentLag, paymentConvention, QuantLib::Annual));
            rateHelpers.push_back(rateHelper);       
        }
        void AddFuture(const pQuote& quote, const QuantLib::Date& IMMDate, int lengthInMonths, const QuantLib::Calendar& calendar,
            QuantLib::BusinessDayConvention convention, const QuantLib::DayCounter& dayCounter, bool endOfMonth = true) {
            pHelper rateHelper(new QuantLib::FuturesRateHelper(QuantLib::Handle<QuantLib::Quote>(quote), IMMDate, lengthInMonths, calendar, convention, endOfMonth, dayCounter));
            rateHelpers.push_back(rateHelper);        
        }
        // T = traits, I = interpolation
        QuantLib::ext::shared_ptr<QuantLib::PiecewiseYieldCurve<T, I>> GetCurve(const QuantLib::Date& curveReferenceDate, const QuantLib::DayCounter& dayCounter, const I& interp = I()) {
            QuantLib::ext::shared_ptr<QuantLib::PiecewiseYieldCurve<T, I>> pTS(new QuantLib::PiecewiseYieldCurve<T, I>(curveReferenceDate, rateHelpers, dayCounter, interp));
			pTS->discount(0);	// trigger the bootstrap
            return pTS;
        }
    };
}