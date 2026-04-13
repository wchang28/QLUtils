#pragma once

#include <string>
#include <algorithm>
#include <cmath>
#include <ql/quantlib.hpp>
#include <ql_utils/types.hpp>
#include <ql_utils/fixing-date-adjustment.hpp>
#include <ql_utils/swap-index-traits.hpp>
#include <ql_utils/ratehelpers/nominal_forward_ratehelper.hpp>

namespace QLUtils {
    // base class for all yied curve bootstrap instruments
    class BootstrapInstrument {
    public:
        enum ValueType {
            vtRate = 0,
            vtPrice = 1,    // 100 nominal/notional price
            vtSpread = 2,
        };
        typedef QuantLib::Handle<QuantLib::YieldTermStructure> YieldTermStructureHandle;
        typedef QuantLib::ext::shared_ptr<QuantLib::RateHelper> pRateHelper;
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
            use_(true)
        {}
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
        const QuantLib::Spread& spread() const {
            return value_;
        }
        QuantLib::Spread& spread() {
            return value_;
        }
        // derived classes can override the following methods to provide more specific alias for the instrument type and value type for better error message
        //////////////////////////////////////////////////////////
        virtual std::string instrumentTypeAlias() const {
            return "bootstrap instrument";
        }
        virtual std::string valueTypeAlias() const {
            if (valueType_ == vtRate) {
                return "rate";
            }
            else if (valueType_ == vtSpread) {
                return "spread";
            }
            else if (valueType_ == vtPrice) {
                return "price";
            }
            else {
                return "value";
            }
        }
        //////////////////////////////////////////////////////////
        // checking the quoted value is set or not
        //////////////////////////////////////////////////////////
        bool valueIsSet() const {
            return (value_ != QuantLib::Null<QuantLib::Real>());
        }
        void ensureValueIsSet() const {
            QL_REQUIRE(valueIsSet(), "quoted " << this->valueTypeAlias() << " is not set for " << this->instrumentTypeAlias() << (ticker_.empty() ? std::string{} : std::string(": ") + ticker_));
        }
        //////////////////////////////////////////////////////////
        // a multiplier that converts the difference in values between actual vs. implied to decimal unit
        // (implied-actual)*absoluteDiffMultiplier()
        QuantLib::Real absoluteDiffMultiplier() const {
            if (valueType_ == vtRate || valueType_ == vtSpread) {   // values already in decimal unit
                return 1.;
            } else if (valueType_ == vtPrice) {
                return 0.01;    // (100 - price) is "rate" in % unit, multiply by 0.01 to convert to decimal unit
            } else {
                return 1.;
            }
        }
        // a multiplier that converts the difference in values between actual vs. implied to basis point unit
        // (implied-actual)*basisPointDiffMultiplier() << " bp"
        QuantLib::Real basisPointDiffMultiplier() const {
            return absoluteDiffMultiplier() * 10000.;
        }
        // a multiplier that convert quote value to it's native unit
        QuantLib::Real valueMultiplier() const {
            if (valueType_ == vtRate) {
                return 100.; // decimal to percent unit (%)
            } else if (valueType_ == vtSpread) {
                return 10000.;  // decimal to basis point unit (bp)
            } else if (valueType_ == vtPrice) {
                return 1.;
            } else {
                return 1.;
            }
        }
        std::string nativeUnitString() const {
            if (valueType_ == vtRate) {
                return "%";
            } else if (valueType_ == vtSpread) {
                return "bp";
            } else if (valueType_ == vtPrice) {
                return "";
            } else {
                return "";
            }
        }
        QuantLib::Handle<QuantLib::Quote> quote() const {
            ensureValueIsSet();
            QuantLib::Handle<QuantLib::Quote> q(QuantLib::ext::shared_ptr<QuantLib::Quote>(new QuantLib::SimpleQuote(value())));
            return q;
        }
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
        // virtual methods that must be implemented by derived classes for bootstrapping
        ////////////////////////////////////////////////////////////////////////////////
        virtual QuantLib::Date startDate() const = 0;
        virtual QuantLib::Date maturityDate() const = 0;
        virtual pRateHelper rateHelper(
            const YieldTermStructureHandle& discountingTermStructure = {}   // empty if bootstrapping both estimating and discounting term structures concurrently, non-empty if bootstrapping estimating term structure only with an already exist exogenous discounting term structure
        ) const = 0;    // rate helper factory for the yield term structure bootstrapping
        virtual QuantLib::Real impliedQuote(
            const YieldTermStructureHandle& estimatingTermStructure,
            const YieldTermStructureHandle& discountingTermStructure = {}
        ) const = 0;
        ////////////////////////////////////////////////////////////////////////////////
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
        {}
        const IborIndexFactory& iborIndexFactory() const {
            return iborIndexFactory_;
        }
        IborIndexFactory& iborIndexFactory() {
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
            BasisSwap = 4,
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
        QuantLib::Natural fixingDays_;  // fixing days for the index
        QuantLib::Calendar fixingCalendar_; // fixing calendar for the index
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
            fixingDays_ = iborIndex->fixingDays();
            fixingCalendar_ = iborIndex->fixingCalendar();
            dayCounter_ = iborIndex->dayCounter();
            QuantLib::Utils::FixingDateAdjustment fixingAdj(fixingDays_, fixingCalendar_);
            auto ret = fixingAdj.calculate(refDate);
            fixingDate_ = std::get<0>(ret);
            valueDate_ = std::get<1>(ret);
            datedDate() = iborIndex->maturityDate(valueDate_);  // calculate the maturity date
        }
    public:
        QuantLib::Natural fixingDays() const {
            return fixingDays_;
        }
        const QuantLib::Calendar& fixingCalendar() const {
            return fixingCalendar_;
        }
        QuantLib::Date fixingDate() const {
            return fixingDate_;
        }
        QuantLib::Date valueDate() const {
            return valueDate_;
        }
        std::string instrumentTypeAlias() const override {
            return "cash deposit";
        }
        std::string valueTypeAlias() const override {
            return "cash rate";
        }
        QuantLib::Date startDate() const override {
            return valueDate_;
        }
        QuantLib::Date maturityDate() const override {
            return datedDate();
        }
        const QuantLib::DayCounter& dayCounter() const {
            return dayCounter_;
        }
        pRateHelper rateHelper(
            const YieldTermStructureHandle& discountingTermStructure = {}   // exogenous discounting curve
        ) const override {
            QuantLib::ext::shared_ptr<QuantLib::DepositRateHelper> helper(
                new QuantLib::DepositRateHelper(
                    quote(),    // rate
                    fixingDate(),   // fixingDate
                    makeIborIndex() // iborIndex
                )
            );
            return helper;
        }
        QuantLib::Rate impliedRate(
            const YieldTermStructureHandle& h // index estimating term structure
        ) const {
            auto iborIndex = makeIborIndex(h);
            auto rate = iborIndex->fixing(fixingDate());
            return rate;
        }
        QuantLib::Real impliedQuote(
            const YieldTermStructureHandle& estimatingTermStructure,
            const YieldTermStructureHandle& discountingTermStructure = {}   // exogenous discounting curve
        ) const override {
            return impliedRate(estimatingTermStructure);
        }
    };

