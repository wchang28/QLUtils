#pragma once

#include <string>
#include <ql/quantlib.hpp>

namespace QLUtils {
    class BootstrapInstrument {
    public:
        enum ValueType {
            Rate = 0,
            Price = 1,
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
            const ValueType valueType,
            const QuantLib::Period& tenor = QuantLib::Period(),
            const QuantLib::Date& datedDate = QuantLib::Date()
        ) :
            valueType_(valueType),
            tenor_(tenor),
            datedDate_(datedDate),
            value_(QuantLib::Null < QuantLib::Real >()),
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
    };

    class OISSwapIndex : public BootstrapInstrument {
    public:
        OISSwapIndex(const QuantLib::Period& tenor) : BootstrapInstrument(Rate, tenor) {}
    };

    class SwapCurveInstrument : public BootstrapInstrument {
    public:
        enum InstType {
            Deposit = 0,
            Future = 1,
            FRA = 2,
            Swap = 3,
        };
    protected:
        InstType instType_;
    public:
        SwapCurveInstrument(
        const ValueType valueType,
        InstType instType,
        const QuantLib::Period& tenor = QuantLib::Period(),
        const QuantLib::Date& datedDate = QuantLib::Date()
        ) : BootstrapInstrument(valueType, tenor, datedDate),
            instType_(instType) {}
        const InstType& instType() const {
            return instType_;
        }
        InstType& instType() {
            return instType_;
        }
    };

    class IMMFuture : public SwapCurveInstrument {
    protected:
        QuantLib::Natural immOrdinal_;
        std::string immTicker_;
    public:
        static void ensureIMMDate(const QuantLib::Date& immDate) {
            QL_REQUIRE(QuantLib::IMM::isIMMdate(immDate, true), "specified date " << immDate << " is not a main cycle IMM date");
        }
        static void ensureIMMDateNotExpired(const QuantLib::Date& immDate,
            const QuantLib::Date& refDate) {
            QL_REQUIRE(immDate >= refDate, "IMM date " << immDate << " already expired");
        }
        static QuantLib::Date IMMMainCycleStartDateForOrdinal(QuantLib::Natural iMMOrdinal,
            const QuantLib::Date& today = QuantLib::Date()) {
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
        static QuantLib::Natural IMMMainCycleOrdinalForStartDate(const QuantLib::Date& immDate,
            const QuantLib::Date& today = QuantLib::Date()) {
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
        static QuantLib::Period calculateTenor(const QuantLib::Date& immDate,
            const QuantLib::Date& today = QuantLib::Date()) {
            ensureIMMDate(immDate);
            QuantLib::Date d = (today == QuantLib::Date() ? QuantLib::Settings::instance().evaluationDate() : today);
            d += 2;
            ensureIMMDateNotExpired(immDate, d);
            auto days = immDate - d;
            return days * QuantLib::Days;
        }
        IMMFuture(QuantLib::Natural immOrdinal) : SwapCurveInstrument(Price, Future, QuantLib::Period(), QuantLib::Null < QuantLib::Date >()),
            immOrdinal_(immOrdinal) {
            datedDate_ = IMMMainCycleStartDateForOrdinal(immOrdinal);
            tenor_ = calculateTenor(datedDate_);
            immTicker_ = QuantLib::IMM::code(datedDate_);
        }
        IMMFuture(const QuantLib::Date& immDate) : SwapCurveInstrument(Price, Future, calculateTenor(immDate), immDate),
            immOrdinal_(IMMMainCycleOrdinalForStartDate(immDate)),
            immTicker_(QuantLib::IMM::code(immDate)) {}
        const QuantLib::Date& immDate() const {
            return datedDate_;
        }
        const QuantLib::Natural& immOrdinal() const {
            return immOrdinal_;
        }
        const std::string& immTicker() const {
            return immTicker_;
        }
    };

    class CashDepositIndex : public SwapCurveInstrument {
    public:
        CashDepositIndex(const QuantLib::Period& tenor) : SwapCurveInstrument(Rate, Deposit, tenor) {}
    };

    class ForwardRateAgreement : public SwapCurveInstrument {
    public:
        ForwardRateAgreement(const QuantLib::Period& tenor) : SwapCurveInstrument(Rate, FRA, tenor) {}
    };

    class SwapIndex : public SwapCurveInstrument {
    public:
        SwapIndex(const QuantLib::Period& tenor) : SwapCurveInstrument(Rate, Swap, tenor) {}
    };
}
