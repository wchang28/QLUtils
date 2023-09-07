#pragma once

#include <ql/quantlib.hpp>

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
        typedef typename BASE_SWAP_INDEX BaseSwapIndex;
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
        auto createOvernightIndex(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& indexEstimatingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            return QuantLib::ext::shared_ptr<OvernightIndex>(new OvernightIndex(indexEstimatingTermStructure));
        }
    };

    // traits for the vanilla swap
    // examples:
    // VanillaSwapTraits<QuantLib::UsdLiborSwapIsdaFixAm>
    // VanillaSwapTraits<QuantLib::ChfLiborSwapIsdaFix>
    // VanillaSwapTraits<QuantLib::EuriborSwapIfrFix>
    // VanillaSwapTraits<QuantLib::EurLiborSwapIfrFix>
    // VanillaSwapTraits<QuantLib::GbpLiborSwapIsdaFix>
    // VanillaSwapTraits<QuantLib::JpyLiborSwapIsdaFixPm>
    template<typename BaseSwapIndex>
    struct VanillaSwapTraits {
        QuantLib::Natural settlementDays(const QuantLib::Period& tenor) const {
            BaseSwapIndex swapIndex(tenor);
            return swapIndex.fixingDays();
        }
        // !!! calendar for both legs !!!
        QuantLib::Calendar fixingCalendar(const QuantLib::Period& tenor) const {
            BaseSwapIndex swapIndex(tenor);
            return swapIndex.fixingCalendar();
        }
        // !!! end of month for both legs !!!
        bool endOfMonth(const QuantLib::Period& tenor) const {
            return false;
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