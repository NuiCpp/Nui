#include "raw_encoder.hpp"
#include "constants.hpp"

#include <iostream>

std::ostream& RawEncoder::header(std::ostream& output) const
{
    output << "#pragma once\n";
    output << "\n";
    output << "#include <string_view>\n";
    output << "#include <string>\n";
    output << "\n";
    output << "static const std::string_view " << name_ << "_data[] = {\n";

    return output;
}

std::ostream& RawEncoder::content(std::ostream& output, std::istream& input) const
{
    do
    {
        char buffer[1024];
        input.read(buffer, sizeof(buffer));
        if (input.gcount() == 0)
            break;
        for (std::streamsize i = 0; i != input.gcount();)
        {
            std::streamsize j = 0;
            output << "\tR\"NUI(";
            std::size_t widthUsed = 0;
            for (; (widthUsed < (lineWidth - 14)) && (j + i != input.gcount()); ++j)
            {
                char c = buffer[i + j];
                output.put(c);
                ++widthUsed;
            }
            output << ")NUI\"\n";
            i += j;
        }
        output << "\t,\n";
    } while (input.gcount() > 0);
    return output;
}

std::ostream& RawEncoder::index(std::ostream& output) const
{
    output << "};\n\n";
    output << "static std::string " << name_ << "()\n";
    output << "{\n";
    output << "\tstatic std::string memo;\n";
    output << "\tif(!memo.empty())\n";
    output << "\t\treturn memo;\n";
    output << "\tfor (std::size_t i = 0; i != sizeof(" << name_ << "_data) / sizeof(std::string_view)"
           << "; ++i) {\n";
    output << "\t\tmemo += " << name_ << "_data[i];\n";
    output << "\t}\n";
    output << "\treturn memo;\n";
    output << "}\n";

    return output;
}