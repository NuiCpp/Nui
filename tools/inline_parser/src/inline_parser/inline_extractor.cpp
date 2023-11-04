#include <inline_parser/inline_extractor.hpp>
#include <inline_parser/constants.hpp>
#include <inline_parser/parser.hpp>

#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>
#include <thread>

#include <iostream>
#include <regex>

namespace
{
    constexpr bool nameMatcher(char c)
    {
        return c != ')' && c != ',' && c != ' ' && c != '\t' && c != '\r' && c != '\n';
    }
}

InlineExtractor::InlineExtractor(std::function<void(std::string const&)> onWarning)
    : onWarning_{std::move(onWarning)}
    , sectionCache_{}
{}

bool InlineExtractor::parseInlineDirective(std::string const& line, Section& section) const
{
    if (line.empty())
        return false;

    using StringParser = Parser<std::string::const_iterator>;

    StringParser parser{line};
    auto state = parser.skipWhitespace();
    if (state == StringParser::State::ReachedEnd)
        return false;

    state = parser.matchComment();
    if (state != StringParser::State::Success)
        return false;

    state = parser.skipWhitespace();
    if (state == StringParser::State::ReachedEnd)
        return false;

    state = parser.match(Constants::inlineDirective);
    if (state != StringParser::State::Success)
        return false;

    // Warnings from here on out

    state = parser.skipWhitespace();
    if (state == StringParser::State::ReachedEnd)
    {
        onWarning_("Found start marker without parameters, skipping");
        return false;
    }

    state = parser.match('(');
    if (state != StringParser::State::Success)
    {
        onWarning_("Found start marker without parameters, skipping");
        return false;
    }

    state = parser.skipWhitespace();
    if (state == StringParser::State::ReachedEnd)
    {
        onWarning_("Marker parameter clause not closed on same line, skipping");
        return false;
    }

    auto [result, type] = parser.match(nameMatcher);
    if (result != StringParser::State::Success)
    {
        onWarning_("Expected type, skipping");
        return false;
    }

    state = parser.skipWhitespace();
    if (state == StringParser::State::ReachedEnd)
    {
        onWarning_("Unexpected end of line after marker type, skipping");
        return false;
    }

    state = parser.match(',');
    if (state != StringParser::State::Success)
    {
        onWarning_("Expected comma after marker type, skipping");
        return false;
    }

    state = parser.skipWhitespace();
    if (state == StringParser::State::ReachedEnd)
    {
        onWarning_("Unexpected end of line after comma, skipping");
        return false;
    }

    auto [result2, name] = parser.match(nameMatcher);
    if (result2 != StringParser::State::Success)
    {
        onWarning_("Expected name, skipping");
        return false;
    }

    state = parser.skipWhitespace();
    if (state == StringParser::State::ReachedEnd)
    {
        onWarning_("Unexpected end of line after marker name, skipping");
        return false;
    }

    state = parser.match(')');
    if (state != StringParser::State::Success)
    {
        onWarning_("Expected closing parenthesis after marker name, skipping");
        return false;
    }

    section.type = std::move(type);
    section.name = std::move(name);

    return true;
}

void InlineExtractor::postProcessImports(std::string& line)
{
    std::regex pattern(R"(^\s*js_import)");
    std::string replacement = "import";
    line = std::regex_replace(line, pattern, replacement);
}

void InlineExtractor::parseFileForSections(std::filesystem::path const& path)
{
    std::ifstream reader{path, std::ios_base::binary};
    if (!reader.is_open())
        return;

    std::string line;
    Section currentSection;
    std::function<void()> currentParser;
    std::function<void()> doParseInlineDirective;

    auto doSearchEndInlineDirective = [this, &currentSection, &line, &currentParser, &doParseInlineDirective]() {
        if (line.find(Constants::endInlineDirective) != std::string::npos)
        {
            {
                std::lock_guard<std::mutex> guard{sectionCacheMutex_};
                sectionCache_.addSection(currentSection);
                currentSection = {};
            }
            currentParser = doParseInlineDirective;
        }
        else
        {
            line.resize(line.size() + 1);
            line.back() = '\n';
            postProcessImports(line);
            currentSection.content.append(std::move(line));
        }
    };

    doParseInlineDirective = [this, &currentSection, &line, &currentParser, &doSearchEndInlineDirective]() {
        if (parseInlineDirective(line, currentSection))
            currentParser = doSearchEndInlineDirective;
    };

    currentParser = doParseInlineDirective;

    while (std::getline(reader, line))
    {
        currentParser();
    }
    if (!currentSection.type.empty())
    {
        onWarning_("Found start marker without end marker, skipping");
    }
}

void InlineExtractor::parallelParseFilesForSections(std::vector<std::filesystem::path> const& paths)
{
    boost::asio::thread_pool pool{
        std::min(paths.size(), static_cast<std::size_t>(std::thread::hardware_concurrency()))};

    for (auto const& path : paths)
    {
        boost::asio::post(pool, [this, path]() {
            parseFileForSections(path);
        });
    }

    pool.join();
}

SectionCache& InlineExtractor::sectionCache()
{
    return sectionCache_;
}
SectionCache const& InlineExtractor::sectionCache() const
{
    return sectionCache_;
}