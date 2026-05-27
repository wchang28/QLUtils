#pragma once

#include <ql_utils/types.hpp>

namespace QuantLib {
    namespace Utils {
        template<
            typename MULTIPLIER_TYPE = double
        >
        struct RateUnitMultipliers {
            static MULTIPLIER_TYPE output_multiplier(
                QLUtils::RateUnit unit
            ) {
                if (unit == QLUtils::RateUnit::Percent) {
                    return 100.0;
                }
                else if (unit == QLUtils::RateUnit::BasisPoint) {
                    return 10000.0;
                }
                else if (unit == QLUtils::RateUnit::Decimal) {
                    return 1.0;
                }
                else {
                    return 1.0;
                }            
            }
            static MULTIPLIER_TYPE native_multiplier(
                QLUtils::RateUnit unit
            ) {
                return 1.0 / output_multiplier(unit);
            }
        };
    }
}
