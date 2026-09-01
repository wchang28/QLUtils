#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/utilities/time.hpp>
#include <ql_utils/utilities/iso-date-conv.hpp>
#include <vector>
#include <map>
#include <memory>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <utility>
#include <functional>
#include <set>
#include <exception>
#include <iomanip>
#include <sstream>

namespace QuantLib {
    namespace Utils {
        class DatedMonthlyTimeGrid {
        public:
            typedef Size TimeIndex;
            typedef Size MonthNumber;
            typedef std::shared_ptr<TimeGrid> TimeGridPtr;
            typedef std::shared_ptr<DatedMonthlyTimeGrid> DatedMonthlyTimeGridPtr;
            typedef ext::shared_ptr<VanillaSwap> VanillaSwapPtr;
            typedef Handle<YieldTermStructure> YieldTermStructureHandle;
            typedef std::vector<DiscountFactor> DiscountVector;
            typedef Schedule::const_iterator const_iterator;
            typedef std::function<std::pair<bool, Period>(const TimeIndex&)> ImportantTenorFilter;
        protected:
            DayCounter dayCounter_;   // day counter for the time grid
            Schedule schedule_;   // grid date schedule
            std::map<Date::serial_type, TimeIndex> dateToTimeIndex_;    // map from grid date to grid time index for fast date to time index lookup
            TimeGridPtr pTimeGrid_; // internal time grid
            MonthNumber maxFwdMonthNumber_; // max forward month number allowed for the time grid system
        private:
            static Schedule makeGridSchedule(
                const std::vector<Date>& gridDates
            ) {
                return Schedule(
                    gridDates, // dates
                    NullCalendar(),   // calendar
                    BusinessDayConvention::Unadjusted,    // convention
                    BusinessDayConvention::Unadjusted,    // terminationDateConvention
                    1 * TimeUnit::Months,    // tenor
                    DateGeneration::Rule::Forward, // rule
                    false   // endOfMonth
                );
            }
            // make a forward vanilla swap leg (for both legs) schedule for the given forward, swap tenor and coupon tenor on this grid
            // returns empty schedule if the forward swap legs schedule cannot be made on this grid
            Schedule makeFwdSwapLegSchedule(
                const Period& forward,
                const Period& swapTenor,
                const Period& couponTenor
            ) const {
                auto [is_mult_f, m_1] = isMultiple(forward, 1 * Months);
                QL_REQUIRE(is_mult_f, "forward period (" << forward << ") must be a multiple of 1 month");
                MonthNumber fwdMonths = m_1;
                QL_REQUIRE(fwdMonths >= 0, "forward period (" << forward << ") must be greater than or equal to 0");
                auto [is_mult_s, m_2] = isMultiple(swapTenor, 1 * Months);
                QL_REQUIRE(is_mult_s, "swap tenor (" << swapTenor << ") must be a multiple of 1 month");
                MonthNumber swapTenorMonths = m_2;
                QL_REQUIRE(swapTenorMonths > 0, "swap tenor (" << swapTenor << ") must be greater than 0");
                auto [is_mult_c, m_3] = isMultiple(couponTenor, 1 * Months);
                QL_REQUIRE(is_mult_c, "coupon tenor (" << couponTenor << ") must be a multiple of 1 month");
                MonthNumber couponTenorMonths = m_3;
                QL_REQUIRE(couponTenorMonths > 0, "coupon tenor (" << couponTenor << ") must be greater than 0");
                auto [is_mult_nc, m_4] = isMultiple(swapTenor, couponTenor);
                QL_REQUIRE(is_mult_nc, "swap tenor (" << swapTenor << ") must be a multiple of coupon tenor (" << couponTenor << ")");
                Size numCoupons = m_4;
                QL_REQUIRE(numCoupons > 0, "number of coupons (" << numCoupons << ") must be greater than 0");
                MonthNumber swapMaturityMonths = fwdMonths + swapTenorMonths;
                auto forwardAllowed = (fwdMonths <= this->maxFwdMonth());
                auto swapMaturityInBound = (swapMaturityMonths <= this->maxMonth());
                if (forwardAllowed && swapMaturityInBound) {    // swap schedule can be made on this grid
                    std::vector<Date> dates;
                    for (MonthNumber month = fwdMonths; month <= swapMaturityMonths; month += couponTenorMonths) {
                        TimeIndex timeIndex = month;
                        dates.push_back(this->at(timeIndex));
                    }
                    QL_ASSERT(dates.size() == numCoupons + 1, "swap schedule size (" << dates.size() << ") is not what's expected (" << (numCoupons + 1) << ")");
                    return Schedule(
                        dates, // dates
                        NullCalendar(),   // calendar
                        BusinessDayConvention::Unadjusted,    // convention
                        BusinessDayConvention::Unadjusted,    // terminationDateConvention
                        couponTenorMonths * TimeUnit::Months,    // tenor
                        DateGeneration::Rule::Forward, // rule
                        false   // endOfMonth
                    );
                }
                else {  // swap schedule cannot be made on this grid
                    return {};  // return empty schedule
                }
            }
        public:
            DatedMonthlyTimeGrid(
                const DayCounter& dayCounter, // grid day counter
                const std::vector<Date>& gridDates,  // grid dates
                MonthNumber maxFwdMonthNumber = Null<MonthNumber>()  // max forward month number (optional)
            ) :
                dayCounter_(dayCounter),
                schedule_(makeGridSchedule(gridDates)),
                maxFwdMonthNumber_(maxFwdMonthNumber)
            {
                if (schedule_.size() > 0) {
                    // build grid times
                    std::vector<Time> gridTimes(schedule_.size());
                    for (TimeIndex timeIndex = 0; timeIndex < schedule_.size(); ++timeIndex) {   // for every month
                        const auto& date = schedule_.at(timeIndex);
                        gridTimes[timeIndex] = dayCounter_.yearFraction(schedule_.at(0), date);
                        dateToTimeIndex_[date.serialNumber()] = timeIndex;
                    }
                    pTimeGrid_.reset(new TimeGrid(gridTimes.begin(), gridTimes.end()));
                }
            }
            virtual ~DatedMonthlyTimeGrid() = default;
            MonthNumber maxFwdMonth() const {
                return (maxFwdMonthNumber_ == Null<MonthNumber>() ? maxMonth() : std::min(maxFwdMonthNumber_, maxMonth()));
            }
            Date maxFwdDate() const {
                return this->date(this->maxFwdMonth());
            }
            bool isFwdAllowed(
                TimeIndex timeIndex
            ) const {
                return (timeIndex <= this->maxFwdMonth());
            }
            // makes at-the-money forward vanilla swap
            // return nullptr if swap goes out of the bound of the time grid
            VanillaSwapPtr makeFwdATMVanillaSwap(
                const Period& forward,
                const Period& swapTenor,
                const Period& couponTenor,
                const DayCounter& legsDayCounter,
                Swap::Type swapType,
                const YieldTermStructureHandle& hTS // index estimating term structure handle
            ) const {
                QL_REQUIRE(!hTS.empty(), "estimating term structure handle is empty");
                Schedule schedule = makeFwdSwapLegSchedule(forward, swapTenor, couponTenor);
                if (!schedule.empty()) {    // swap schedule can be made on this grid
                    auto indexTenor = couponTenor;  // use the same tenor for the index as the legs
                    auto indexDayCounter = legsDayCounter;  // use the same day counter for the index as the legs
                    ext::shared_ptr<IborIndex> iborIndex(new IborIndex(
                        "TheoreticalIborIndex",    // familyName
                        indexTenor,    // tenor
                        0, // settlementDays
                        Currency(),    // currency
                        NullCalendar(), // fixingCalendar
                        BusinessDayConvention::Unadjusted,    // convention
                        false,    // endOfMonth
                        indexDayCounter,    // dayCounter
                        hTS    // estimating term structure
                    ));
                    auto makeSwap = [&swapType, &schedule, &iborIndex, &legsDayCounter](Rate fixedRate) {
                        Real nominal = 1.0;
                        Spread spread = 0.0;
                        return VanillaSwapPtr(new VanillaSwap(
                            swapType,    // type
                            nominal, // nominal,
                            schedule,    // fixedSchedule
                            fixedRate,    // fixedRate
                            legsDayCounter,    // fixedDayCount
                            schedule,    // floatSchedule
                            iborIndex,    // iborIndex
                            spread,    // spread
                            legsDayCounter    // floatingDayCount
                        ));
                    };
                    VanillaSwapPtr swap = makeSwap(0.0);
                    ext::shared_ptr<PricingEngine> engine(new DiscountingSwapEngine(hTS));
                    swap->setPricingEngine(engine);
                    auto fixedRate = swap->fairRate();
                    return makeSwap(fixedRate);
                }
                else {  // swap schedule cannot be made on this grid
                    return nullptr; // return null pointer
                }
            }
            const DayCounter& dayCounter() const { return dayCounter_; }
            MonthNumber minMonth() const { return 0; }
            MonthNumber maxMonth() const {
                QL_REQUIRE(!schedule_.empty(), "grid is empty");
                return schedule_.size() - 1;
            }
            Size nTimes() const { return schedule_.size(); }
            Size nSteps() const {
                QL_REQUIRE(!schedule_.empty(), "grid is empty");
                return schedule_.size() - 1;
            }
            const std::vector<Date>& dates() const { return schedule_.dates(); }
            bool empty() const { return schedule_.empty(); }
            const Date& front() const {
                QL_REQUIRE(!schedule_.empty(), "grid is empty");
                return schedule_.front();
            }
            const Date& back() const {
                QL_REQUIRE(!schedule_.empty(), "grid is empty");
                return schedule_.back();
            }
            Date referenceDate() const { return this->front(); }
            const_iterator begin() const { return schedule_.begin(); }
            const_iterator end() const { return schedule_.end(); }
            Size size() const { return schedule_.size(); }
            // Check if the specified date is a grid date in the time grid system.
            bool isGridDate(
                const Date& date
            ) const {
                return dateToTimeIndex_.find(date.serialNumber()) != dateToTimeIndex_.end();
            }
            // Get the time index of the specified date in the grid. If the date is not a grid date, an exception will be thrown.
            TimeIndex timeIndex(
                const Date& date
            ) const {
                auto iter = dateToTimeIndex_.find(date.serialNumber());
                QL_REQUIRE(iter != dateToTimeIndex_.end(), "date (" << ISODateConv::to_str(date) << ") is not a grid date");
                return iter->second;
            }
            // returns grid time for the date
            Time timeForDate(
                const Date& date
            ) const {
                auto timeIndex = this->timeIndex(date);
                return pTimeGrid_->at(timeIndex);
            }
            // returns grid date for the time index
            const Date& at(
                TimeIndex timeIndex
            ) const {
                QL_REQUIRE(timeIndex < schedule_.size(), "time index (" << timeIndex << ") is out of the range [0, " << schedule_.size() << ")");
                return schedule_[timeIndex];
            }
            // returns grid date for the time index
            const Date& operator[] (
                TimeIndex timeIndex
                ) const {
                return this->at(timeIndex);
            }
            // returns grid date for the time index
            const Date& date(
                TimeIndex timeIndex
            ) const {
                return this->at(timeIndex);
            }
            // month number is the same as the time index in the monthly grid system
            static MonthNumber monthNumber(
                TimeIndex timeIndex
            ) {
                return timeIndex;
            }
            // returns the month-unit tenor for a grid date
            Period tenor(
                const Date& date
            ) const {
                auto timeIndex = this->timeIndex(date);
                return Period(this->monthNumber(timeIndex), TimeUnit::Months); // time index is the same as the month number in the monthly grid system
            }
            // returns the month-unit forward period for the time index
            Period forwardPeriod(
                TimeIndex timeIndex
            ) const {
                return Period(this->monthNumber(timeIndex), TimeUnit::Months);
            }
            operator TimeGridPtr() const { return pTimeGrid_; }
            const TimeGrid& timeGrid() const {
                QL_REQUIRE(pTimeGrid_ != nullptr, "grid is empty");
                return *pTimeGrid_;
            }
            std::vector<Time> times() const {
                if (pTimeGrid_ == nullptr) {
                    return {};
                }
                else {
                    return std::vector<Time>(pTimeGrid_->begin(), pTimeGrid_->end());
                }
            }
            Time meanDt() const {
                const TimeGrid& grid = this->timeGrid();
                GeneralStatistics stats;
                for (Size i = 0; i < grid.size() - 1; ++i) {
                    stats.add(grid.dt(i));
                }
                return stats.mean();
            }
            template<
                typename MONTH_FILTER
            >
            ImportantTenorFilter importTenorFilter(
                const MONTH_FILTER& monthFilter
            ) const {
                std::set<MonthNumber> importantMonths;
                for (MonthNumber month = this->minMonth(); month <= this->maxMonth(); ++month) {    // for each month in the grid
                    bool important = monthFilter(month);
                    if (important) {
                        importantMonths.insert(month);
                    }
                }
                auto filter = [importantMonths](
                    const TimeIndex& timeIndex
                ) -> std::pair<bool, Period> {
                    MonthNumber month = DatedMonthlyTimeGrid::monthNumber(timeIndex);
                    auto important = (importantMonths.find(month) != importantMonths.end());
                    if (important) {
                        Period tenor =
                        (
                            month == 0 ?
                            0 * Days :
                            (
                                month % 12 == 0 ?
                                month / 12 * Years :
                                month * Months
                            )
                        );
                        return { true, tenor };
                    }
                    else {
                        return { false, month * Months };
                    }
                };
                return filter;
            }
            // ZV functions
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // retuns short rate vector
            std::vector<Real> shortRates(
                const YieldTermStructureHandle& zvCurve,
                Real multiplier = 1.0
            ) const {
                auto n = schedule_.size();
                std::vector<Real> rates(n);
                for (TimeIndex timeIndex = 0; timeIndex < n; ++timeIndex) {
                    try {
                        auto date = schedule_[timeIndex];
                        Rate rate = zvCurve->forwardRate(
                            date,
                            0 * Days,
                            zvCurve->dayCounter(),
                            Compounding::Continuous,
                            Frequency::NoFrequency,
                            true
                        );
                        rates[timeIndex] = rate * multiplier;
                    }
                    catch (const std::exception& e) {
                        QL_FAIL("error occurred at time index " << timeIndex << ": " << e.what());
                    }
                }
                return rates;
            }
            // retuns discount factor vector
            DiscountVector discountFactors(
                const YieldTermStructureHandle& zvCurve
            ) const {
                auto n = schedule_.size();
                DiscountVector dfs(n);
                for (TimeIndex timeIndex = 0; timeIndex < n; ++timeIndex) {
                    try {
                        auto date = schedule_[timeIndex];
                        dfs[timeIndex] = zvCurve->discount(date, true);
                    }
                    catch (const std::exception& e) {
                        QL_FAIL("error occurred at time index " << timeIndex << ": " << e.what());
                    }
                }
                return dfs;
            }
            // returns forward ATM swap rates projection all the way to the grid boundary for a certain swap tenor
            std::vector<Real> forwardATMSwapRates(
                const YieldTermStructureHandle& zvCurve,
                const Period& swapTenor,
                const Period& couponTenor,
                const DayCounter& legsDayCounter,
                Real multiplier = 1.0
            ) const {
                std::vector<Real> rates;
                for (TimeIndex timeIndex = 0; timeIndex < schedule_.size(); ++timeIndex) {
                    try {
                        auto forward = this->forwardPeriod(timeIndex);
                        auto atmSwap = makeFwdATMVanillaSwap(
                            forward,
                            swapTenor,
                            couponTenor,
                            legsDayCounter,
                            Swap::Type::Payer,
                            zvCurve
                        );
                        if (atmSwap != nullptr) {
                            rates.push_back(atmSwap->fixedRate() * multiplier);
                        }
                        else {  // swap maturity is out of the grid boundary
                            break;
                        }
                    }
                    catch (const std::exception& e) {
                        QL_FAIL("error occurred at time index " << timeIndex << ": " << e.what());
                    }
                }
                return rates;
            }
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // IO functions
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            Time outputTime(
                TimeIndex timeIndex
            ) const {
                auto date = this->date(timeIndex);
                auto tenor = this->tenor(date);
                QL_ASSERT(tenor.units() == TimeUnit::Months, "tenor (" << tenor << ") must have unit in months");
                return Real(tenor.length()) / 12.0;
            }
            void writeVector(
                std::ostream& os,
                const std::vector<Real>& vec,
                Real multiplier = 1.0,
                std::streamsize precision = 6
            ) const {
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(precision);
                for (TimeIndex timeIndex = 0; timeIndex < vec.size(); ++timeIndex) {
                    auto t = outputTime(timeIndex);
                    std::vector<Real> rowVector{
                        t,
                        vec[timeIndex] * multiplier
                    };
                    for (Size j = 0; j < rowVector.size(); ++j) {
                        if (j > 0) {
                            oss << "\t";
                        }
                        oss << rowVector[j];
                    }
                    oss << std::endl;
                }
                os << oss.str();
            }
            void writeMatrix(
                std::ostream& os,
                const Matrix& matrix,
                Real multiplier = 1.0,
                std::streamsize precision = 6
            ) const {
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(precision);
                for (TimeIndex timeIndex = 0; timeIndex < matrix.rows(); ++timeIndex) {
                    auto t = outputTime(timeIndex);
                    Array row(matrix.row_begin(timeIndex), matrix.row_end(timeIndex));
                    row *= multiplier;
                    std::vector<Real> rowVector(matrix.columns() + 1);
                    rowVector[0] = t;
                    std::copy(row.begin(), row.end(), rowVector.begin() + 1);
                    for (Size j = 0; j < rowVector.size(); ++j) {
                        if (j > 0) {
                            oss << "\t";
                        }
                        oss << rowVector[j];
                    }
                    oss << std::endl;
                }
                os << oss.str();
            }
            /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        };
    }
}
