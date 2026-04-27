#pragma once

#include <ql/quantlib.hpp>
#include <string>
#include <sstream>

namespace QuantLib {
    namespace Utils {
        struct ISODateConv {
            static std::string to_str(const Date& dt) {
                return (std::ostringstream() << io::iso_date(dt)).str();
            }
            static QuantLib::Date from_str(const std::string& str) {
                return DateParser::parseISO(str);
            }
        };
    }
}
