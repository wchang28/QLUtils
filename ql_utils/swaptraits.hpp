#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/indexes/ois-swap-index.hpp>

namespace QLUtils {
    // traits for the overnight indexes swap
    // example:
    // OvernightIndexedSwapTraits<UsdOvernightIndexedSwapIsdaFix<FedFunds>>
    // OvernightIndexedSwapTraits<UsdOvernightIndexedSwapIsdaFix<Sofr>>
    // OvernightIndexedSwapTraits<GbpOvernightIndexedSwapIsdaFix<Sonia>>
    // OvernightIndexedSwapTraits<EurOvernightIndexedSwapIsdaFix<Estr>>
    // OvernightIndexedSwapTraits<EurOvernightIndexedSwapIsdaFix<Eonia>>
    template<
        typename BASE_SWAP_INDEX
    >
    struct OvernightIndexedSwapTraits {
        typedef BASE_SWAP_INDEX BaseSwapIndex;
        typedef typename BaseSwapIndex::OvernightIndex OvernightIndex;
        QuantLib::Natural settlementDays(const QuantLib::Period& tenor) const {
            BaseSwapIndex swapIndex(tenor);
            return swapIndex.fixingDays();
        }
        bool telescopicValueDates(const QuantLib::Period& tenor) const {
            BaseSwapIndex swapIndex(tenor);
            return swapIndex.telescopicValueDates();
        }
        QuantLib::RateAveraging::Type averagingMethod(const QuantLib::Period& tenor) const {
            BaseSwapIndex swapIndex(tenor);
            return swapIndex.averagingMethod();
        }
        QuantLib::Frequency paymentFrequency(const QuantLib::Period& tenor) const {
            BaseSwapIndex swapIndex(tenor);
            return swapIndex.paymentFrequency();
        }
        QuantLib::BusinessDayConvention paymentConvention(const QuantLib::Period& tenor) const {
            BaseSwapIndex swapIndex(tenor);
            return swapIndex.paymentConvention();
        }
        QuantLib::Natural paymentLag(const QuantLib::Period& tenor) const {
            BaseSwapIndex swapIndex(tenor);
            return swapIndex.paymentLag();
        }
        QuantLib::Calendar paymentCalendar(const QuantLib::Period& tenor) const {
            BaseSwapIndex swapIndex(tenor);
            return swapIndex.paymentCalendar();
        }
        // !!! fixing calendar for both legs !!!
        QuantLib::Calendar fixingCalendar(const QuantLib::Period& tenor) const {
            BaseSwapIndex swapIndex(tenor);
            return swapIndex.fixingCalendar();
        }
        // !!! end of month flag for both legs !!!
        bool endOfMonth(const QuantLib::Period& tenor) const {
            BaseSwapIndex swapIndex(tenor);
            return swapIndex.endOfMonth();
        }
        // bisiness day adj convention for both legs
        QuantLib::BusinessDayConvention convention(const QuantLib::Period& tenor) const {
            BaseSwapIndex swapIndex(tenor);
            return swapIndex.convention();
        }
    };

    // traits for the vanilla swap
    // examples:
    // VanillaSwapTraits<QuantLib::EuriborSwapIsdaFixA>
    // VanillaSwapTraits<QuantLib::UsdTermSofrSwapIsdaFix<1>>
    // VanillaSwapTraits<QuantLib::UsdTermSofrSwapIsdaFix<3>>
    // VanillaSwapTraits<QuantLib::UsdTermSofrSwapIsdaFix<6>>
    // VanillaSwapTraits<QuantLib::UsdTermSofrSwapIsdaFix<12>>
    // VanillaSwapTraits<QuantLib::UsdFwdOISVanillaSwapIndex<Sofr>>
    template<
        typename BASE_SWAP_INDEX
    >
    struct VanillaSwapTraits {
        typedef BASE_SWAP_INDEX BaseSwapIndex;
        QuantLib::Natural settlementDays(const QuantLib::Period& tenor) const {
            BaseSwapIndex swapIndex(tenor);
            return swapIndex.fixingDays();
        }
        // !!! fixing calendar for both legs !!!
        QuantLib::Calendar fixingCalendar(const QuantLib::Period& tenor) const {
            BaseSwapIndex swapIndex(tenor);
            return swapIndex.fixingCalendar();
        }
        // !!! end of month flag for both legs !!!
        bool endOfMonth(const QuantLib::Period& tenor) const {
            std::shared_ptr<BaseSwapIndex> pSwapIndex(new BaseSwapIndex(tenor));
            auto pIndexEx = std::dynamic_pointer_cast<QuantLib::SwapIndexEx>(pSwapIndex);
            return (pIndexEx != nullptr ? pIndexEx->endOfMonth() : false);
        }
        QuantLib::Period fixedLegTenor(const QuantLib::Period& tenor) const {
            BaseSwapIndex swapIndex(tenor);
            return swapIndex.fixedLegTenor();
        }
        QuantLib::Frequency fixedLegFrequency(const QuantLib::Period& tenor) const {
            auto freq = fixedLegTenor(tenor).frequency();
            QL_ASSERT(freq != QuantLib::OtherFrequency, "bad tenor for the fixed leg of the swap");
            return freq;
        }
        QuantLib::BusinessDayConvention fixedLegConvention(const QuantLib::Period& tenor) const {
            BaseSwapIndex swapIndex(tenor);
            return swapIndex.fixedLegConvention();
        }
        QuantLib::DayCounter fixedLegDayCount(const QuantLib::Period& tenor) const {
            BaseSwapIndex swapIndex(tenor);
            return swapIndex.dayCounter();
        }
    };
}
