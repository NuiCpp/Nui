#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

std::string readFile(const std::filesystem::path& path)
{
    std::ifstream file{path, std::ios_base::binary};
    if (!file)
    {
        throw std::runtime_error{"Could not open " + path.string()};
    }

    file.seekg(0, std::ios::end);
    std::string content;
    content.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(&content[0], content.size());
    return content;
}

int main(int argc, char** argv)
{
    if (argc != 6)
    {
        std::cout << "Expected 4 argument: <index.html> <import_scripts> <import_styles> <import_scripts_defer> "
                     "<lean_html>, but got "
                  << argc - 1 << "\n";
        return 1;
    }

    const auto index = std::filesystem::path{argv[1]};
    const auto importScripts = std::filesystem::path{argv[2]};
    const auto importStyles = std::filesystem::path{argv[3]};
    const auto importScriptsDefer = std::string{argv[4]} == "defer" ? true : false;
    const auto leanHtml = std::string{argv[5]} == "lean" ? true : false;

    std::string indexHtml;
    try
    {
        indexHtml = readFile(index);
    }
    catch (const std::exception& e)
    {
        std::cout << "Error reading file: " << e.what() << "\n";
        return 1;
    }

    // make relative path of import scripts to index file:
    const auto relativeImportScriptsFile = std::filesystem::relative(importScripts, index.parent_path());
    const auto relativeImportStylesFile = std::filesystem::relative(importStyles, index.parent_path());
    const auto binIndex =
        std::filesystem::relative(index.parent_path() / ".." / "bin" / "index.js", index.parent_path());

    const std::string deferTag = importScriptsDefer ? " defer" : "";

    const std::string importScriptsHtml = "\t<script type=\"module\" " + deferTag + ">\n\t\timport \"" +
        relativeImportScriptsFile.generic_string() + "\";\n\t</script>\n";
    const std::string importStylesHtml =
        "\t<style>\n\t\t@import \"" + relativeImportStylesFile.generic_string() + "\";\n\t</style>\n";

    std::string importBinIndexHtml;
    if (!leanHtml)
    {
        importBinIndexHtml =
            "\t<script type=\"module\" defer>\n\t\timport \"" + binIndex.generic_string() + "\";\n\t</script>\n";
    }
    else
    {
        importBinIndexHtml = "\t<script type=\"module\" defer src=\"" + binIndex.generic_string() + "\"></script>\n";
    }

    // find end of header </head> from behind in indexHtml:
    auto headEnd = indexHtml.rfind("</head>");
    if (headEnd == std::string::npos)
    {
        std::cout << "Could not find </head> in " << index << "\n";
        return 1;
    }

    const auto headBegin = indexHtml.find("<head>");
    if (headBegin == std::string::npos)
    {
        std::cout << "Could not find <head> in " << index << "\n";
        return 1;
    }

    auto insertPoint = headEnd;

    const std::string insertionHint = "<!-- Nui Inline Insertion Slot -->";
    const auto insertHintPos = indexHtml.find(insertionHint);
    if (insertHintPos != std::string::npos)
    {
        // insert after the hint:
        insertPoint = insertHintPos + insertionHint.size();
    }

    // insert importScriptsHtml before headEnd:
    indexHtml.insert(insertPoint, importScriptsHtml);

    // insert importStylesHtml before headEnd:
    indexHtml.insert(insertPoint, importStylesHtml);

    headEnd = indexHtml.rfind("</head>");
    if (headEnd == std::string::npos)
    {
        std::cout << "Could not find </head> in " << index << "\n";
        return 1;
    }

    // insert importBinIndexHtml before headEnd (always):
    if (indexHtml.find(binIndex.generic_string()) == std::string::npos)
        indexHtml.insert(headEnd, importBinIndexHtml);

    // write indexHtml back to index file:
    std::ofstream file{index, std::ios_base::binary};
    if (!file)
    {
        std::cout << "Could not open " << index << " for writing\n";
        return 1;
    }

    file.write(indexHtml.data(), indexHtml.size());
    return 0;
}