    template <
        typename IborIndexType // can be Sofr, FedFunds, Sonia, Estr, Eonia, TermSofr<1>, TermSofr<3>, TermSofr<6>, TermSofr<12>
    >
    class IborIndexCashDeposit : public CashDepositIndex {
    public:
        IborIndexCashDeposit(
            QuantLib::Date refDate = QuantLib::Date()
        ) :
            CashDepositIndex(
                [](const YieldTermStructureHandle& h) {return pIborIndex(new IborIndexType(h));},
                IborIndexType{}.tenor(),
                refDate
            )
        {}
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

    // swap index base template class for both vanilla swap and OIS swap, the specific swap type is determined by the template parameter SwapTraits which provides the necessary traits and factory methods to create the swap and the ibor index for the swap floating leg
    template<
        typename SWAP_TRAITS
    >
    class SwapIndexBase : public SwapCurveInstrument {
    public:
        typedef SWAP_TRAITS TraitsType;
        typedef typename TraitsType::BaseSwapIndex BaseSwapIndex;
        typedef typename TraitsType::SwapPtr SwapPtr;
    private:
        TraitsType swapTraits_;
        typename TraitsType::FixingResult fixingResult_;
    private:
        // create a swap with the given swap spec/traits
        SwapPtr makeSwap(
            const YieldTermStructureHandle& h = {}    // index estimating term structure
        ) const {
            return swapTraits_.makeSwap(
                startDate(),    // startDate
                QuantLib::Swap::Type::Payer,    // type
                0., // fixedRate
                0., // floatSpread or overnightSpread
                h   // h - index estimating term structure
            );
        }
    public:
        SwapIndexBase(
            const QuantLib::Period& tenor,
            QuantLib::Date refDate = QuantLib::Date()
        ) :
            SwapCurveInstrument(
                nullptr,
                BootstrapInstrument::vtRate,
                SwapCurveInstrument::Swap,
                tenor
            ),
            swapTraits_(tenor),
            fixingResult_(swapTraits_.calculateFixing(refDate))
        {
            const auto& swapTraits = this->swapTraits_;
            this->iborIndexFactory_ = [&swapTraits](const YieldTermStructureHandle& h) {
                return swapTraits.makeIborIndex(h);
            };
            this->datedDate() = makeSwap()->maturityDate(); // calculate the swap maturity date
        }
        const QuantLib::Calendar& fixingCalendar() const {
            return fixingResult_.fixingCalendar;
        }
        QuantLib::Date fixingDate() const {
            return fixingResult_.fixingDate;
        }
        std::string valueTypeAlias() const override {
            return "swap rate";
        }
        QuantLib::Date startDate() const override {
            return fixingResult_.startDate;
        }
        QuantLib::Date maturityDate() const override {
            return this->datedDate();
        }
        pRateHelper rateHelper(
            const YieldTermStructureHandle& discountingTermStructure = {}   // exogenous discounting curve
        ) const override {
            this->ensureValueIsSet();
            return swapTraits_.makeRateHelper(
                this->rate(),   // quotedFixedRate
                startDate(),    // startDate
                maturityDate(),  // endDate
                discountingTermStructure    // discountingTermStructure - exogenous discounting curve
            );
        }
        // given both estimating and discounting term structures, calculate the swap's implied fair fixed rate
        QuantLib::Rate impliedSwapRate(
            const YieldTermStructureHandle& estimatingTermStructure,
            const YieldTermStructureHandle& discountingTermStructure
        ) const {
            QL_REQUIRE(estimatingTermStructure != YieldTermStructureHandle(), "estimating term structure is required to calculate the implied swap rate");
            QL_REQUIRE(discountingTermStructure != YieldTermStructureHandle(), "discounting term structure is required to calculate the implied swap rate");
            QuantLib::ext::shared_ptr<QuantLib::PricingEngine> swapPricingEngine(new QuantLib::DiscountingSwapEngine(discountingTermStructure));
            auto swap = makeSwap(estimatingTermStructure);  // make a swap with the estimating term structure for the index forecasting
            swap->setPricingEngine(swapPricingEngine);
            auto swapRate = swap->fairRate();
            return swapRate;
        }
        QuantLib::Real impliedQuote(
            const YieldTermStructureHandle& estimatingTermStructure,
            const YieldTermStructureHandle& discountingTermStructure = {}
        ) const override {
            return impliedSwapRate(estimatingTermStructure, discountingTermStructure);
        }
    };

