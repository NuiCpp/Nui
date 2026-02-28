#pragma once

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>

#include <string>

namespace Nui
{
    inline std::string base64Encode(const std::string& input)
    {
        using namespace boost::archive::iterators;
        typedef
            base64_from_binary<    // convert binary values to base64 characters
                transform_width<   // retrieve 6 bit integers from a sequence of 8 bit bytes
                    const char *,
                    6,
                    8
                >
            >
        base64_text;

        std::string result(base64_text(input.data()), base64_text(input.data() + input.size()));
        return result.append((3 - input.size() % 3) % 3, '=');
    }

    inline std::string base64Decode(const std::string& val)
    {
        using namespace boost::archive::iterators;
        typedef transform_width<binary_from_base64<std::string::const_iterator>, 8, 6> base64_text;

        std::string result(base64_text(val.begin()), base64_text(val.end()));
        return result.substr(0, result.find_last_not_of('=') + 1);
    }
}