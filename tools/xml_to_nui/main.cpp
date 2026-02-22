#include "pugixml.hpp"

#include <boost/program_options.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_set>
#include <cctype>
#include <algorithm>

#define STRINGIZE(x) #x
#define STRINGIZE_EXPANDED(x) STRINGIZE(x)

namespace
{
    int indentWidth = 4;
    int lineWidth = 120;

    std::vector<std::string> cppKeywords()
    {
        static std::vector<std::string> keywords{"alignas",   "alignof",      "and",           "and_eq",
                                                 "asm",       "auto",         "bitand",        "bitor",
                                                 "bool",      "break",        "case",          "catch",
                                                 "char",      "char8_t",      "char16_t",      "char32_t",
                                                 "class",     "compl",        "concept",       "const",
                                                 "consteval", "constexpr",    "constinit",     "const_cast",
                                                 "continue",  "co_await",     "co_return",     "co_yield",
                                                 "decltype",  "default",      "delete",        "do",
                                                 "double",    "dynamic_cast", "else",          "enum",
                                                 "explicit",  "export",       "extern",        "false",
                                                 "float",     "for",          "friend",        "goto",
                                                 "if",        "inline",       "int",           "long",
                                                 "mutable",   "namespace",    "new",           "noexcept",
                                                 "not",       "not_eq",       "nullptr",       "operator",
                                                 "or",        "or_eq",        "private",       "protected",
                                                 "public",    "reflexpr",     "register",      "reinterpret_cast",
                                                 "requires",  "return",       "short",         "signed",
                                                 "sizeof",    "static",       "static_assert", "static_cast",
                                                 "struct",    "switch",       "synchronized",  "template",
                                                 "this",      "thread_local", "throw",         "true",
                                                 "try",       "typedef",      "typeid",        "typename",
                                                 "union",     "unsigned",     "using",         "virtual",
                                                 "void",      "volatile",     "wchar_t",       "while",
                                                 "xor",       "xor_eq"};
        return keywords;
    }

