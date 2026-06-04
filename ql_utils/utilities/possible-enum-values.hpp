#pragma once

#include <set>

namespace QuantLib {
    namespace Utils {
        template <
            typename ENUM
        >
        struct possible_enum_values {
            // get() is to be specialized by the enum type
            static const std::set<ENUM>& get() {
                static std::set<ENUM> s;
                return s;
            }
        };
    }
}