    template<
        typename BASE_SWAP_INDEX
    >
    class VanillaSwapIndex : public SwapIndexBase<VanillaSwapIndexTraits<BASE_SWAP_INDEX>> {
    public:
        VanillaSwapIndex(
            const QuantLib::Period& tenor,
            QuantLib::Date refDate = QuantLib::Date()
        ) :
            SwapIndexBase<VanillaSwapIndexTraits<BASE_SWAP_INDEX>>(tenor, refDate)
        {}
        std::string instrumentTypeAlias() const override {
            return "vanilla swap";
        }
    };

    template<
        typename BASE_SWAP_INDEX
    >
    class OISSwapIndex : public SwapIndexBase<OISSwapIndexTraits<BASE_SWAP_INDEX>> {
    public:
        OISSwapIndex(
            const QuantLib::Period& tenor,
            QuantLib::Date refDate = QuantLib::Date()
        ) :
            SwapIndexBase<OISSwapIndexTraits<BASE_SWAP_INDEX>>(tenor, refDate)
        {}
        std::string instrumentTypeAlias() const override {
            return "OIS swap";
        }
    };

    // basis swap between a base ois swap's overnight leg and a target vanilla swap's floating leg
    // the reason ois swap being the base swap is because ois rates are now considered risk free
    template <
        typename TARGET_VANILLA_SWAP_INDEX,
        typename BASE_OIS_SWAP_INDEX
    >
    class VanillaOISBasisSwapIndex : public SwapCurveInstrument {
    private:
        typedef VanillaSwapIndexTraits<TARGET_VANILLA_SWAP_INDEX> TargetTraitsType;
        typedef OISSwapIndexTraits<BASE_OIS_SWAP_INDEX> BaseTraitsType;
    public:
        typedef TARGET_VANILLA_SWAP_INDEX TargetVanillaSwapIndex;
        typedef BASE_OIS_SWAP_INDEX BaseOISSwapIndex;
        typedef QuantLib::ext::shared_ptr<QuantLib::VanillaSwap> pVanillaSwap;
        typedef QuantLib::ext::shared_ptr<QuantLib::OvernightIndexedSwap> pOvernightIndexedSwap;
    private:
        TargetTraitsType targetSwapTraits_;
        BaseTraitsType baseSwapTraits_;
        bool spreadIsOnBaseLeg_;
        typename TargetTraitsType::FixingResult targetFixingResult_;
        typename BaseTraitsType::FixingResult baseFixingResult_;
        QuantLib::Date baseSwapMaturityDate_;   // maturity date for the base ois swap
        mutable QuantLib::Rate quotedSpreadImpliedTargetSwapFairRate_ = QuantLib::Null<QuantLib::Rate>();
    private:
        // base ois swap factory
        pOvernightIndexedSwap makeBaseSwap(
            QuantLib::Swap::Type type = QuantLib::Swap::Type::Payer,    // swap type
            QuantLib::Rate fixedRate = 0.,  // fixed rate
            QuantLib::Spread overnightSpread = 0.,  // spread added to the overnight leg
            const YieldTermStructureHandle& h = {}    // index estimating term structure
        ) const {
            auto pSwap = baseSwapTraits_.makeSwap(
                baseSwapStartDate(),
                type,   // a payer swap will give the overnight leg a positive npv
                0., // fixed rate on the fixed leg does not matter, only overnight leg matters in basis swap calculation
                overnightSpread,
                h
            );
            return pSwap;
        }
        // target vanilla swap factory
        pVanillaSwap makeTargetSwap(
            QuantLib::Swap::Type type = QuantLib::Swap::Type::Payer,    // swap type
            QuantLib::Rate fixedRate = 0.,  // fixed rate on the fixed leg
            QuantLib::Spread floatSpread = 0.,  // spread added to the floating leg
            const YieldTermStructureHandle& h = {}  // index estimating term structure
        ) const {
            auto pSwap = targetSwapTraits_.makeSwap(
                targetSwapStartDate(),
                type,
                fixedRate,
                floatSpread,
                h
            );
            return pSwap;
        }
    public:
        VanillaOISBasisSwapIndex(
            const QuantLib::Period& tenor,  // tenor of the basis swap
            bool spreadIsOnBaseLeg,   // a flag indicating whether the spread is applying on base swap's overnight leg or targe swap's floating leg
            QuantLib::Date refDate = QuantLib::Date()
        ):
            SwapCurveInstrument(
                nullptr,
                BootstrapInstrument::vtSpread,
                SwapCurveInstrument::BasisSwap,
                tenor
            ),
            targetSwapTraits_(tenor),
            baseSwapTraits_(tenor),
            spreadIsOnBaseLeg_(spreadIsOnBaseLeg),
            targetFixingResult_(targetSwapTraits_.calculateFixing(refDate)),
            baseFixingResult_(baseSwapTraits_.calculateFixing(refDate))
        {
            const auto& swapTraits = this->targetSwapTraits_;
            this->iborIndexFactory_ = [&swapTraits](const YieldTermStructureHandle& h) {
                return swapTraits.makeIborIndex(h);
            };
            QL_REQUIRE(baseSwapTraits_.bothLegsCouponsAligned(true), "for Vanilla-OIS basis swap, the base OIS swap's fixed leg and overnight leg coupons must be aligned");
            QL_REQUIRE(targetSwapTraits_.bothLegsCouponsAligned(true), "for Vanilla-OIS basis swap, the target vanilla swap's fixed leg and floating leg coupons must be aligned");
            QL_REQUIRE(baseSwapFixingDate() == targetSwapFixingDate(), "for Vanilla-OIS basis swap, the base OIS swap's fixing date (" << baseSwapFixingDate() << ") must match the target vanilla swap's fixing date (" << targetSwapFixingDate() << ")");
            baseSwapMaturityDate_ = makeBaseSwap()->maturityDate();  // calculate the base swap maturity date
            this->datedDate() = makeTargetSwap()->maturityDate();  // calculate the target swap maturity date
        }
        bool spreadIsOnBaseLeg() const {
            return spreadIsOnBaseLeg_;
        }
        std::string instrumentTypeAlias() const override {
            return "basis swap";
        }
        std::string valueTypeAlias() const override {
            return "basis spread";
        }
        QuantLib::Date baseSwapFixingDate() const {
            return baseFixingResult_.fixingDate;
        }
        QuantLib::Date baseSwapStartDate() const {
            return baseFixingResult_.startDate;
        }
        QuantLib::Date baseSwapMaturityDate() const {
            return baseSwapMaturityDate_;
        }
        QuantLib::Date targetSwapFixingDate() const {
            return targetFixingResult_.fixingDate;
        }
        QuantLib::Date targetSwapStartDate() const {
            return targetFixingResult_.startDate;
        }
        QuantLib::Date targetSwapMaturityDate() const {
            return this->datedDate();
        }
        QuantLib::Date startDate() const override {
            return targetSwapStartDate();
        }
        QuantLib::Date maturityDate() const override {
            return targetSwapMaturityDate();
        }
        QuantLib::Rate quotedSpreadImpliedTargetSwapFairRate() const {
            return quotedSpreadImpliedTargetSwapFairRate_;
        }
    protected:
        // solver setting functions
        /////////////////////////////////////////////////////////////////
        static QuantLib::Real solverAccuracy() {
            return 1.0e-16;
        }
        static QuantLib::Real solverRateStep() {
            return 0.0010; // 10 bp step
        }
        static QuantLib::Size solverMaxIterations() {
            return 300;
        }
        /////////////////////////////////////////////////////////////////
    public:
        // calculate the fair rate for the base OIS swap implied by the discount term structure
        QuantLib::Rate calculateImpliedBaseSwapFairRate(
            const YieldTermStructureHandle& discountTermStructure // discount term structure
        ) const {
            QuantLib::ext::shared_ptr<QuantLib::PricingEngine> swapPricingEngine(new QuantLib::DiscountingSwapEngine(discountTermStructure));
            auto estimatingTermStructure = discountTermStructure;    // for OIS swap, the index estimating term structure is the same as the discounting term structure
            auto swap = makeBaseSwap(
                QuantLib::Swap::Type::Payer, // type
                0., // fixedRate
                0., // overnightSpread
                estimatingTermStructure // h
            );
            swap->setPricingEngine(swapPricingEngine);
            auto swapRate = swap->fairRate();
            return swapRate;
        }
        // calculate the fair rate for the target vanilla swap implied by the quoted basis spread and discount term structure
        QuantLib::Rate calculateQuotedSpreadImpliedTargetSwapFairRate(
            const YieldTermStructureHandle& discountTermStructure // discount term structure
        ) const {
            QL_REQUIRE(discountTermStructure != YieldTermStructureHandle(), "discounting term structure is required");
            this->ensureValueIsSet();
            QuantLib::ext::shared_ptr<QuantLib::PricingEngine> swapPricingEngine(new QuantLib::DiscountingSwapEngine(discountTermStructure));
            auto quotedBasisSpread = this->spread();
            auto baseSwapOvernightLegSpread = (spreadIsOnBaseLeg() ? quotedBasisSpread : 0.);
            auto baseSwapIndexEstimatingTermStructure = discountTermStructure;    // for OIS swap, the index estimating term structure is the same as the discounting term structure
            auto baseSwap = makeBaseSwap(
                QuantLib::Swap::Type::Payer,    // a payer swap will give the overnight leg a positive npv
                0., // fixedRate - don't care the fixed rate
                baseSwapOvernightLegSpread,
                baseSwapIndexEstimatingTermStructure
            );
            baseSwap->setPricingEngine(swapPricingEngine);
            auto tryToMatchNPV = baseSwap->overnightLegNPV();   // should be positive since it's a payer (receive overnight) swap
            auto baseSwapFairRate = baseSwap->fairRate() - baseSwapOvernightLegSpread;    // fair rate of the base swap without adding the overnight spread
            auto targetSwapFairRateGuess = baseSwapFairRate + (spreadIsOnBaseLeg() ? quotedBasisSpread : -quotedBasisSpread);
            const auto& me = *this;
            auto f = [
                &me,
                &discountTermStructure,
                &quotedBasisSpread,
                &tryToMatchNPV,
                &swapPricingEngine
            ](const QuantLib::Rate& targetSwapFairRate) -> QuantLib::Real {
                auto fixedRate = targetSwapFairRate + (me.spreadIsOnBaseLeg() ? 0. : quotedBasisSpread);
                auto targetSwapIndexEstimatingTermStructure = discountTermStructure;    // since target vanilla swap's floating leg does not matter, just use an arbitrary term structure for index estimating
                auto targetSwap = me.makeTargetSwap(
                    QuantLib::Swap::Type::Receiver, // a receiver swap will give the fixed leg a positive npv
                    fixedRate,  // fixedRate
                    0., // floatSpread - don't care the floating spread
                    targetSwapIndexEstimatingTermStructure
                );
                targetSwap->setPricingEngine(swapPricingEngine);
                auto targetSwapFixedLegNPV = targetSwap->fixedLegNPV();    // should be positive since it's a receiver (receive fixed) swap
                return targetSwapFixedLegNPV - tryToMatchNPV;
            };
            QuantLib::Brent solver;
            solver.setMaxEvaluations(solverMaxIterations());
            QuantLib::Rate targetSwapFairRate = solver.solve(f, solverAccuracy(), targetSwapFairRateGuess, solverRateStep());
            return targetSwapFairRate;
        }
        pRateHelper rateHelper(
            const YieldTermStructureHandle& discountingTermStructure = {} 
        ) const override {
            quotedSpreadImpliedTargetSwapFairRate_ = calculateQuotedSpreadImpliedTargetSwapFairRate(discountingTermStructure);
            auto helper = targetSwapTraits_.makeRateHelper(
                quotedSpreadImpliedTargetSwapFairRate_,
                targetSwapStartDate(),
                targetSwapMaturityDate(),
                discountingTermStructure
            );
            return helper;
        }
        QuantLib::Spread impliedBasisSpread(
            const YieldTermStructureHandle& estimatingTermStructure,
            const YieldTermStructureHandle& discountingTermStructure
        ) const {
            QL_REQUIRE(estimatingTermStructure != YieldTermStructureHandle(), "estimating term structure is required to calculate the implied basis spread");
            QL_REQUIRE(discountingTermStructure != YieldTermStructureHandle(), "discounting term structure is required to calculate the implied basis spread");
            QuantLib::ext::shared_ptr<QuantLib::PricingEngine> swapPricingEngine(new QuantLib::DiscountingSwapEngine(discountingTermStructure));
            const auto& me = *this;
            auto f = [
                &me,
                &estimatingTermStructure,
                &discountingTermStructure,
                &swapPricingEngine
            ](const QuantLib::Spread& spread) -> QuantLib::Real {
                // setup a base payer OIS swap
                ////////////////////////////////////////////////////////////////////////
                auto baseSwapIndexEstimatingTermStructure = discountingTermStructure;   // for OIS swap, the index estimating term structure is the same as the discounting term structure
                auto baseSwapOvernightLegSpread = (me.spreadIsOnBaseLeg() ? spread : 0.);
                auto baseSwap = me.makeBaseSwap(
                    QuantLib::Swap::Type::Payer,    // a payer OIS swap will give the overnight leg a positive npv
                    0., // fixedRate - don't care the fixed rate since we only want the npv of the overnight leg
                    baseSwapOvernightLegSpread, // overnightSpread
                    baseSwapIndexEstimatingTermStructure
                );
                baseSwap->setPricingEngine(swapPricingEngine);
                auto npv_overnight = baseSwap->overnightLegNPV();   // should be positive since it's a payer (receive overnight) swap
                ////////////////////////////////////////////////////////////////////////
                // setup a target payer vanilla swap
                ////////////////////////////////////////////////////////////////////////
                auto targetSwapIndexEstimatingTermStructure = estimatingTermStructure;
                auto targetSwapFloatingLegSpread = (me.spreadIsOnBaseLeg() ? 0. : spread);
                auto targetSwap = me.makeTargetSwap(
                    QuantLib::Swap::Type::Payer,    // a payer vanilla swap will give the floating leg a positive npv
                    0., // fixedRate - don't care the fixed rate since we only want the npv of the floating leg
                    targetSwapFloatingLegSpread,    // floatSpread
                    targetSwapIndexEstimatingTermStructure
                );
                targetSwap->setPricingEngine(swapPricingEngine);
                auto npv_floating = targetSwap->floatingLegNPV(); // should be positive since it's a payer (receive floatiing) swap
                ////////////////////////////////////////////////////////////////////////
                return npv_floating - npv_overnight;
            };
            QuantLib::Real guessBasisSpread = 0.;
            QuantLib::Brent solver;
            solver.setMaxEvaluations(solverMaxIterations());
            QuantLib::Spread impliedSpread = solver.solve(f, solverAccuracy(), guessBasisSpread, solverRateStep());
            return impliedSpread;
        }
        QuantLib::Real impliedQuote(
            const YieldTermStructureHandle& estimatingTermStructure,
            const YieldTermStructureHandle& discountingTermStructure = {}
        ) const override {
            return impliedBasisSpread(estimatingTermStructure, discountingTermStructure);
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
                QuantLib::Period(TENOR_MONTHS, QuantLib::Months),    // tenor
                QuantLib::Thirty360(THIRTY_360_DC_CONVENTION),    // day counter
                COMPOUNDING,    // compounding
                FREQUENCY   // frequency
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
