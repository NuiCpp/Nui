#include <nlohmann/json.hpp>

#include <fstream>
#include <string>
#include <iostream>
#include <string_view>

using namespace std::string_literals;

std::string readFile(std::ifstream& ifs)
{
    ifs.seekg(0, std::ios_base::end);
    auto size = ifs.tellg();
    ifs.seekg(0);
    std::string buffer(size, '\0');
    ifs.read(buffer.data(), size);
    return buffer;
}

void disablePolyfillIfNotSet(nlohmann::json& alias, std::string_view aliasName)
{
    if (!alias.contains(aliasName))
        alias[aliasName] = false;
}

int main(int argc, char** argv)
{
    using nlohmann::json;

    if (argc != 4)
    {
        std::cout << "Expected 3 argument: <package_in.json> <package_out.json> <target-name>, but got " << argc - 1
                  << "\n";
        std::cout << "Usage: " << argv[0] << " <package_in.json> <package_out.json> <target-name>"
                  << "\n";
        return 1;
    }

    std::ifstream ifs(argv[1], std::ios_base::binary);
    if (!ifs.is_open())
    {
        std::cout << "Could not open file: '" << argv[1] << "'\n";
        return 1;
    }
    auto package = json::parse(readFile(ifs));
    if (!package.contains("source"))
        package["source"] = "static/"s + std::string{argv[3]} + ".html";

    if (!package.contains("targets"))
    {
        package["targets"] = json::object();
        package["targets"]["main"] = false;
    }

    if (!package.contains("alias"))
        package["alias"] = json::object();

    disablePolyfillIfNotSet(package["alias"], "assert");
    disablePolyfillIfNotSet(package["alias"], "fs");
    disablePolyfillIfNotSet(package["alias"], "buffer");
    disablePolyfillIfNotSet(package["alias"], "console");
    disablePolyfillIfNotSet(package["alias"], "constants");
    disablePolyfillIfNotSet(package["alias"], "crypto");
    disablePolyfillIfNotSet(package["alias"], "domain");
    disablePolyfillIfNotSet(package["alias"], "events");
    disablePolyfillIfNotSet(package["alias"], "http");
    disablePolyfillIfNotSet(package["alias"], "https");
    disablePolyfillIfNotSet(package["alias"], "os");
    disablePolyfillIfNotSet(package["alias"], "path");
    disablePolyfillIfNotSet(package["alias"], "process");
    disablePolyfillIfNotSet(package["alias"], "punycode");
    disablePolyfillIfNotSet(package["alias"], "querystring");
    disablePolyfillIfNotSet(package["alias"], "stream");
    disablePolyfillIfNotSet(package["alias"], "string_decoder");
    disablePolyfillIfNotSet(package["alias"], "sys");
    disablePolyfillIfNotSet(package["alias"], "timers");
    disablePolyfillIfNotSet(package["alias"], "tty");
    disablePolyfillIfNotSet(package["alias"], "url");
    disablePolyfillIfNotSet(package["alias"], "util");
    disablePolyfillIfNotSet(package["alias"], "vm");
    disablePolyfillIfNotSet(package["alias"], "zlib");

    std::ofstream ofs(argv[2], std::ios_base::binary);
    ofs << package.dump(4);
}