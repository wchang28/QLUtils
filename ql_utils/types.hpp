#pragma once

#include <string>
#include <sstream>
#include <ql/quantlib.hpp>
#include <functional>
#include <ostream>

namespace QLUtils {
    template<typename _Elem> using string_t = std::basic_string<_Elem>;
    template<typename _Elem> using ostringstream_t = std::basic_ostringstream<_Elem>;

    struct Verifiable {
        virtual void verify(std::ostream& os) const = 0;
    };
    using IborIndexFactory = std::function<QuantLib::ext::shared_ptr<QuantLib::IborIndex>(const QuantLib::Handle<QuantLib::YieldTermStructure>&)>;
}