#pragma once

#include <string>
#include <algorithm>
#include <cmath>
#include <ql/quantlib.hpp>
#include <ql_utils/types.hpp>
#include <ql_utils/fixing-date-adjustment.hpp>
#include <ql_utils/ratehelpers/nominal_forward_ratehelper.hpp>

namespace QLUtils {
    class BootstrapInstrument {
    public:
        enum ValueType {
            vtRate = 0,
            vtPrice = 1,
        };
    protected:
        std::string ticker_;
        QuantLib::Period tenor_;
        QuantLib::Date datedDate_;
        ValueType valueType_;
        QuantLib::Real value_;
        bool use_;
    public:
        BootstrapInstrument(
            const ValueType& valueType,
            const QuantLib::Period& tenor = QuantLib::Period(),
            const QuantLib::Date& datedDate = QuantLib::Date()
        ) :
            valueType_(valueType),
            tenor_(tenor),
            datedDate_(datedDate),
            value_(QuantLib::Null<QuantLib::Real>()),
            use_(true) {}
        const std::string& ticker() const {
            return ticker_;
        }
        std::string& ticker() {
            return ticker_;
        }
        const QuantLib::Period& tenor() const {
            return tenor_;
        }
        QuantLib::Period& tenor() {
            return tenor_;
        }
        const QuantLib::Date& datedDate() const {
            return datedDate_;
        }
        QuantLib::Date& datedDate() {
            return datedDate_;
        }
        const ValueType& valueType() const {
            return valueType_;
        }
        ValueType& valueType() {
            return valueType_;
        }
        const QuantLib::Real& value() const {
            return value_;
        }
        QuantLib::Real& value() {
            return value_;
        }
        const bool& use() const {
            return use_;
        }
        bool& use() {
            return use_;
        }
        const QuantLib::Rate& rate() const {
            return value_;
        }
        QuantLib::Rate& rate() {
            return value_;
        }
        const QuantLib::Real& price() const {
            return value_;
        }
        QuantLib::Real& price() {
            return value_;
        }
        bool valueIsSet() const {
            return (value_ != QuantLib::Null<QuantLib::Real>());
        }
        QuantLib::Handle<QuantLib::Quote> quote() const {
            QuantLib::Handle<QuantLib::Quote> q(QuantLib::ext::shared_ptr<QuantLib::Quote>(new QuantLib::SimpleQuote(value())));
            return q;
        }
        virtual QuantLib::Date startDate() const = 0;
        virtual QuantLib::Date maturityDate() const = 0;
        virtual QuantLib::ext::shared_ptr<QuantLib::RateHelper> rateHelper(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const = 0;
        virtual QuantLib::Real impliedQuote(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const = 0;
		// calculate the simple forward rate between start and end date using the discount factors from the given yield term structure
        static QuantLib::Rate simpleForwardRate(
            const QuantLib::Date& start,
            const QuantLib::Date& end,
            const QuantLib::DayCounter& dayCounter,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& h
        ) {
            auto t = dayCounter.yearFraction(start, end);
			auto df_start = h->discount(start, true);
			auto df_end = h->discount(end, true);
            auto compounding = df_start / df_end;
			return (compounding - 1.) / t;  // compounding = 1 + forwardRate * t
        }
    };

    // base class for all par rate instruments
    // par rate instrument is a for rate/yield-based (not price-based) bootstrap bacause the price is anchored at 100 (par)
    class ParRateInstrument :
        public BootstrapInstrument,
        public IParYieldSplineNode,
        public IParRateInstrument
    {
    public:
        ParRateInstrument(
            const QuantLib::Period& tenor,
            const QuantLib::Date& datedDate = QuantLib::Date()
        ) : BootstrapInstrument(BootstrapInstrument::vtRate, tenor, datedDate) {}
    protected:
        QuantLib::ext::shared_ptr<QuantLib::Bond> parBond() const {
            return this->fixedRateBondHelper()->bond();
        }
    public:
        const QuantLib::Rate& parRate() const {
            return rate();
        }
        QuantLib::Rate& parRate() {
            return rate();
        }
        QuantLib::Date startDate() const {
            return parBond()->settlementDate();
        }
        QuantLib::Date maturityDate() const {
            return parBond()->maturityDate();
        }
        QuantLib::ext::shared_ptr<QuantLib::RateHelper> rateHelper(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            return this->fixedRateBondHelper();
        };
        QuantLib::Real impliedQuote(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            return this->impliedParRate(discountingTermStructure);
        }
        QuantLib::Rate parYield() const {
            return parRate();
        }
        QuantLib::Time parTerm() const {
            return this->parYieldSplineDayCounter().yearFraction(parBond()->settlementDate(), parBond()->maturityDate());
        }
    };

    // for converting spot or fwd par rates back to zero curve (par-to-zero bootstrapping)
	// rate/yield-based par instrument
    // used during par-shock
    template <
        QuantLib::Frequency COUPON_FREQ = QuantLib::Semiannual,
        QuantLib::Thirty360::Convention THIRTY_360_DC_CONVENTION = QuantLib::Thirty360::BondBasis
    >
    class ParRate : public ParRateInstrument {
    private:
        using ParYieldHelperType = ParYieldHelper<COUPON_FREQ, THIRTY_360_DC_CONVENTION>;
        QuantLib::Date baseReferenceDate_;
        QuantLib::Period forwardStart_;
    public:
        ParRate(
            const QuantLib::Period& tenor,
            const QuantLib::Date& baseReferenceDate,
            const QuantLib::Period& forwardStart = QuantLib::Period(0, QuantLib::Days)
        ) :
            ParRateInstrument(tenor),
            baseReferenceDate_(baseReferenceDate),
            forwardStart_(forwardStart)
        {}
        QuantLib::Date& baseReferenceDate() {
            return baseReferenceDate_;
        }
        const QuantLib::Date& baseReferenceDate() const {
            return baseReferenceDate_;
        }
        QuantLib::Period& forwardStart() {
            return forwardStart_;
        }
        const QuantLib::Period& forwardStart() const {
            return forwardStart_;
        }
        QuantLib::DayCounter parYieldSplineDayCounter() const {
            return ParYieldHelperType::parBondDayCounter();
        }
        QuantLib::ext::shared_ptr<QuantLib::FixedRateBondHelper> fixedRateBondHelper() const {
            QuantLib::ext::shared_ptr<QuantLib::FixedRateBondHelper> helper = ParYieldHelperType(tenor())
                .withParYield(parRate())
                .withBaseReferenceDate(baseReferenceDate())
                .withForwardStart(forwardStart());
            return helper;
        }
        QuantLib::Rate impliedParRate(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure
        ) const {
            QL_ASSERT(discountingTermStructure->referenceDate() == baseReferenceDate(), "discount curve base reference date (" << discountingTermStructure->referenceDate() << ") is not what's expected (" << baseReferenceDate() << ")");
            return ParYieldHelperType::parYield(
                discountingTermStructure.currentLink(),
                tenor(),
                forwardStart()
            );
        }
    };

    template
    <
        typename SWAP_TRAITS
    >
    class OISSwapIndex:
        public BootstrapInstrument {
    public:
        typedef SWAP_TRAITS SwapTraits;
        typedef typename SwapTraits::BaseSwapIndex BaseSwapIndex;
        typedef typename SwapTraits::OvernightIndex OvernightIndex;
        typedef QuantLib::ext::shared_ptr<QuantLib::OvernightIndex> pOvernightIndex;
        typedef QuantLib::ext::shared_ptr<QuantLib::OvernightIndexedSwap> pOvernightIndexedSwap;
    private:
        SwapTraits swapTraits_;
        QuantLib::Calendar fixingCalendar_; // swap fixing calendar for both legs
        QuantLib::Date fixingDate_; // swap fixing date
        QuantLib::Date effectiveDate_;  // swap effective date
    private:
        pOvernightIndex makeOvernightIndex(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& h = {}    // index estimating term structure
        ) const {
            return pOvernightIndex(new OvernightIndex(h));
        }
        // create a overnight index swap with the given swap spec/traits
        pOvernightIndexedSwap makeSwap(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& h = {}    // index estimating term structure
        ) const {
            auto overnightIndex = makeOvernightIndex(h);
            pOvernightIndexedSwap swap = QuantLib::MakeOIS(tenor(), overnightIndex, 0.0)
                .withEffectiveDate(effectiveDate())
                .withTelescopicValueDates(swapTraits_.telescopicValueDates(tenor()))
                .withAveragingMethod(swapTraits_.averagingMethod(tenor()))
                .withPaymentLag(swapTraits_.paymentLag(tenor()))
                .withPaymentAdjustment(swapTraits_.paymentConvention(tenor()))
                .withPaymentFrequency(swapTraits_.paymentFrequency(tenor()))
                .withPaymentCalendar(swapTraits_.paymentCalendar(tenor()))
                .withFixedLegCalendar(fixingCalendar())
                .withOvernightLegCalendar(fixingCalendar())
                ;
            return swap;
        }
    public:
        OISSwapIndex(
            const QuantLib::Period& tenor,
            QuantLib::Date refDate = QuantLib::Date()
        ) : BootstrapInstrument(BootstrapInstrument::vtRate, tenor)
        {
            if (refDate == QuantLib::Date()) {
                refDate = QuantLib::Settings::instance().evaluationDate();
            }
            fixingCalendar_ = swapTraits_.fixingCalendar(tenor);
            auto settlementDays = swapTraits_.settlementDays(tenor);
            QuantLib::Utils::FixingDateAdjustment fixingAdj(settlementDays, fixingCalendar_);
            auto ret = fixingAdj.calculate(refDate);
            fixingDate_ = std::get<0>(ret);
            effectiveDate_ = std::get<1>(ret);
            this->datedDate() = makeSwap()->maturityDate();
        }
        const QuantLib::Calendar& fixingCalendar() const {
            return fixingCalendar_;
        }
        QuantLib::Date fixingDate() const {
            return fixingDate_;
        }
        QuantLib::Date effectiveDate() const {
            return effectiveDate_;
        }
        QuantLib::Date startDate() const {
            return effectiveDate_;
        }
        QuantLib::Date maturityDate() const {
            return this->datedDate();
        }
        QuantLib::ext::shared_ptr<QuantLib::RateHelper> rateHelper(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = {}   // exogenous discounting curve
        ) const {
			auto startDate = this->startDate();
			auto endDate = this->maturityDate();
            auto overnightIndex = makeOvernightIndex();
            QuantLib::ext::shared_ptr<QuantLib::OISRateHelper> helper(
                new QuantLib::OISRateHelper(
                    startDate,    // startDate
					endDate,  // endDate
                    quote(),    // fixedRate
                    overnightIndex, // overnightIndex
                    discountingTermStructure,   // discountingCurve - exogenous discounting curve
                    swapTraits_.telescopicValueDates(tenor()),  // telescopicValueDates
                    swapTraits_.paymentLag(tenor()),    // paymentLag
                    swapTraits_.paymentConvention(tenor()), // paymentConvention
                    swapTraits_.paymentFrequency(tenor()),   // paymentFrequency
                    swapTraits_.paymentCalendar(tenor()),   // paymentCalendar
                    QuantLib::Spread(0.),    // overnightSpread
                    QuantLib::Pillar::MaturityDate, // pillar
                    QuantLib::Date(),   // customPillarDate
                    swapTraits_.averagingMethod(tenor()),    // averagingMethod
                    QuantLib::ext::nullopt,   // endOfMonth
                    QuantLib::ext::nullopt,   // fixedPaymentFrequency
                    fixingCalendar(), // fixedCalendar
                    QuantLib::Null<QuantLib::Natural>(),    // lookbackDays
                    0,  // lockoutDays
                    false,  // applyObservationShift
                    {}, // pricer
                    QuantLib::DateGeneration::Backward,   // rule
                    fixingCalendar()  // overnightCalendar
                )
            );
            auto swap = helper->swap();
			QL_ASSERT(swap->startDate() == startDate, "swap start date (" << swap->startDate() << ") is not what's expected (" << startDate << ")");
			QL_ASSERT(swap->maturityDate() == endDate, "swap maturity date (" << swap->maturityDate() << ") is not what's expected (" << endDate << ")");
            QL_ASSERT(swap->fixedRate() == rate(), "swap fixed rate (" << QuantLib::io::percent(swap->fixedRate()) << ") is not what's expected (" << QuantLib::io::percent(rate()) << ")");
            return helper;
        }
        QuantLib::Real impliedQuote(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = {}
        ) const {
            QuantLib::ext::shared_ptr<QuantLib::PricingEngine> swapPricingEngine(new QuantLib::DiscountingSwapEngine(discountingTermStructure));
            auto swap = makeSwap(estimatingTermStructure);
            swap->setPricingEngine(swapPricingEngine);
            auto swapRate = swap->fairRate();
            return swapRate;
        }
    };

    class IborIndexInstrument : public BootstrapInstrument {
    protected:
        IborIndexFactory iborIndexFactory_;
    public:
        typedef QuantLib::ext::shared_ptr<QuantLib::IborIndex> pIborIndex;
    public:
        IborIndexInstrument(
            const IborIndexFactory& iborIndexFactory,
            const BootstrapInstrument::ValueType& valueType,
            const QuantLib::Period& tenor = QuantLib::Period(),
            const QuantLib::Date& datedDate = QuantLib::Date()
        ) : BootstrapInstrument(valueType, tenor, datedDate), iborIndexFactory_(iborIndexFactory)
        {
        }
        const IborIndexFactory& iborIndexFactory() const {
            return iborIndexFactory_;
        }
        pIborIndex makeIborIndex(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& h = {}    // estimating term structure
        ) const {
            return iborIndexFactory()(h);
        }
    };

    class SwapCurveInstrument : public IborIndexInstrument {
    public:
        enum InstType {
            Deposit = 0,    // cash deposit
            Future = 1,
            FRA = 2,
            Swap = 3,
        };
    protected:
        InstType instType_;
    public:
        SwapCurveInstrument(
            const IborIndexFactory& iborIndexFactory,
            const BootstrapInstrument::ValueType& valueType,
            const InstType& instType,
            const QuantLib::Period& tenor = QuantLib::Period(),
            const QuantLib::Date& datedDate = QuantLib::Date()
        ) : IborIndexInstrument(iborIndexFactory, valueType, tenor, datedDate), instType_(instType)
        {}
        InstType instType() const {
            return instType_;
        }
    };

    class CashDepositIndex : public SwapCurveInstrument {
    private:
        QuantLib::DayCounter dayCounter_;   // day counter for the index
        QuantLib::Date fixingDate_; // fixing date for the index
        QuantLib::Date valueDate_;  // value date for the index, also the start date of the deposit
    public:
        CashDepositIndex(
            const IborIndexFactory& iborIndexFactory,
            const QuantLib::Period& tenor,
            QuantLib::Date refDate = QuantLib::Date()
        ) :
            SwapCurveInstrument(iborIndexFactory, BootstrapInstrument::vtRate, SwapCurveInstrument::Deposit, tenor)
        {
            if (refDate == QuantLib::Date()) {
                refDate = QuantLib::Settings::instance().evaluationDate();
            }
            auto iborIndex = makeIborIndex();
            dayCounter_ = iborIndex->dayCounter();
            QuantLib::Utils::FixingDateAdjustment fixingAdj(iborIndex->fixingDays(), iborIndex->fixingCalendar());
            auto ret = fixingAdj.calculate(refDate);
            fixingDate_ = std::get<0>(ret);
            valueDate_ = std::get<1>(ret);
            this->datedDate() = iborIndex->maturityDate(valueDate_);
        }
    public:
        QuantLib::Date fixingDate() const {
            return fixingDate_;
        }
        QuantLib::Date valueDate() const {
            return valueDate_;
        }
        QuantLib::Date startDate() const {
            return valueDate_;
        }
        QuantLib::Date maturityDate() const {
            return datedDate();
        }
        const QuantLib::DayCounter& dayCounter() const {
            return dayCounter_;
        }
        QuantLib::ext::shared_ptr<QuantLib::RateHelper> rateHelper(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = {}   // exogenous discounting curve
        ) const {
            QuantLib::ext::shared_ptr<QuantLib::DepositRateHelper> helper(
                new QuantLib::DepositRateHelper(
                    quote(),    // rate
					fixingDate(),   // fixingdate
                    makeIborIndex() // iborIndex
                )
            );
            return helper;
        }
        QuantLib::Rate impliedRate(
			const QuantLib::Handle<QuantLib::YieldTermStructure>& h // index estimating term structure
        ) const {
            auto iborIndex = makeIborIndex(h);
            auto rate = iborIndex->fixing(fixingDate());
            return rate;
        }
        QuantLib::Real impliedQuote(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = {}   // exogenous discounting curve
        ) const {
            return impliedRate(estimatingTermStructure);
        }
    };

    class IMMFuture : public SwapCurveInstrument {
    protected:
        QuantLib::Natural immOrdinal_;
        std::string immTicker_;
        QuantLib::Rate convexityAdj_;
    public:
        static void ensureIMMDate(const QuantLib::Date& immDate) {
            QL_REQUIRE(QuantLib::IMM::isIMMdate(immDate, true), "specified date " << immDate << " is not a main cycle IMM date");
        }
        static void ensureIMMDateNotExpired(
            const QuantLib::Date& immDate,
            const QuantLib::Date& refDate
        ) {
            QL_REQUIRE(immDate >= refDate, "IMM date " << immDate << " already expired");
        }
        static QuantLib::Date IMMMainCycleStartDateForOrdinal(
            QuantLib::Natural iMMOrdinal,
            const QuantLib::Date& today = QuantLib::Date()
        ) {
            QL_REQUIRE(iMMOrdinal > 0, "IMM ordinal must be an integer greater than 0");
            QuantLib::Date d = (today == QuantLib::Date() ? QuantLib::Settings::instance().evaluationDate() : today);
            d += 2;
            if (!QuantLib::IMM::isIMMdate(d, true)) {
                d = QuantLib::IMM::nextDate(d, true);
            }
            // d is now the first IMM date after today
            QuantLib::Natural immFound = 1;
            while (immFound < iMMOrdinal) {
                d = QuantLib::IMM::nextDate(d, true);
                immFound++;
            }
            return d;
        }
        static QuantLib::Natural IMMMainCycleOrdinalForStartDate(
            const QuantLib::Date& immDate,
            const QuantLib::Date& today = QuantLib::Date()
        ) {
            ensureIMMDate(immDate);
            QuantLib::Date d = (today == QuantLib::Date() ? QuantLib::Settings::instance().evaluationDate() : today);
            d += 2;
            if (!QuantLib::IMM::isIMMdate(d, true)) {
                d = QuantLib::IMM::nextDate(d, true);
            }
            // d is now the first IMM date after today
            ensureIMMDateNotExpired(immDate, d);
            QuantLib::Natural ordinal = 1;
            while (d < immDate) {
                d = QuantLib::IMM::nextDate(d, true);
                ordinal++;
            }
            return ordinal;
        }
        static QuantLib::Period calculateTenor(
            const QuantLib::Date& immDate,
            const QuantLib::Date& today = QuantLib::Date()
        ) {
            ensureIMMDate(immDate);
            QuantLib::Date d = (today == QuantLib::Date() ? QuantLib::Settings::instance().evaluationDate() : today);
            d += 2;
            ensureIMMDateNotExpired(immDate, d);
            auto days = immDate - d;
            return days * QuantLib::Days;
        }
        IMMFuture(const IborIndexFactory& iborIndexFactory, QuantLib::Natural immOrdinal) :
            SwapCurveInstrument(iborIndexFactory, BootstrapInstrument::vtPrice, SwapCurveInstrument::Future, QuantLib::Period(), QuantLib::Null < QuantLib::Date >()),
            immOrdinal_(immOrdinal), convexityAdj_(0.0) {
            this->datedDate_ = IMMMainCycleStartDateForOrdinal(immOrdinal);
            this->tenor_ = calculateTenor(this->datedDate_);
            this->immTicker_ = QuantLib::IMM::code(this->datedDate_);
        }
        IMMFuture(const IborIndexFactory& iborIndexFactory, const QuantLib::Date& immDate) :
            SwapCurveInstrument(iborIndexFactory, BootstrapInstrument::vtPrice, SwapCurveInstrument::Future, calculateTenor(immDate), immDate),
            immOrdinal_(IMMMainCycleOrdinalForStartDate(immDate)),
            immTicker_(QuantLib::IMM::code(immDate)),
            convexityAdj_(0.0){}
        const QuantLib::Date& immDate() const {
            return this->datedDate_;
        }
        const QuantLib::Natural& immOrdinal() const {
            return immOrdinal_;
        }
        const std::string& immTicker() const {
            return immTicker_;
        }
        const QuantLib::Rate& convexityAdj() const {
            return convexityAdj_;
        }
        QuantLib::Rate& convexityAdj() {
            return convexityAdj_;
        }
        QuantLib::Date immEndDate() const {
            return QuantLib::IMM::nextDate(immDate(), true);
        }
        QuantLib::Date startDate() const {
            return immDate();
        }
        QuantLib::Date maturityDate() const {
            return immEndDate();
        }
        QuantLib::ext::shared_ptr<QuantLib::RateHelper> rateHelper(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            auto iborIndex = this->makeIborIndex();
            QuantLib::Handle<QuantLib::Quote> convAdjQuote(QuantLib::ext::shared_ptr<QuantLib::Quote>(new QuantLib::SimpleQuote(convexityAdj())));
            QuantLib::ext::shared_ptr<QuantLib::FuturesRateHelper> helper(
                new QuantLib::FuturesRateHelper(
                    quote(),
                    immDate(),
                    QuantLib::Date(),
                    iborIndex->dayCounter(),
                    convAdjQuote
                )
            );
            return helper;
        }
        QuantLib::Real impliedQuote(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            auto iborIndex = this->makeIborIndex();
            auto forwardRate = this->simpleForwardRate(immDate(), immEndDate(), iborIndex->dayCounter(), estimatingTermStructure);
            auto const& convAdj = convexityAdj();
            auto futureRate = forwardRate + convAdj;
            return 100.0 * (1.0 - futureRate);
        }
    };
    
    // FRA
    // this can be use to bootstrap Libor 3m estimating zero curve given a monthly Libor 3m forward rate curves
    class FRA : public SwapCurveInstrument {
    private:
        QuantLib::ext::shared_ptr<QuantLib::FraRateHelper> createRateHelperImpl(
            QuantLib::Rate quotedRate = 0.
        ) const {
            auto iborIndex = this->makeIborIndex();
            QuantLib::ext::shared_ptr<QuantLib::FraRateHelper> rateHelper(
                new QuantLib::FraRateHelper(
                    quotedRate,
                    forward(),
                    iborIndex,
                    QuantLib::Pillar::MaturityDate,
                    QuantLib::Date(),
                    false
                )
            );
            return rateHelper;
        }
        typedef std::pair<QuantLib::Date, QuantLib::Date> CalcDatesResult;
        CalcDatesResult calcDates() const {
            auto rateHelper = createRateHelperImpl();
            return CalcDatesResult(rateHelper->earliestDate(), rateHelper->maturityDate());
        }
    public:
        FRA(
            const IborIndexFactory& iborIndexFactory,
            const QuantLib::Period& forward
        ) : SwapCurveInstrument(iborIndexFactory, BootstrapInstrument::vtRate, SwapCurveInstrument::FRA, forward, QuantLib::Null<QuantLib::Date>())
        {
            this->datedDate_ = calcDates().first;    // datedDate_ stores the earliestDate/startDate
        }
        // for FRA tenor is the forward period
        const QuantLib::Period& forward() const {
            return tenor();
        }
        QuantLib::Date startDate() const {
            return this->datedDate_;
        }
        QuantLib::Date maturityDate() const {
            return calcDates().second;
        }
        QuantLib::ext::shared_ptr<QuantLib::RateHelper> rateHelper(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            return createRateHelperImpl(rate());
        }
        QuantLib::Rate impliedRate(
            const QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure>& estimatingTermStructure
        ) const {
            auto rateHelper = createRateHelperImpl();
            rateHelper->setTermStructure(estimatingTermStructure.get());
            auto rate = rateHelper->impliedQuote();
            return rate;
        }
        QuantLib::Real impliedQuote(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            return impliedRate(estimatingTermStructure.currentLink());
        }
    };
    
    template<
        typename SwapTraits
    >
    class SwapIndex : public SwapCurveInstrument {
    private:
        SwapTraits swapTraits_;
        QuantLib::Calendar fixingCalendar_; // swap fixing calendar for both legs
        QuantLib::Date fixingDate_;    // swap fixing date
        QuantLib::Date effectiveDate_;  // swap effective date
        bool endOfMonth_;   // end of month flag for both legs
    public:
        typedef QuantLib::ext::shared_ptr<QuantLib::VanillaSwap> pVanillaSwap;
    private:
		// create a vanilla swap with the given swap spec/traits
        pVanillaSwap makeSwap(
			const QuantLib::Handle<QuantLib::YieldTermStructure>& h = {}    // index estimating term structure
        ) const {
            auto iborIndex = this->makeIborIndex(h);
            pVanillaSwap swap = QuantLib::MakeVanillaSwap(this->tenor(), iborIndex, 0.0)
                .withEffectiveDate(effectiveDate())
                .withFixedLegCalendar(fixingCalendar())
                .withFixedLegDayCount(swapTraits_.fixedLegDayCount(tenor()))
                .withFixedLegTenor(swapTraits_.fixedLegTenor(tenor()))
                .withFixedLegConvention(swapTraits_.fixedLegConvention(tenor()))
                .withFixedLegTerminationDateConvention(swapTraits_.fixedLegConvention(tenor()))
                .withFixedLegEndOfMonth(endOfMonth())
                .withFloatingLegCalendar(fixingCalendar())
                .withFloatingLegEndOfMonth(endOfMonth());
            return swap;
        }
    public:
        SwapIndex(
            const IborIndexFactory& iborIndexFactory,
            const QuantLib::Period& tenor,
            QuantLib::Date refDate = QuantLib::Date()
        ) : SwapCurveInstrument(iborIndexFactory, BootstrapInstrument::vtRate, SwapCurveInstrument::Swap, tenor)
        {
            if (refDate == QuantLib::Date()) {
                refDate = QuantLib::Settings::instance().evaluationDate();
            }
            fixingCalendar_ = swapTraits_.fixingCalendar(tenor);
            auto settlementDays = swapTraits_.settlementDays(tenor);
            QuantLib::Utils::FixingDateAdjustment fixingAdj(settlementDays, fixingCalendar_);
            auto ret = fixingAdj.calculate(refDate);
            fixingDate_ = std::get<0>(ret);
            effectiveDate_ = std::get<1>(ret);
            this->datedDate() = makeSwap()->maturityDate();
            endOfMonth_ = swapTraits_.endOfMonth(tenor);
        }
        const QuantLib::Calendar& fixingCalendar() const {
            return fixingCalendar_;
        }
        QuantLib::Date fixingDate() const {
            return fixingDate_;
        }
        QuantLib::Date effectiveDate() const {
            return effectiveDate_;
        }
        QuantLib::Date startDate() const {
            return effectiveDate_;
        }
        QuantLib::Date maturityDate() const {
            return this->datedDate();
        }
        bool endOfMonth() const {
            return endOfMonth_;
        }
        QuantLib::ext::shared_ptr<QuantLib::RateHelper> rateHelper(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = {}   // exogenous discounting curve
        ) const {
            auto startDate = this->startDate();
            auto endDate = this->maturityDate();
            auto iborIndex = this->makeIborIndex();
            QuantLib::ext::shared_ptr<QuantLib::SwapRateHelper> helper(
                new QuantLib::SwapRateHelper(
                    quote(),    // rate
                    startDate,    // startDate
                    endDate,    // endDate
                    fixingCalendar(),   // calendar
                    swapTraits_.fixedLegFrequency(tenor()), // fixedFrequency
                    swapTraits_.fixedLegConvention(tenor()),    // fixedConvention
                    swapTraits_.fixedLegDayCount(tenor()),  // fixedDayCount
                    iborIndex,  // iborIndex
                    {},    // spread
                    discountingTermStructure,   // discountingCurve - exogenous discounting curve
                    QuantLib::Pillar::MaturityDate, // pillar
                    QuantLib::Date(),   // customPillarDate
                    endOfMonth()  // endOfMonth
                )
            );
            auto swap = helper->swap();
            QL_ASSERT(swap->startDate() == startDate, "swap start date (" << swap->startDate() << ") is not what's expected (" << startDate << ")");
            QL_ASSERT(swap->maturityDate() == endDate, "swap maturity date (" << swap->maturityDate() << ") is not what's expected (" << endDate << ")");
            QL_ASSERT(swap->fixedRate() == rate(), "swap fixed rate (" << QuantLib::io::percent(swap->fixedRate()) << ") is not what's expected (" << QuantLib::io::percent(rate()) << ")");
            return helper;
        }
        QuantLib::Real impliedQuote(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = {}
        ) const {
            QuantLib::ext::shared_ptr<QuantLib::PricingEngine> swapPricingEngine(new QuantLib::DiscountingSwapEngine(discountingTermStructure));
            auto swap = makeSwap(estimatingTermStructure);
            swap->setPricingEngine(swapPricingEngine);
            auto swapRate = swap->fairRate();
            return swapRate;
        }
    };

    // for converting simple forward rate curve to zero curve (simple-forward-to-zero bootstrapping)
    // use QuantLib::Thirty360::ISDA as default day count convention for simplified calculation
    template <
        QuantLib::Thirty360::Convention THIRTY_360_DC_CONVENTION = QuantLib::Thirty360::ISDA
    >
    class SimpleForward : public FRA {
    private:
        QuantLib::Natural lengthInMonths_;  // tenor in months
        std::string familyName_;
    public:
        static IborIndexFactory getIborFactory(
            QuantLib::Natural lengthInMonths,
            const std::string& familyName
        ) {
            return [lengthInMonths, familyName](const QuantLib::Handle<QuantLib::YieldTermStructure>& h) {
                return QuantLib::ext::make_shared<QuantLib::IborIndex>(
                    familyName, // family name
                    lengthInMonths * QuantLib::Months,
                    0,  // no fixing day
                    QuantLib::Currency(),
                    QuantLib::NullCalendar(),   // no holiday
                    QuantLib::BusinessDayConvention::Unadjusted,    // no business day adjustment
                    false,   // endOfMonth = false
                    QuantLib::Thirty360(THIRTY_360_DC_CONVENTION), // 30/360 day counting
                    h
                );
            };
        }
        SimpleForward(
            const QuantLib::Period& forward,
            QuantLib::Natural lengthInMonths = 1,    // default to 1-month tenor
            const std::string& familyName = "no-fix"
        ) :
            FRA(getIborFactory(lengthInMonths, familyName), forward),
            lengthInMonths_(lengthInMonths),
            familyName_(familyName)
        {}
        QuantLib::Natural lengthInMonths() const {
            return lengthInMonths_;
        }
        const std::string& familyName() const {
            return familyName_;
        }
        IborIndexFactory getIborFactory() const {
            return getIborFactory(lengthInMonths(), familyName());
        }
    };

    // for converting forward par rate curve to zero curve (forward-par-to-zero bootstrapping)
    // use QuantLib::Thirty360::ISDA as default day count convention for simplified calculation
    template <
        QuantLib::Frequency COUPON_FREQ = QuantLib::Semiannual,
        QuantLib::Thirty360::Convention THIRTY_360_DC_CONVENTION = QuantLib::Thirty360::ISDA
    >
    class ParForward : public ParRate<COUPON_FREQ, THIRTY_360_DC_CONVENTION> {
    public:
        ParForward(
            const QuantLib::Period& tenor,
            const QuantLib::Period& forward
        ) :
            ParRate<COUPON_FREQ, THIRTY_360_DC_CONVENTION>(
                tenor,
                QuantLib::Settings::instance().evaluationDate(),
                forward
            )
        {}
    };
    // for converting par rate curve to zero curve (spot-par-to-zero bootstrapping)
    // use QuantLib::Thirty360::ISDA as default day count convention for simplified calculation
    template <
        QuantLib::Frequency COUPON_FREQ = QuantLib::Semiannual,
        QuantLib::Thirty360::Convention THIRTY_360_DC_CONVENTION = QuantLib::Thirty360::ISDA
    >
    class ParSpot : public ParForward<COUPON_FREQ, THIRTY_360_DC_CONVENTION> {
    public:
        ParSpot(
            const QuantLib::Period& tenor
        ) :
            ParForward<COUPON_FREQ, THIRTY_360_DC_CONVENTION>(tenor, QuantLib::Period(0, QuantLib::Days))
        {}
    };

    // for bootstrapping zero curve from nominal (no calendar adjustment) forward rates
    template<
        QuantLib::Integer TENOR_MONTHS = 1,
        QuantLib::Thirty360::Convention THIRTY_360_DC_CONVENTION = QuantLib::Thirty360::BondBasis,
        QuantLib::Compounding COMPOUNDING = QuantLib::Compounding::Continuous,
        QuantLib::Frequency FREQUENCY = QuantLib::Frequency::NoFrequency
    >
    class NominalForwardRate : public BootstrapInstrument {
    private:
        QuantLib::NominalForwardRateHelper helperInternal_;
    public:
        NominalForwardRate(
            const QuantLib::Period& forward,
            const QuantLib::Date& baseReferenceDate = QuantLib::Date()
        ) :
            BootstrapInstrument(ValueType::vtRate, forward),
            helperInternal_(
                0.,
                forward,
                baseReferenceDate,
                QuantLib::Period(TENOR_MONTHS, QuantLib::Months),	// tenor
                QuantLib::Thirty360(THIRTY_360_DC_CONVENTION),	// day counter
                COMPOUNDING,	// compounding
                FREQUENCY	// frequency
            )
        {
            this->datedDate() = helperInternal_.startDate();
        }
        QuantLib::Date startDate() const {
            return helperInternal_.startDate();
        }
        QuantLib::Date maturityDate() const {
            return helperInternal_.maturityDate();
        }
        QuantLib::ext::shared_ptr<QuantLib::RateHelper> rateHelper(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            return QuantLib::ext::shared_ptr<QuantLib::RateHelper>(
                new QuantLib::NominalForwardRateHelper(
                    this->rate(),
                    helperInternal_.forward(),
                    helperInternal_.baseReferenceDate(),
                    helperInternal_.tenor(),
                    helperInternal_.dayCounter(),
                    helperInternal_.compounding(),
                    helperInternal_.frequency()
                )
            );
        }
        QuantLib::Rate impliedRate(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure
        ) const {
            auto rate = QuantLib::NominalForwardRateHelper::impliedRate(
                *(estimatingTermStructure.currentLink().get()),
                helperInternal_.startDate(),
                helperInternal_.maturityDate(),
                helperInternal_.dayCounter(),
                helperInternal_.compounding(),
                helperInternal_.frequency()
            );
            return rate;
        }
        QuantLib::Real impliedQuote(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            return impliedRate(estimatingTermStructure);
        }
    };
}
