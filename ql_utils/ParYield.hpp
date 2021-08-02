#pragma once

#include <ql/quantlib.hpp>
#include <tuple>

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

    template <template<typename> typename COMP = std::greater, QuantLib::Natural COUPON_CUTOFF_MONTHS = 12>
    struct ParBondTenorCouponedWithCutoff {
        bool operator() (const QuantLib::Natural& tenorMonths) const {
            COMP<QuantLib::Natural> compare;
            return compare(tenorMonths, COUPON_CUTOFF_MONTHS);
        }
    };

    template <QuantLib::Frequency FREQ = QuantLib::Semiannual, typename TENOR_COUPONED = ParBondTenorCouponedWithCutoff<>>
    class ParYieldHelper {
    protected:
        QuantLib::Natural tenorMonths_;
        QuantLib::Rate parYield_;
    public:
        ParYieldHelper(QuantLib::Natural tenorMonths) : tenorMonths_(tenorMonths), parYield_(QuantLib::Null<QuantLib::Rate>()) {}
        const QuantLib::Rate& parYield() const { return parYield_; }
        const QuantLib::Natural& tenorMonth() const { return tenorMonths_; }
        ParYieldHelper& withParYield(const QuantLib::Rate& parYield) {
            parYield_ = parYield;
            return *this;
        }
        static bool tenorIsCouponed(const QuantLib::Natural& tenorMonths) {
            TENOR_COUPONED tenorCouponed;
            return tenorCouponed(tenorMonths);
        }
        static QuantLib::Frequency frequency() {
            return FREQ;
        }
        // returns number of months between cashflows
        static QuantLib::Natural cashflowTenorMonths() {
            return 12 / (QuantLib::Natural)frequency();
        }
        // returns day counter for the par bond calculation
        static QuantLib::DayCounter parBondDayCounter() {
            return QuantLib::Thirty360(QuantLib::Thirty360::BondBasis);
        }
    private:
        // given tenor in months return par bond schedule, bond settlementDate, and bond maturityDate
        static std::tuple<QuantLib::Schedule, QuantLib::Date, QuantLib::Date, QuantLib::Integer> getParBondSchedule(QuantLib::Natural tenorMonths, const QuantLib::Period& settlePeriod = QuantLib::Period(0, QuantLib::Days)) {
            auto cfTenorMonths = cashflowTenorMonths();
            QuantLib::Date evalDate = QuantLib::Settings::instance().evaluationDate();
            auto settlementDate = evalDate + settlePeriod;
            auto settlementDays = settlementDate - evalDate;
            auto bondMaturityDate = settlementDate + tenorMonths * QuantLib::Months;
            bool endOfMonth = false;
            auto bondStartDate = settlementDate - (tenorMonths % cfTenorMonths == 0 ? 0 : cfTenorMonths - tenorMonths % cfTenorMonths) * QuantLib::Months;
            QuantLib::Schedule schedule(bondStartDate, bondMaturityDate, QuantLib::Period(frequency()), QuantLib::NullCalendar(), QuantLib::Unadjusted, QuantLib::Unadjusted, QuantLib::DateGeneration::Backward, endOfMonth);
            return std::tuple<QuantLib::Schedule, QuantLib::Date, QuantLib::Date, QuantLib::Integer>(schedule, settlementDate, bondMaturityDate, settlementDays);
        }
    public:
        // create spot FixedRateBondHelper for discount curve bootstraping (par yield => zero curve)
        operator QuantLib::ext::shared_ptr<QuantLib::FixedRateBondHelper>() const {
            QL_REQUIRE(tenorMonths_ > 0, "tenor month must be greater than 0");
            QL_REQUIRE(parYield_ != QuantLib::Null<QuantLib::Rate>(), "par yield not set");
            auto ret = getParBondSchedule(tenorMonths_); // settle on the spot
            auto const& schedule = std::get<0>(ret);
            auto const& settlementDate = std::get<1>(ret);
            auto const& maturityDate = std::get<2>(ret);
            auto const& settlementDays = std::get<3>(ret);
            QuantLib::DayCounter dc = parBondDayCounter();
            QL_ASSERT(settlementDays == 0, "spot settle must have zero settlementDays");
            QuantLib::Real notional = 100.0;
            QuantLib::Real price = QuantLib::Null<QuantLib::Real>();
            QuantLib::Rate coupon = QuantLib::Null<QuantLib::Rate>();
            if (tenorIsCouponed(tenorMonths_)) {   // couponed bond
                price = notional;  // price at par => coupon is par yield
                coupon = parYield_;  // coupon is par yield
            }
            else {  // zero coupon bond
                // df = 1.0/pow(1 + yield/freq, yearFraction * freq), price = df * notional
                QuantLib::InterestRate ir(parYield_, dc, QuantLib::Compounding::Compounded, frequency());
                auto discountFactor = ir.discountFactor(settlementDate, maturityDate);
                price = discountFactor * notional; // zero coupon bond sold at a discount
                coupon = 0.0;  // zero coupon bond
            }
            auto quote = QuantLib::ext::make_shared<QuantLib::SimpleQuote>(price);
            return QuantLib::ext::shared_ptr<QuantLib::FixedRateBondHelper>(new QuantLib::FixedRateBondHelper(QuantLib::Handle<QuantLib::Quote>(quote), settlementDays, notional, schedule, std::vector<QuantLib::Rate>(1, coupon), dc, QuantLib::Unadjusted, notional));
        }
        // calculate par yield for the given the discounting term structure (zero curve => par yield)
        static QuantLib::Rate parYield(const QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure>& discountTermStructure, QuantLib::Natural tenorMonths, const QuantLib::Period& forwardTerm = QuantLib::Period(0, QuantLib::Days)) {
            QL_REQUIRE(tenorMonths > 0, "tenor month must be greater than 0");
            QL_REQUIRE(discountTermStructure != nullptr, "discount term structure not set");
            auto ret = getParBondSchedule(tenorMonths, forwardTerm);
            auto const& schedule = std::get<0>(ret);
            auto const& settlementDate = std::get<1>(ret);
            auto const& maturityDate = std::get<2>(ret);
            auto const& settlementDays = std::get<3>(ret);
            QuantLib::DayCounter dc = parBondDayCounter();
            if (tenorIsCouponed(tenorMonths)) { // couponed bond
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