    std::vector<std::string> splitList(std::string list)
    {
        size_t pos = 0;
        std::vector<std::string> result;
        while ((pos = list.find(',')) != std::string::npos)
        {
            result.push_back(list.substr(0, pos));
            list.erase(0, pos + 1);
        }
        if (!list.empty())
            result.push_back(list);
        return result;
    }
    std::string toLower(std::string_view str)
    {
        std::string result;
        result.reserve(str.size());
        for (char c : str)
            result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return result;
    }
    std::string trimWhitespace(std::string_view str)
    {
        size_t start = 0;
        while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start])))
            ++start;

        size_t end = str.size();
        while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1])))
            --end;

        return std::string(str.substr(start, end - start));
    }
    bool isStringNumeric(std::string_view str)
    {
        return std::all_of(str.begin(), str.end(), [](char c) {
            return std::isdigit(static_cast<unsigned char>(c));
        });
    }
    // Remove everything that does not constitute a valid C++ identifier:
    std::string sanitizeName(std::string const& name)
    {
        std::string result;
        result.reserve(name.size());
        for (char c : name)
        {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_')
                result += c;
        }
        const auto keywords = cppKeywords();
        if (std::find(keywords.begin(), keywords.end(), result) != keywords.end())
            result += "_";
        return result;
    }

    std::vector<std::string> knownElements()
    {
        static std::vector<std::string> elementsMemo;
        if (elementsMemo.empty())
        {
            // NUI_ELEMENTS is set by CMake and a ; separated list:
            elementsMemo = splitList(STRINGIZE_EXPANDED(NUI_ELEMENTS));
        }
        return elementsMemo;
    }
    bool isKnownElement(const std::string& name)
    {
        const auto knownElems = knownElements();
        static std::unordered_set<std::string> knownElementsSet{knownElems.begin(), knownElems.end()};
        return knownElementsSet.contains(toLower(name));
    }

    std::vector<std::string> knownAttributes()
    {
        static std::vector<std::string> attributesMemo;
        if (attributesMemo.empty())
        {
            // NUI_ATTRIBUTES is set by CMake and a ; separated list:
            attributesMemo = splitList(STRINGIZE_EXPANDED(NUI_ATTRIBUTES));
        }
        return attributesMemo;
    }
    bool isKnownAttribute(const std::string& name)
    {
        const auto knownAttrs = knownAttributes();
        static std::unordered_set<std::string> knownAttributesSet{knownAttrs.begin(), knownAttrs.end()};
        return knownAttributesSet.contains(toLower(name));
    }

    std::vector<std::string> knownSvgElements()
    {
        static std::vector<std::string> svgElementsMemo;
        if (svgElementsMemo.empty())
        {
            // NUI_SVG_ELEMENTS is set by CMake and a ; separated list:
            svgElementsMemo = splitList(STRINGIZE_EXPANDED(NUI_SVG_ELEMENTS));
        }
        return svgElementsMemo;
    }
    bool isKnownSvgElement(const std::string& name)
    {
        const auto knownSvgElems = knownSvgElements();
        static std::unordered_set<std::string> knownSvgElementsSet{knownSvgElems.begin(), knownSvgElems.end()};
        return knownSvgElementsSet.contains(toLower(name));
    }

    std::vector<std::string> knownSvgAttributes()
    {
        static std::vector<std::string> svgAttributesMemo;
        if (svgAttributesMemo.empty())
        {
            // NUI_SVG_ATTRIBUTES is set by CMake and a ; separated list:
            svgAttributesMemo = splitList(STRINGIZE_EXPANDED(NUI_SVG_ATTRIBUTES));
        }
        return svgAttributesMemo;
    }
    bool isKnownSvgAttribute(const std::string& name)
    {
        const auto knownSvgAttrs = knownSvgAttributes();
        static std::unordered_set<std::string> knownSvgAttributesSet{knownSvgAttrs.begin(), knownSvgAttrs.end()};
        return knownSvgAttributesSet.contains(toLower(name));
    }

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

    std::string formatStringAsLiteralBreakingLine(std::string_view str, int indent)
    {
        std::string result = "\"";
        int currentLineLength = 1; // account for opening quote
        for (size_t i = 0; i < str.size(); ++i)
        {
            char c = str[i];
            if (c == '\\' || c == '"')
                result += '\\';
            result += c;
            ++currentLineLength;

            if (currentLineLength >= lineWidth && i + 1 < str.size())
            {
                result += "\"\n" + std::string(indent + indentWidth, ' ') + "\"";
                currentLineLength = indent + 3; // account for closing quote, newline, indentation, and opening quote
            }
        }
        result += "\"";
        return result;
    }

    struct PrintContext
    {
        bool withinSvg = false;
        int indent = 0;
        bool inFunction = false;
        bool isTopLevel = true;
        bool useNamespaceAbbreviations = false;
    };

    auto makeIndentString(int indent)
    {
        return std::string(static_cast<std::size_t>(indent * indentWidth), ' ');
    }

    struct ScopedIndent
    {
        PrintContext& context;
        explicit ScopedIndent(PrintContext& context)
            : context{context}
        {
            ++context.indent;
        }
        ~ScopedIndent()
        {
            --context.indent;
        }
        ScopedIndent(const ScopedIndent&) = delete;
        ScopedIndent& operator=(const ScopedIndent&) = delete;
        ScopedIndent(ScopedIndent&&) = delete;
        ScopedIndent& operator=(ScopedIndent&&) = delete;
    };
