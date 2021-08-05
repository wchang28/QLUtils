#pragma once
#include <vector>
#include <ql/quantlib.hpp>

#ifdef QL_OVERNIGHT_INDEXED_SWAP_INDEX_MISSING_IMPL
namespace QuantLib {
    class OvernightIndexedSwapIndexEx : public OvernightIndexedSwapIndex {
    public:
        OvernightIndexedSwapIndexEx(
            const std::string& familyName,
            const Period& tenor,
            Natural settlementDays,
            const Currency& currency,
            const ext::shared_ptr<OvernightIndex>& overnightIndex,
            bool telescopicValueDates = false,
            RateAveraging::Type averagingMethod = RateAveraging::Compound
        ) : OvernightIndexedSwapIndex(familyName, tenor, settlementDays, currency, overnightIndex, telescopicValueDates, averagingMethod)
        {}
        // missing implementations from QuantLib
        const bool& telescopicValueDates() const { return telescopicValueDates_; }
        const RateAveraging::Type& averagingMethod() const { return averagingMethod_; }
    };
}
#endif

namespace QLUtils {
    // the main curve bootstrapper
    // T = traits, I = interpolation
    template<typename T, typename I>
    class PiecewiseCurveBuilder {
    public:
        typedef QuantLib::ext::shared_ptr<QuantLib::Quote> pQuote;
        typedef QuantLib::ext::shared_ptr<QuantLib::IborIndex> pIborIndex;
        typedef QuantLib::ext::shared_ptr<QuantLib::OvernightIndex> pOvernightIndex;
        typedef QuantLib::ext::shared_ptr<QuantLib::SwapIndex> pSwapIndex;
#ifdef QL_OVERNIGHT_INDEXED_SWAP_INDEX_MISSING_IMPL
        typedef QuantLib::ext::shared_ptr<QuantLib::OvernightIndexedSwapIndexEx> pOvernightIndexedSwapIndex;
#else
        typedef QuantLib::ext::shared_ptr<QuantLib::OvernightIndexedSwapIndex> pOvernightIndexedSwapIndex;
#endif
        typedef QuantLib::ext::shared_ptr<QuantLib::RateHelper> pRateHelper;
        typedef QuantLib::Handle<QuantLib::Quote> QuoteHandle;
        typedef QuantLib::Handle<QuantLib::YieldTermStructure> YieldTermStructureHandle;
    protected:
        std::vector<pRateHelper> rateHelpers;
    public:
        PiecewiseCurveBuilder() {}
        QuantLib::ext::shared_ptr<QuantLib::DepositRateHelper> AddDeposit(
            const pQuote& quote,
            const pIborIndex& iborIndex
        ) {
            QuantLib::ext::shared_ptr<QuantLib::DepositRateHelper> rateHelper(new QuantLib::DepositRateHelper(QuoteHandle(quote), iborIndex));
            rateHelpers.push_back(rateHelper);
            return rateHelper;
        }
        QuantLib::ext::shared_ptr<QuantLib::FraRateHelper> AddFRA(
            const pQuote& quote,
            const QuantLib::Period& periodLengthToStart,
            const pIborIndex& iborIndex
        ) {
            QuantLib::ext::shared_ptr<QuantLib::FraRateHelper> rateHelper(new QuantLib::FraRateHelper(QuoteHandle(quote), periodLengthToStart, pIborIndex(iborIndex)));
            rateHelpers.push_back(rateHelper);   
            return rateHelper;
        }
        QuantLib::ext::shared_ptr<QuantLib::FuturesRateHelper> AddFuture(
            const pQuote& quote,
            const QuantLib::Date& IMMDate,  // IMMDate = iborStartDate
            int lengthInMonths,
            const QuantLib::Calendar& calendar,
            QuantLib::BusinessDayConvention convention,
            const QuantLib::DayCounter& dayCounter,
            bool endOfMonth = true,
            const pQuote& convexityAdjustment = pQuote()
        ) {
            QuantLib::ext::shared_ptr<QuantLib::FuturesRateHelper> rateHelper(new QuantLib::FuturesRateHelper(QuoteHandle(quote), IMMDate, lengthInMonths, calendar, convention, endOfMonth, dayCounter, QuoteHandle(convexityAdjustment)));
            rateHelpers.push_back(rateHelper);
            return rateHelper;
        }
        // "indexed" version of AddFuture()
        QuantLib::ext::shared_ptr<QuantLib::FuturesRateHelper> AddFuture(
            const pQuote& quote,
            const QuantLib::Date& IMMDate,  // IMMDate = iborStartDate
            const pIborIndex& iborIndex,
            const pQuote& convexityAdjustment = pQuote()
        ) {
            QuantLib::ext::shared_ptr<QuantLib::FuturesRateHelper> rateHelper(new QuantLib::FuturesRateHelper(QuoteHandle(quote), IMMDate, iborIndex, QuoteHandle(convexityAdjustment)));
            rateHelpers.push_back(rateHelper);
            return rateHelper;
        }
        QuantLib::ext::shared_ptr<QuantLib::SwapRateHelper> AddSwap(
            const pQuote& quote,
            QuantLib::Natural settlementDays,
            const QuantLib::Period& tenor,  // swap tenor
            const QuantLib::Calendar& fixedCalendar,
            QuantLib::Frequency fixedFrequency,
            QuantLib::BusinessDayConvention fixedConvention,
            const QuantLib::DayCounter& fixedDayCount,
            const pIborIndex& floatIndex,
            const YieldTermStructureHandle& discountingCurve = YieldTermStructureHandle()  // exogenous discounting curve - for dual bootstrapping
        ) {
            QuantLib::ext::shared_ptr<QuantLib::SwapRateHelper> rateHelper(new QuantLib::SwapRateHelper(QuoteHandle(quote), tenor, fixedCalendar, fixedFrequency, fixedConvention, fixedDayCount, floatIndex, QuoteHandle(), 0 * QuantLib::Days, discountingCurve, settlementDays));
            rateHelpers.push_back(rateHelper);
            return rateHelper;
        }
        // "indexed" version of AddSwap()
        QuantLib::ext::shared_ptr<QuantLib::SwapRateHelper> AddSwap(
            const pQuote& quote,
            const pSwapIndex& swapIndex,
            const YieldTermStructureHandle& discountingCurve = YieldTermStructureHandle()  // exogenous discounting curve - for dual bootstrapping
        ) {
            QuantLib::ext::shared_ptr<QuantLib::SwapRateHelper> rateHelper(new QuantLib::SwapRateHelper(QuoteHandle(quote), swapIndex, QuantLib::Handle<QuantLib::Quote>(), 0 * QuantLib::Days, discountingCurve));
            rateHelpers.push_back(rateHelper);
            return rateHelper;
        }
        QuantLib::ext::shared_ptr<QuantLib::OISRateHelper> AddOIS(
            const pQuote& quote,
            QuantLib::Natural settlementDays,
            const QuantLib::Period& tenor,  // swap tenor
            const pOvernightIndex& overnightIndex,
            bool telescopicValueDates = false,  // true run faster
            QuantLib::RateAveraging::Type averagingMethod = QuantLib::RateAveraging::Compound,
            QuantLib::BusinessDayConvention paymentConvention = QuantLib::ModifiedFollowing,
            const YieldTermStructureHandle& discountingCurve = YieldTermStructureHandle()  // exogenous discounting curve - for dual bootstrapping
        ) {
            QuantLib::ext::shared_ptr<QuantLib::OISRateHelper> rateHelper(
                new QuantLib::OISRateHelper(
                    settlementDays,
                    tenor,
                    QuoteHandle(quote),
                    overnightIndex,
                    discountingCurve,   // exogenous discounting curve
                    telescopicValueDates,
                    0,
                    paymentConvention,
                    QuantLib::Annual,
                    QuantLib::Calendar(),
                    0 * QuantLib::Days,
                    0.0,
                    QuantLib::Pillar::LastRelevantDate,
                    QuantLib::Date(),
                    averagingMethod
                )
            );
            rateHelpers.push_back(rateHelper); 
            return rateHelper;
        }
        // "indexed" version of AddOIS()
        QuantLib::ext::shared_ptr<QuantLib::OISRateHelper> AddOIS(
            const pQuote& quote,
            pOvernightIndexedSwapIndex oisSwapIndex,
            const YieldTermStructureHandle& discountingCurve = YieldTermStructureHandle()  // exogenous discounting curve - for dual bootstrapping
        ) {
            QuantLib::ext::shared_ptr<QuantLib::OISRateHelper> rateHelper(
                new QuantLib::OISRateHelper(
                    oisSwapIndex->fixingDays(),
                    oisSwapIndex->tenor(),
                    QuoteHandle(quote),
                    oisSwapIndex->overnightIndex(),
                    discountingCurve,   // exogenous discounting curve
                    oisSwapIndex->telescopicValueDates(),
                    0,
                    oisSwapIndex->fixedLegConvention(),
                    QuantLib::Annual,
                    QuantLib::Calendar(),
                    0 * QuantLib::Days,
                    0.0,
                    QuantLib::Pillar::LastRelevantDate,
                    QuantLib::Date(),
                    oisSwapIndex->averagingMethod()
                )
            );
            rateHelpers.push_back(rateHelper);
            return rateHelper;
        }
        // T = traits, I = interpolation
        QuantLib::ext::shared_ptr<QuantLib::PiecewiseYieldCurve<T, I>> GetCurve(
            const QuantLib::Date& curveReferenceDate,
            const QuantLib::DayCounter& dayCounter,
            const I& interp = I()   // custom interpretor of type I
        ) {
            QuantLib::ext::shared_ptr<QuantLib::PiecewiseYieldCurve<T, I>> pTS(new QuantLib::PiecewiseYieldCurve<T, I>(curveReferenceDate, rateHelpers, dayCounter, interp));
            pTS->discount(0);   // trigger the bootstrap
            return pTS;
        }
    };
}