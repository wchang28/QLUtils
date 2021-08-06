#pragma once

#include <ql/quantlib.hpp>

namespace QLUtils {
    // par coupon solver functor for fixed rate couponed bond
    // given a settlement days, fixed rate bond schedule, accrual day counter, and a bond pricing engine, can we find it's par coupon?
    template <QuantLib::Natural NOTIONAL = 100>
    class ParCouponSolverFunctor {
    public:
        ParCouponSolverFunctor(QuantLib::ext::shared_ptr<QuantLib::PricingEngine> pricingEngine, QuantLib::Natural settlementDays, const QuantLib::Schedule& sched, const QuantLib::DayCounter& accrualDayCounter)
            : pricingEngine_(pricingEngine),
            settlementDays_(settlementDays),
            sched_(sched),
            accrualDayCounter_(accrualDayCounter)
        {
            QL_REQUIRE(pricingEngine_ != nullptr, "bond pricing engine not set");
        }
        QuantLib::Real operator()(QuantLib::Rate coupon) const {
            auto notional = (QuantLib::Real)NOTIONAL;
            QuantLib::FixedRateBond bond(settlementDays_, notional, sched_, std::vector<QuantLib::Rate>(1, coupon), accrualDayCounter_, QuantLib::Unadjusted, notional);
            bond.setPricingEngine(pricingEngine_);
            auto targetCleanPrice = notional;    // target clean price at par
            return bond.cleanPrice() - targetCleanPrice;
        }
    private:
        QuantLib::ext::shared_ptr<QuantLib::PricingEngine> pricingEngine_;
        QuantLib::Natural settlementDays_;
        QuantLib::Schedule sched_;
        QuantLib::DayCounter accrualDayCounter_;
    };

    // functor that determines whether the given bond tenor is a par couponed bond based on a cutoff months
    template <template<typename> typename COMP = std::greater, QuantLib::Natural COUPON_CUTOFF_MONTHS = 12>
    struct ParBondTenorCouponedWithCutoffMonths {
        // returns true if the given tenor is couponed, false otherwise
        bool operator() (const QuantLib::Period& tenor) const {
            COMP<QuantLib::Period> compare;
            QuantLib::Period tenorCutoff = COUPON_CUTOFF_MONTHS * QuantLib::Months;
            return compare(tenor, tenorCutoff);
        }
    }
    ;
    // calculate theoretical (spot or forward) bond schedule and settlement date
    template <QuantLib::Frequency FREQ = QuantLib::Semiannual>
    class TheoreticalBondSchedule {
    protected:
        QuantLib::Date settlementDate_;
        QuantLib::Natural settlementDays_;
        QuantLib::Date maturityDate_;
        QuantLib::Date startDate_;
        QuantLib::Schedule schedule_;
    public:
        QuantLib::Period tenor;
        QuantLib::Period settlePeriod;

        static QuantLib::Frequency frequency() {
            return FREQ;
        }
        // returns number of months between cashflows
        static QuantLib::Natural cashflowTenorMonths() {
            return 12 / (QuantLib::Natural)frequency();
        }
    protected:
        void initialize() {
            QuantLib::Date today = QuantLib::Settings::instance().evaluationDate();
            settlementDate_ = today + settlePeriod;
            settlementDays_ = settlementDate_ - today;
            maturityDate_ = settlementDate_ + tenor;
            // calculate theoretical start date of the bond
            auto cfTenorMonths = cashflowTenorMonths();
            auto d = maturityDate_; // starting with the maturity and project backward
            while (d > settlementDate_) {
                d -= cfTenorMonths * QuantLib::Months;
            }
            startDate_ = d;
            schedule_ = QuantLib::Schedule(startDate_, maturityDate_, QuantLib::Period(frequency()), QuantLib::NullCalendar(), QuantLib::Unadjusted, QuantLib::Unadjusted, QuantLib::DateGeneration::Backward, false);
        }
    public:
        TheoreticalBondSchedule(
            const QuantLib::Period& tenor_,    // tenor of the bonds - time to maturity from settlement
            const QuantLib::Period& forwardSettlePeriod = QuantLib::Period(0, QuantLib::Days)   // forward settle period
        ) : tenor(tenor_), settlePeriod(forwardSettlePeriod) {
            QL_REQUIRE(tenor_.length() > 0, "tenor must be positive in time");
            QL_REQUIRE(forwardSettlePeriod.length() >= 0, "forward settle cannot be negative in time");
            initialize();
        }
        const QuantLib::Date& settlementDate() const {
            return settlementDate_;
        }
        const QuantLib::Natural& settlementDays() const {
            return settlementDays_;
        }
        const QuantLib::Date& maturityDate() const {
            return maturityDate_;
        }
        const QuantLib::Date& startDate() const {   // theoretical start date of the bond
            return startDate_;
        }
        const QuantLib::Schedule& schedule() const {
            return schedule_;
        }
    };

