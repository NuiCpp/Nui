#include <nlohmann/json.hpp>

#include <fstream>
#include <string>
#include <iostream>

using namespace std::string_literals;

std::string readFile(std::string const& filename)
{
    std::ifstream ifs(filename, std::ios_base::binary);
    ifs.seekg(0, std::ios_base::end);
    auto size = ifs.tellg();
    ifs.seekg(0);
    std::string buffer(size, '\0');
    ifs.read(buffer.data(), size);
    return buffer;
}

int main(int argc, char** argv)
{
    using nlohmann::json;

    if (argc != 3)
    {
        std::cout << "Expected 2 argument: <package.json> <target-name>, but got " << argc - 1 << "\n";
        std::cout << "Usage: " << argv[0] << " <package.json> <target-name>"
                  << "\n";
        return 1;
    }

    auto package = json::parse(readFile(argv[1]));
    package["source"] = "static/"s + std::string{argv[2]} + ".html";

    std::ofstream ofs(argv[1], std::ios_base::binary);
    ofs << package.dump(4);
}