#pragma once

#include <string>
#include <ql/quantlib.hpp>

namespace QuantLib {
    namespace Utils {
        struct BootstrapQuote {
            std::string ticker;
            bool use;   // include for bootstrapping flag
            Real value;   // quoted value
            BootstrapQuote() :
                use(true),
                value(0.0)
            {}
        };
        
        enum GovernmentSecurityType {
            gstBill = 0,	// zero-coupon government security
            gstBond = 1,	// coupon-bearing government security
        };
        
        struct GovernmentSecurityQuote: public BootstrapQuote {
            GovernmentSecurityType type;
            Period tenor;
            Rate coupon;
            Date settleDate;
            Date maturityDate;
            std::string valueType;
            GovernmentSecurityQuote() :
                type(GovernmentSecurityType::gstBill),
                coupon(0.0)
            {}   
        };
        
        enum BondQuoteValueType {
            bqvtCleanPrice = 0,
            bqvtDirtyPrice = 1,
            bqvtYTM = 2,
        };
        enum BillQuoteValueType {
            bqvtDiscountRate = 0,
            bqvtPrice = 1,	// dirty/clean price, since clean price=dirty price for bill
            bqvtYield = 2,
            bqvtDiscountFactor = 3
        };
    }
}