#pragma once
#include <ql/quantlib.hpp>

namespace QuantLib {
    class USDLibor3M : public USDLibor {
        USDLibor3M(
            const Handle<YieldTermStructure>& h = {}
        ) : USDLibor(3 * Months, h)
        {}
    };
}