    template <QuantLib::Frequency FREQ = QuantLib::Semiannual, typename TENOR_COUPONED = ParBondTenorCouponedWithCutoffMonths<>>
    class ParYieldHelper {
    protected:
        QuantLib::Period tenor_;
        QuantLib::Rate parYield_;
    public:
        ParYieldHelper(const QuantLib::Period& tenor)
            : tenor_(tenor),
            parYield_(QuantLib::Null<QuantLib::Rate>()) {}
        const QuantLib::Period& tenor() const { return tenor_; }
        const QuantLib::Rate& parYield() const { return parYield_; }
        ParYieldHelper& withParYield(const QuantLib::Rate& parYield) {
            parYield_ = parYield;
            return *this;
        }
        static bool tenorIsCouponed(const QuantLib::Period& tenor) {
            TENOR_COUPONED tenorCouponed;
            return tenorCouponed(tenor);
        }
        static QuantLib::Frequency frequency() {
            return TheoreticalBondSchedule<FREQ>::frequency();
        }
        // returns number of months between cashflows
        static QuantLib::Natural cashflowTenorMonths() {
            return TheoreticalBondSchedule<FREQ>::cashflowTenorMonths();
        }
        // returns day counter for the par bond calculation
        static QuantLib::DayCounter parBondDayCounter() {
            return QuantLib::Thirty360(QuantLib::Thirty360::BondBasis);
        }
    public:
        // create spot FixedRateBondHelper for discount curve bootstraping (par yield => zero curve)
        operator QuantLib::ext::shared_ptr<QuantLib::FixedRateBondHelper>() const {
            QL_REQUIRE(parYield_ != QuantLib::Null<QuantLib::Rate>(), "par yield not set");
            TheoreticalBondSchedule<FREQ> theoreticalBondDef(tenor_); // for theoretical bond settle on the spot
            auto const& schedule = theoreticalBondDef.schedule();
            auto const& settlementDate = theoreticalBondDef.settlementDate();
            auto const& maturityDate = theoreticalBondDef.maturityDate();
            auto const& settlementDays = theoreticalBondDef.settlementDays();
            QuantLib::DayCounter dc = parBondDayCounter();
            QuantLib::Real notional = 100.0;
            QuantLib::Rate coupon = QuantLib::Null<QuantLib::Rate>();
            QuantLib::Real targetPrice = QuantLib::Null<QuantLib::Real>();    // target price
            if (tenorIsCouponed(tenor_)) {   // couponed bond
                coupon = parYield_;  // for par bond, coupon is par yield
                targetPrice = notional;  // target the price at par
            }
            else {  // zero coupon bond
                coupon = 0.0;  // zero coupon bond
                // df = 1.0/pow(1 + yield/freq, yearFraction * freq), price = df * notional
                QuantLib::InterestRate ir(parYield_, dc, QuantLib::Compounding::Compounded, frequency());
                auto discountFactor = ir.discountFactor(settlementDate, maturityDate);
                targetPrice = discountFactor * notional; // zero coupon bond is sold at a discount, so this is the target price
            }
            auto quote = QuantLib::ext::make_shared<QuantLib::SimpleQuote>(targetPrice);    // make target price the quote
            return QuantLib::ext::shared_ptr<QuantLib::FixedRateBondHelper>(new QuantLib::FixedRateBondHelper(QuantLib::Handle<QuantLib::Quote>(quote), settlementDays, notional, schedule, std::vector<QuantLib::Rate>(1, coupon), dc, QuantLib::Unadjusted, notional));
        }
        QuantLib::ext::shared_ptr<QuantLib::Bond> parBond() {
            auto helper = this->operator boost::shared_ptr<QuantLib::FixedRateBondHelper>();
            return helper->bond();
        }
        // calculate par yield for the given the discounting term structure (zero curve => par yield)
        static QuantLib::Rate parYield(const QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure>& discountTermStructure, const QuantLib::Period& tenor, const QuantLib::Period& forwardTerm = QuantLib::Period(0, QuantLib::Days)) {
            QL_REQUIRE(discountTermStructure != nullptr, "discount term structure not set");
            TheoreticalBondSchedule<FREQ> theoreticalBondDef(tenor, forwardTerm);
            auto const& schedule = theoreticalBondDef.schedule();
            auto const& settlementDate = theoreticalBondDef.settlementDate();
            auto const& maturityDate = theoreticalBondDef.maturityDate();
            auto const& settlementDays = theoreticalBondDef.settlementDays();
            QuantLib::DayCounter dc = parBondDayCounter();
            if (tenorIsCouponed(tenor)) { // couponed bond
                QuantLib::RelinkableHandle<QuantLib::YieldTermStructure> discountCurve;
                QuantLib::ext::shared_ptr<QuantLib::PricingEngine> pricingEngine(new QuantLib::DiscountingBondEngine(discountCurve));
                discountCurve.linkTo(discountTermStructure);
                ParCouponSolverFunctor<> f(pricingEngine, settlementDays, schedule, dc);
                QuantLib::Solver1D<QuantLib::Bisection> solver;
                QuantLib::Real accuracy = 1.0e-16;
                QuantLib::Real guess = discountTermStructure->zeroRate(maturityDate, dc, QuantLib::Compounding::Compounded, frequency());
                solver.setMaxEvaluations(200);
                QuantLib::Rate parYield = solver.solve(f, accuracy, guess, guess / 10.0);
                return parYield;
            }
            else {  // zero coupon bond => no need to use solver
                auto dfStart = discountTermStructure->discount(settlementDate);
                auto dfEnd = discountTermStructure->discount(maturityDate);
                auto compound = dfStart / dfEnd;
                auto ir = QuantLib::InterestRate::impliedRate(compound, dc, QuantLib::Compounding::Compounded, frequency(), settlementDate, maturityDate);
                auto parYield = ir.rate();
                return parYield;
            }
        }
    };
}