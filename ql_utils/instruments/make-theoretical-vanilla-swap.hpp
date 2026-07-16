#pragma once

#include <ql/quantlib.hpp>

namespace QuantLib {
    class MakeTheoreticalVanillaSwap {
    protected:
        Frequency couponFreq_ = Frequency::Annual;
        Size numCoupons_;
        Handle<YieldTermStructure> h_;
        Date fixingDate_;
        Rate fixedRate_ = Null<Rate>();
        Swap::Type type_ = Swap::Type::Payer;
        Real nominal_ = 1.0;
        Spread spread_ = 0.0;
    public:
        MakeTheoreticalVanillaSwap(
            Frequency couponFreq,
            Size numCoupons,
            Handle<YieldTermStructure> h = {},    // estimating term structure
            Rate fixedRate = Null<Rate>()
        ) : couponFreq_(couponFreq), numCoupons_(numCoupons), h_(h), fixedRate_(fixedRate)
        {
            QL_REQUIRE(numCoupons_ > 0, "Number of coupons (" << numCoupons << ") must be positive");
        }
        MakeTheoreticalVanillaSwap& withFixingDate(const Date& fixingDate) {
            fixingDate_ = fixingDate;
            return *this;
        }
        MakeTheoreticalVanillaSwap& withFixedRate(Rate fixedRate) {
            fixedRate_ = fixedRate;
            return *this;
        }
        MakeTheoreticalVanillaSwap& withType(Swap::Type type) {
            type_ = type;
            return *this;
        }
        MakeTheoreticalVanillaSwap& withSpread(Spread spread) {
            spread_ = spread;
            return *this;
        }
        MakeTheoreticalVanillaSwap& withNominal(Real nominal) {
            nominal_ = nominal;
            return *this;
        }
        operator ext::shared_ptr<VanillaSwap>() const {
            Period couponTenor = Period(couponFreq_);
            Date fixingDate = fixingDate_;
            if (fixingDate == Null<Date>()) {
                fixingDate = (!h_.empty() ? h_->referenceDate() : Settings::instance().evaluationDate());
            }
            QL_REQUIRE(fixingDate != Date(), "swap fixing date is invalid");
            Date startDate = fixingDate;
            std::vector<Date> dates;
            dates.push_back(startDate);
            for (Size i = 0; i < numCoupons_; ++i) {
                Date dt = startDate + couponTenor * (i + 1);
                dates.push_back(dt);
            }
            Schedule schedule(
                dates,
                NullCalendar(),    // calendar
                BusinessDayConvention::Unadjusted,    // convention
                BusinessDayConvention::Unadjusted,    // terminationDateConvention
                couponTenor, // tenor
                DateGeneration::Rule::Forward,    // rule
                false // endOfMonth
            );    // schedule for both legs
            DayCounter dayCounter = ActualActual(ActualActual::Bond, schedule);    // fixing day counter for both legs and for the ibor index
            ext::shared_ptr<IborIndex> iborIndex(new IborIndex(
                "TheoreticalIborIndex",    // familyName
                couponTenor,    // tenor
                0, // settlementDays
                Currency(),    // currency
                NullCalendar(),
                BusinessDayConvention::Unadjusted,    // convention
                false,    // endOfMonth
                dayCounter,    // dayCounter
                h_    // estimating term structure
            ));
            Swap::Type type = type_;
            Real nominal = nominal_;
            Spread spread = spread_;
            auto makeSwap = [&type, &schedule, &iborIndex, &dayCounter, &nominal, &spread](Rate fixedRate) {
                return ext::shared_ptr<VanillaSwap>(new VanillaSwap(
                    type,    // type
                    nominal, // nominal,
                    schedule,    // fixedSchedule
                    fixedRate,    // fixedRate
                    dayCounter,    // fixedDayCount
                    schedule,    // floatSchedule
                    iborIndex,    // iborIndex
                    spread,    // spread
                    dayCounter    // floatingDayCount
                ));
            };
            Rate fixedRate = fixedRate_;
            if (fixedRate == Null<Rate>()) {
                if (!h_.empty()) {
                    ext::shared_ptr<VanillaSwap> swap = makeSwap(0.0);
                    ext::shared_ptr<PricingEngine> engine(new DiscountingSwapEngine(h_));
                    swap->setPricingEngine(engine);
                    fixedRate = swap->fairRate();
                }
                else {
                    fixedRate = 0.0;
                }
            }
            QL_ASSERT(fixedRate != Null<Rate>(), "swap fixed rate is null");
            return makeSwap(fixedRate);
        }
    };
}