// create a unique name via preprocessor
#define INDENT_VAR_NAME(line) indent##line
#define INDENT() ScopedIndent INDENT_VAR_NAME(__LINE__)(context)

    // NOLINTNEXTLINE(misc-no-recursion)
    std::ostream& convertNode(pugi::xml_node const& node, std::ostream& stream, PrintContext context = {})
    {
        if (context.inFunction)
        {
            stream << makeIndentString(context.indent) << "return ";
        }

        auto ns = [&context](bool isSvg, bool isElement) {
            if (!context.useNamespaceAbbreviations)
            {
                if (isSvg && isElement)
                    return "Nui::Elements::Svg::";
                if (isSvg)
                    return "Nui::Attributes::Svg::";
                if (isElement)
                    return "Nui::Elements::";
                return "Nui::Attributes::";
            }

            if (isSvg && isElement)
                return "se::";
            if (isSvg)
                return "sa::";
            if (isElement)
                return "nuie::";
            return "nuia::";
        };

        if (node.type() == pugi::xml_node_type::node_element)
        {
            const auto& name = node.name();
            const bool isSvg = name == std::string_view{"svg"};
            const bool knownElement = isKnownElement(name);
            const bool knownSvgElement = isKnownSvgElement(name);

            if (context.withinSvg || isSvg)
                context.withinSvg = true;

            if (!knownElement && !(context.withinSvg && knownSvgElement))
            {
                std::cerr << "Unknown element: " << name << "\n";
                return stream;
            }

            if (context.inFunction)
            {
                stream << ns(context.withinSvg, true) << name << "{\n";
                context.inFunction = false;
            }
            else
                stream << makeIndentString(context.indent) << ns(context.withinSvg, true) << name << "{\n";

            {
                INDENT();

                for (const auto& attr : node.attributes())
                {
                    const auto& attrName = attr.name();
                    const auto attrValue = [&]() {
                        const auto* const value = attr.value();
                        if (isStringNumeric(value))
                            return std::string{value};
                        return formatStringAsLiteralBreakingLine(value, context.indent * indentWidth);
                    }();
                    if (context.withinSvg)
                    {
                        if (!isKnownSvgAttribute(attrName))
                        {
                            stream << makeIndentString(context.indent) << "\"" << attrName << "\"_attr = " << attrValue
                                   << ",\n";
                            continue;
                        }
                        stream << makeIndentString(context.indent) << ns(context.withinSvg, false) << attrName << " = "
                               << attrValue << ",\n";
                        continue;
                    }
                    if (!isKnownAttribute(attrName))
                    {
                        stream << makeIndentString(context.indent) << "\"" << attrName << "\"_attr = " << attrValue
                               << ",\n";
                        continue;
                    }
                    stream << makeIndentString(context.indent) << ns(context.withinSvg, false) << attrName << " = "
                           << attrValue << ",\n";
                }
            }

            stream << makeIndentString(context.indent) << "}(";

            {
                auto children = node.children();
                if (children.begin() != children.end())
                {
                    stream << "\n";
                    INDENT();
                    auto deeperContext = context;
                    deeperContext.isTopLevel = false;
                    for (auto iter = children.begin(), end = children.end(); iter != end; ++iter)
                    {
                        convertNode(*iter, stream, deeperContext);
                        if (std::next(iter) != end)
                            stream << ",\n";
                    }

                    context.withinSvg = false;
                    stream << "\n" << makeIndentString(context.indent - 1);
                }
            }

            if (context.isTopLevel)
                stream << ");\n";
            else
                stream << ")";
        }
        else if (node.type() == pugi::xml_node_type::node_pcdata)
        {
            stream << makeIndentString(context.indent) << "\"" << trimWhitespace(node.value()) << "\"";
        }
        else if (node.type() == pugi::xml_node_type::node_cdata)
        {
            stream << makeIndentString(context.indent) << "/* <![CDATA[" << node.value() << "]]> */\n";
        }
        else if (node.type() == pugi::xml_node_type::node_comment)
        {
            stream << makeIndentString(context.indent) << "/* " << node.value() << " */\n";
        }
        return stream;
    }
}

