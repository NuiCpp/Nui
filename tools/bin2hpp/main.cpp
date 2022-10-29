#include <cctype>
#include <fstream>
#include <ios>
#include <iostream>
#include <iomanip>
#include <string>
#include <filesystem>

constexpr std::size_t lineWidth = 120;

int main(int argc, char** argv)
{
    if (argc != 5)
    {
        std::cout << "Expected 4 arguments: <yes/no> <input file> <output file> <name>, but got " << argc - 1 << "\n";
        std::cout << "Usage: " << argv[0] << " <yes/no> <input file> <output file> <name>"
                  << "\n";
        return 1;
    }

    std::string enable = argv[1];
    std::string inputFile = argv[2];
    std::string outputFile = argv[3];
    std::string name = argv[4];

    if (enable == "no")
    {
        return 0;
    }

    auto outputPath = std::filesystem::path(outputFile).parent_path();
    if (!std::filesystem::exists(outputPath))
        std::filesystem::create_directories(outputPath);

    std::ifstream input(inputFile, std::ios_base::binary);
    std::ofstream output(outputFile, std::ios_base::binary);
    if (!input.is_open())
    {
        std::cout << "Could not open file \"" << inputFile << "\"\n";
        return 1;
    }
    if (!output.is_open())
    {
        std::cout << "Could not open file \"" << outputFile << "\"\n";
        return 1;
    }

    output << "#pragma once\n";
    output << "\n";
    output << "#include <string_view>\n";
    output << "#include <string>\n";
    output << "\n";
    output << "static const std::string_view " << name << "_data[] = {\n";

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
    output << "static std::string " << name << "()\n";
    output << "{\n";
    output << "\tstatic std::string memo;\n";
    output << "\tif(!memo.empty())\n";
    output << "\t\treturn memo;\n";
    output << "\tfor (std::size_t i = 0; i != sizeof(" << name << "_data) / sizeof(std::string_view)"
           << "; ++i) {\n";
    output << "\t\tmemo += " << name << "_data[i];\n";
    output << "\t}\n";
    output << "\treturn memo;\n";
    output << "}\n";
}