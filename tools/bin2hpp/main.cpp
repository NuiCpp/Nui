#include <cctype>
#include <fstream>
#include <ios>
#include <iostream>
#include <iomanip>
#include <string>

constexpr std::size_t lineWidth = 120;

int main(int argc, char** argv)
{
    if (argc != 4)
    {
        std::cout << "Expected 3 arguments: <input file> <output file> <name>, but got " << argc - 1 << "\n";
        std::cout << "Usage: " << argv[0] << " <input file> <output file> <name>"
                  << "\n";
        return 1;
    }

    std::ifstream input(argv[1], std::ios_base::binary);
    std::ofstream output(argv[2], std::ios_base::binary);
    if (!input.is_open())
    {
        std::cout << "Could not open file \"" << argv[1] << "\"\n";
        return 1;
    }
    if (!output.is_open())
    {
        std::cout << "Could not open file \"" << argv[2] << "\"\n";
        return 1;
    }

    output << "#pragma once\n";
    output << "\n";
    output << "#include <string_view>\n";
    output << "#include <string>\n";
    output << "\n";
    output << "static const std::string_view " << argv[3] << "_data[] = {\n";

    do
    {
        char buffer[1024];
        input.read(buffer, sizeof(buffer));
        if (input.gcount() == 0)
            break;
        for (std::streamsize i = 0; i != input.gcount();)
        {
            std::streamsize j = 0;
            output << "\t\"";
            std::size_t widthUsed = 0;
            for (; (widthUsed < (lineWidth - 9)) && (j + i != input.gcount()); ++j)
            {
                char c = buffer[i + j];
                if (c < 32 || c == '"' || c == '\\')
                {
                    output << '\\' << std::oct << std::setw(3) << std::setfill('0') << static_cast<int>(c);
                    widthUsed += 4;
                }
                else
                {
                    output << c;
                    ++widthUsed;
                }
            }
            output << "\"\n";
            i += j;
        }
        output << "\t,\n";
    } while (input.gcount() > 0);

    output << "};\n\n";
    output << "static std::string " << argv[3] << "()\n";
    output << "{\n";
    output << "\tstatic std::string memo;\n";
    output << "\tif(!memo.empty())\n";
    output << "\t\treturn memo;\n";
    output << "\tfor (std::size_t i = 0; i != sizeof(" << argv[3] << "_data) / sizeof(std::string_view)"
           << "; ++i) {\n";
    output << "\t\tmemo += " << argv[3] << "_data[i];\n";
    output << "\t}\n";
    output << "\treturn memo;\n";
    output << "}\n";
}