int main(int argc, char** argv)
{
    namespace po = boost::program_options;
    po::options_description desc("Allowed options");
    desc.add_options()("help,h", "produce help message")(
        "input,i", po::value<std::string>(), "input XML file (optional, defaults to stdin)")(
        "output,o", po::value<std::string>(), "output file (optional, defaults to stdout)")(
        "function,f", po::value<std::string>()->default_value("createElement"), "function name to wrap the output in")(
        "header,a",
        "whether to generate a header file with a function declaration instead of a source file with a function "
        "definition")(
        "namespace,n", po::value<std::string>()->default_value("NuiGenerated"), "namespace to wrap the function in")(
        "tabWidth,t", po::value<int>()->default_value(4), "number of spaces to use for indentation")(
        "lineWidth,l",
        po::value<int>()->default_value(120),
        "maximum line width before breaking (not implemented yet)")(
        "useNamespaceAbbreviations,u",
        "whether to use namespace abbreviations (e.g. se:: for Nui::Elements::Svg) in the generated code");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    if (vm.contains("help"))
    {
        std::cout << desc << "\n";
        return 0;
    }

    if (vm.contains("tabWidth"))
    {
        const int width = vm["tabWidth"].as<int>();
        if (width < 0)
        {
            std::cerr << "Invalid tab width: " << width << "\n";
            return 1;
        }
        indentWidth = width;
    }

    if (vm.contains("lineWidth"))
    {
        const int width = vm["lineWidth"].as<int>();
        if (width < 0)
        {
            std::cerr << "Invalid line width: " << width << "\n";
            return 1;
        }
        lineWidth = width;
    }

    bool abbreviateNamespaces = vm.contains("useNamespaceAbbreviations");

    std::string content;
    if (vm.contains("input"))
    {
        const auto inputFile = std::filesystem::path{vm["input"].as<std::string>()};
        content = readFile(inputFile);
    }
    else
    {
        // read from stdin
        std::ostringstream ss;
        ss << std::cin.rdbuf();
        content = ss.str();
    }

    // const auto knownElems = knownElements();
    // const auto knownAttrs = knownAttributes();
    // const auto knownSvgElems = knownSvgElements();
    // const auto knownSvgAttrs = knownSvgAttributes();
    // std::cerr << "Known Elements: " << fmt::format("{}", fmt::join(knownElems, ", ")) << "\n\n";
    // std::cerr << "Known Attributes: " << fmt::format("{}", fmt::join(knownAttrs, ", ")) << "\n\n";
    // std::cerr << "Known SVG Elements: " << fmt::format("{}", fmt::join(knownSvgElems, ", ")) << "\n\n";
    // std::cerr << "Known SVG Attributes: " << fmt::format("{}", fmt::join(knownSvgAttrs, ", ")) << "\n\n";

    std::optional<std::filesystem::path> outputFile{std::nullopt};
    if (vm.contains("output"))
    {
        outputFile = std::filesystem::path{vm["output"].as<std::string>()};
    }

    pugi::xml_document doc;
    const auto result = doc.load_string(content.c_str());
    if (!result)
    {
        std::cerr << "Failed to parse XML: " << result.description() << std::endl;
        return 1;
    }

    if (!doc.first_child())
    {
        std::cerr << "No elements in XML." << std::endl;
        return 1;
    }

    std::optional<std::ofstream> out{std::nullopt};
    std::ostream* stream = &std::cout;
    if (outputFile)
    {
        out.emplace(*outputFile, std::ios_base::binary);
        if (!*out)
        {
            std::cerr << "Failed to open output file: " << outputFile->string() << std::endl;
            return 1;
        }
        stream = &*out;
    }

    if (vm.contains("header"))
    {
        *stream << "#pragma once\n";
        *stream << "// This file was generated by xml-to-nui. Do not edit manually.\n\n";
        *stream << "#include <nui/frontend/elements.hpp>\n";
        *stream << "#include <nui/frontend/attributes.hpp>\n";
        *stream << "#include <nui/frontend/svg_elements.hpp>\n";
        *stream << "#include <nui/frontend/svg_attributes.hpp>\n\n";
        *stream << "namespace " << sanitizeName(vm["namespace"].as<std::string>()) << "\n{\n";
        *stream << "    inline Nui::ElementRenderer " << sanitizeName(vm["function"].as<std::string>()) << "() {\n";
        *stream << "        using namespace Nui::Attributes::Literals;\n";
        if (abbreviateNamespaces)
        {
            *stream << "        namespace se = Nui::Elements::Svg;\n";
            *stream << "        namespace sa = Nui::Attributes::Svg;\n";
            *stream << "        namespace nuie = Nui::Elements;\n";
            *stream << "        namespace nuia = Nui::Attributes;\n\n";
        }
        convertNode(
            doc.first_child(),
            *stream,
            PrintContext{
                .withinSvg = false,
                .indent = 2,
                .inFunction = true,
                .useNamespaceAbbreviations = abbreviateNamespaces});
        *stream << "    }\n";
        *stream << "}\n";
        return 0;
    }

    convertNode(doc.first_child(), *stream);

    return 0;
}