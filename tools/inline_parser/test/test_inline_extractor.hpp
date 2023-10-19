#include <inline_parser/inline_extractor.hpp>
#include "temp_dir.hpp"

#include <gtest/gtest.h>

#include <fstream>
#include <sstream>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace Nui::Tests
{
    class TestInlineExtractor : public ::testing::Test
    {
      protected:
        void createFile(std::string const& name, std::string content)
        {
            std::stringstream ss{content};
            std::string line;
            content.clear();
            while (std::getline(ss, line))
            {
                if (!line.empty())
                {
                    line += '\n';
                }
                // trim front whitespace:
                line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](int ch) {
                               return !std::isspace(ch);
                           }));
                content += line;
            }

            createdFiles_.push_back(tempDir_.path() / name);
            std::ofstream{tempDir_.path() / name} << content;
        }

        std::string toLinuxLineEndings(std::string const& str)
        {
            std::stringstream sstr{str};
            std::string line;
            std::string result;
            while (std::getline(sstr, line))
            {
                if (line.back() == '\r')
                    line.pop_back();
                result += line + '\n';
            }
            return result;
        }

        void parallelParse()
        {
            extractor_.parallelParseFilesForSections(createdFiles_);
        }

        SectionCache& cache()
        {
            return extractor_.sectionCache();
        }

      protected:
        TempDir tempDir_;
        std::vector<std::filesystem::path> createdFiles_;
        InlineExtractor extractor_;
    };

    TEST_F(TestInlineExtractor, EmptyFileResultsEmptyCache)
    {
        createFile("empty.cpp", "");

        parallelParse();

        EXPECT_EQ(cache().begin(), cache().end());
    }

    TEST_F(TestInlineExtractor, MultipleEmptyFilesResultsEmptyCache)
    {
        createFile("empty1.cpp", "");
        createFile("empty2.cpp", "");
        createFile("empty3.cpp", "");

        parallelParse();

        EXPECT_EQ(cache().begin(), cache().end());
    }

    TEST_F(TestInlineExtractor, SourceFileWithoutSectionsResultsEmptyCache)
    {
        createFile("source.cpp", "int main() {}");

        parallelParse();

        EXPECT_EQ(cache().begin(), cache().end());
    }

    TEST_F(TestInlineExtractor, SingleSourceSectionIsParsed)
    {
        createFile("source.cpp", R"(
            int main() {}
            // @inline(js, foo)
            let x = () => {};
            // @endinline
        )");

        parallelParse();

        ASSERT_NE(cache().begin(), cache().end());
        EXPECT_EQ(cache().begin()->second.type, "js");
        EXPECT_EQ(cache().begin()->second.name, "foo");
        EXPECT_EQ(toLinuxLineEndings(cache().begin()->second.content), "let x = () => {};\n");
    }

    TEST_F(TestInlineExtractor, MultipleSourceSectionsAreParsed)
    {
        createFile("source.cpp", R"(
            int main() {}
            // @inline(js, foo)
            let x = () => {};
            // @endinline
            // @inline(js, bar)
            let y = () => {};
            // @endinline
        )");

        parallelParse();

        ASSERT_EQ(std::distance(cache().begin(), cache().end()), 2);

        auto jsfoo = cache().find("js", "foo");
        ASSERT_NE(jsfoo, cache().end());
        EXPECT_EQ(jsfoo->second.type, "js");
        EXPECT_EQ(jsfoo->second.name, "foo");
        EXPECT_EQ(toLinuxLineEndings(jsfoo->second.content), "let x = () => {};\n");

        auto jsbar = cache().find("js", "bar");
        ASSERT_NE(jsbar, cache().end());
        EXPECT_EQ(jsbar->second.type, "js");
        EXPECT_EQ(jsbar->second.name, "bar");
        EXPECT_EQ(toLinuxLineEndings(jsbar->second.content), "let y = () => {};\n");
    }

    TEST_F(TestInlineExtractor, MissingEndMarkerSkipsEverything)
    {
        createFile("source.cpp", R"(
            int main() {}
            // @inline(js, foo)
            let x = () => {};
        )");

        parallelParse();

        EXPECT_EQ(cache().begin(), cache().end());
    }

    TEST_F(TestInlineExtractor, MissingEndMarkerSkipsEverything2)
    {
        createFile("source.cpp", R"(
            int main() {}
            // @inline(js, foo)
            let x = () => {};
            // @inline(js, foo2)
            let x = () => {};
        )");

        parallelParse();

        EXPECT_EQ(cache().begin(), cache().end());
    }

    TEST_F(TestInlineExtractor, SpaceAfterCommentIsOptional)
    {
        createFile("source.cpp", R"(
            int main() {}
            //@inline(js, foo)
            let x = () => {};
            //@endinline
        )");

        parallelParse();

        ASSERT_NE(cache().begin(), cache().end());
        EXPECT_EQ(cache().begin()->second.type, "js");
        EXPECT_EQ(cache().begin()->second.name, "foo");
        EXPECT_EQ(toLinuxLineEndings(cache().begin()->second.content), "let x = () => {};\n");
    }

    TEST_F(TestInlineExtractor, MoreSpaceIsAllowedAfterComment)
    {
        createFile("source.cpp", R"(
            int main() {}
            //      @inline(js, foo)
            let x = () => {};
            //   @endinline
        )");

        parallelParse();

        ASSERT_NE(cache().begin(), cache().end());
        EXPECT_EQ(cache().begin()->second.type, "js");
        EXPECT_EQ(cache().begin()->second.name, "foo");
        EXPECT_EQ(toLinuxLineEndings(cache().begin()->second.content), "let x = () => {};\n");
    }

    TEST_F(TestInlineExtractor, SpaceIsAllowedBeforeParameters)
    {
        createFile("source.cpp", R"(
            int main() {}
            // @inline (js, foo)
            let x = () => {};
            // @endinline
        )");

        parallelParse();

        ASSERT_NE(cache().begin(), cache().end());
        EXPECT_EQ(cache().begin()->second.type, "js");
        EXPECT_EQ(cache().begin()->second.name, "foo");
        EXPECT_EQ(toLinuxLineEndings(cache().begin()->second.content), "let x = () => {};\n");
    }

    TEST_F(TestInlineExtractor, SpaceIsAllowedAfterParameters)
    {
        createFile("source.cpp", R"(
            int main() {}
            // @inline(js, foo)
            let x = () => {};
            // @endinline
        )");

        parallelParse();

        ASSERT_NE(cache().begin(), cache().end());
        EXPECT_EQ(cache().begin()->second.type, "js");
        EXPECT_EQ(cache().begin()->second.name, "foo");
        EXPECT_EQ(toLinuxLineEndings(cache().begin()->second.content), "let x = () => {};\n");
    }

    TEST_F(TestInlineExtractor, SpaceIsAllowedBeforeComma)
    {
        createFile("source.cpp", R"(
            int main() {}
            // @inline(js , foo)
            let x = () => {};
            // @endinline
        )");

        parallelParse();

        ASSERT_NE(cache().begin(), cache().end());
        EXPECT_EQ(cache().begin()->second.type, "js");
        EXPECT_EQ(cache().begin()->second.name, "foo");
        EXPECT_EQ(toLinuxLineEndings(cache().begin()->second.content), "let x = () => {};\n");
    }

    TEST_F(TestInlineExtractor, SpaceIsOptionalAfterComma)
    {
        createFile("source.cpp", R"(
            int main() {}
            // @inline(js,foo)
            let x = () => {};
            // @endinline
        )");

        parallelParse();

        ASSERT_NE(cache().begin(), cache().end());
        EXPECT_EQ(cache().begin()->second.type, "js");
        EXPECT_EQ(cache().begin()->second.name, "foo");
        EXPECT_EQ(toLinuxLineEndings(cache().begin()->second.content), "let x = () => {};\n");
    }

    TEST_F(TestInlineExtractor, SpaceIsAllowedBeforeClosingParen)
    {
        createFile("source.cpp", R"(
            int main() {}
            // @inline(js, foo )
            let x = () => {};
            // @endinline
        )");

        parallelParse();

        ASSERT_NE(cache().begin(), cache().end());
        EXPECT_EQ(cache().begin()->second.type, "js");
        EXPECT_EQ(cache().begin()->second.name, "foo");
        EXPECT_EQ(toLinuxLineEndings(cache().begin()->second.content), "let x = () => {};\n");
    }

    TEST_F(TestInlineExtractor, ContentCanBeMultipleLines)
    {
        createFile("source.cpp", R"(
            int main() {}
            // @inline(js, foo)
            let x = () => {
                let y = 1;
                let z = 2;
            };
            // @endinline
        )");

        parallelParse();

        ASSERT_NE(cache().begin(), cache().end());
        EXPECT_EQ(cache().begin()->second.type, "js");
        EXPECT_EQ(cache().begin()->second.name, "foo");
        EXPECT_EQ(toLinuxLineEndings(cache().begin()->second.content), "let x = () => {\nlet y = 1;\nlet z = 2;\n};\n");
    }

    TEST_F(TestInlineExtractor, ContentCanBeMultipleLines2)
    {
        createFile("source.cpp", R"(
            int main() {}
            // @inline(js, foo)
            let x = () => {
                let y = 1;
                let z = 2;
            };
            // @endinline
            // @inline(js, bar)
            let y = () => {
                let y = 1;
                let z = 2;
            };
            // @endinline
        )");

        parallelParse();

        ASSERT_EQ(std::distance(cache().begin(), cache().end()), 2);

        auto jsfoo = cache().find("js", "foo");
        ASSERT_NE(jsfoo, cache().end());
        EXPECT_EQ(jsfoo->second.type, "js");
        EXPECT_EQ(jsfoo->second.name, "foo");
        EXPECT_EQ(toLinuxLineEndings(jsfoo->second.content), "let x = () => {\nlet y = 1;\nlet z = 2;\n};\n");

        auto jsbar = cache().find("js", "bar");
        ASSERT_NE(jsbar, cache().end());
        EXPECT_EQ(jsbar->second.type, "js");
        EXPECT_EQ(jsbar->second.name, "bar");
        EXPECT_EQ(toLinuxLineEndings(jsbar->second.content), "let y = () => {\nlet y = 1;\nlet z = 2;\n};\n");
    }

    TEST_F(TestInlineExtractor, MultipleContentOfMultipleTypesIsPossible)
    {
        createFile("source.cpp", R"(
            int main() {}
            // @inline(js, foo)
            let x = () => {
                let y = 1;
                let z = 2;
            };
            // @endinline
            // @inline(css, bar)
            .foo {
                color: red;
            }
            // @endinline
        )");

        parallelParse();

        ASSERT_EQ(std::distance(cache().begin(), cache().end()), 2);

        auto jsfoo = cache().find("js", "foo");
        ASSERT_NE(jsfoo, cache().end());
        EXPECT_EQ(jsfoo->second.type, "js");
        EXPECT_EQ(jsfoo->second.name, "foo");
        EXPECT_EQ(toLinuxLineEndings(jsfoo->second.content), "let x = () => {\nlet y = 1;\nlet z = 2;\n};\n");

        auto cssbar = cache().find("css", "bar");
        ASSERT_NE(cssbar, cache().end());
        EXPECT_EQ(cssbar->second.type, "css");
        EXPECT_EQ(cssbar->second.name, "bar");
        EXPECT_EQ(toLinuxLineEndings(cssbar->second.content), ".foo {\ncolor: red;\n}\n");
    }

    TEST_F(TestInlineExtractor, DuplicateSectionNameOverwritesPrevious)
    {
        createFile("source.cpp", R"(
            int main() {}
            // @inline(js, foo)
            let x = () => {
                let y = 1;
                let z = 2;
            };
            // @endinline
            // @inline(js, foo)
            let y = () => {
                let y = 3;
                let z = 4;
            };
            // @endinline
        )");

        parallelParse();

        ASSERT_EQ(std::distance(cache().begin(), cache().end()), 1);

        auto jsfoo = cache().find("js", "foo");
        ASSERT_NE(jsfoo, cache().end());
        EXPECT_EQ(jsfoo->second.type, "js");
        EXPECT_EQ(jsfoo->second.name, "foo");
        EXPECT_EQ(toLinuxLineEndings(jsfoo->second.content), "let y = () => {\nlet y = 3;\nlet z = 4;\n};\n");
    }

    TEST_F(TestInlineExtractor, DuplicateSectionNameButDifferentTypeDoesNotOverwritePrevious)
    {
        createFile("source.cpp", R"(
            int main() {}
            // @inline(js, foo)
            let x = () => {
                let y = 1;
                let z = 2;
            };
            // @endinline
            // @inline(css, foo)
            .foo {
                color: red;
            }
            // @endinline
        )");

        parallelParse();

        ASSERT_EQ(std::distance(cache().begin(), cache().end()), 2);

        auto jsfoo = cache().find("js", "foo");
        ASSERT_NE(jsfoo, cache().end());
        EXPECT_EQ(jsfoo->second.type, "js");
        EXPECT_EQ(jsfoo->second.name, "foo");
        EXPECT_EQ(toLinuxLineEndings(jsfoo->second.content), "let x = () => {\nlet y = 1;\nlet z = 2;\n};\n");

        auto cssfoo = cache().find("css", "foo");
        ASSERT_NE(cssfoo, cache().end());
        EXPECT_EQ(cssfoo->second.type, "css");
        EXPECT_EQ(cssfoo->second.name, "foo");
        EXPECT_EQ(toLinuxLineEndings(cssfoo->second.content), ".foo {\ncolor: red;\n}\n");
    }

    TEST_F(TestInlineExtractor, SectionLoadedFromCacheIsOverwritten)
    {
        createFile("source.cpp", R"(
            int main() {}
            // @inline(js, foo)
            let x = () => {};
            // @endinline
        )");

        parallelParse();

        ASSERT_NE(cache().begin(), cache().end());
        EXPECT_EQ(cache().begin()->second.type, "js");
        EXPECT_EQ(cache().begin()->second.name, "foo");
        EXPECT_EQ(toLinuxLineEndings(cache().begin()->second.content), "let x = () => {};\n");

        cache().toFile(tempDir_.path() / "cache.json");
        cache() = {};
        cache().fromFile(tempDir_.path() / "cache.json");

        createdFiles_.clear();
        createFile("source.cpp", R"(
            int main() {}
            // @inline(js, foo)
            let x = () => {
                let y = 3;
                let z = 4;
            };
            // @endinline
        )");

        parallelParse();
        ASSERT_NE(cache().begin(), cache().end());
        EXPECT_EQ(cache().begin()->second.type, "js");
        EXPECT_EQ(cache().begin()->second.name, "foo");
        EXPECT_EQ(toLinuxLineEndings(cache().begin()->second.content), "let x = () => {\nlet y = 3;\nlet z = 4;\n};\n");
    }

    TEST_F(TestInlineExtractor, LoadedSectionsAreCold)
    {
        createFile("source.cpp", R"(
            int main() {}
            // @inline(js, foo)
            let x = () => {};
            // @endinline
        )");

        parallelParse();

        EXPECT_EQ(cache().coldBegin(), cache().coldEnd());

        cache().toFile(tempDir_.path() / "cache.json");
        cache() = {};
        cache().fromFile(tempDir_.path() / "cache.json");

        EXPECT_NE(cache().coldBegin(), cache().coldEnd());
        EXPECT_EQ(std::distance(cache().coldBegin(), cache().coldEnd()), 1);
        EXPECT_EQ(*cache().coldBegin(), "js.foo");
    }

    TEST_F(TestInlineExtractor, OverwrittenSectionsAreNoLongerCold)
    {
        createFile("source.cpp", R"(
            int main() {}
            // @inline(js, foo)
            let x = () => {};
            // @endinline
        )");

        parallelParse();

        EXPECT_EQ(cache().coldBegin(), cache().coldEnd());

        cache().toFile(tempDir_.path() / "cache.json");
        cache() = {};
        cache().fromFile(tempDir_.path() / "cache.json");

        EXPECT_NE(cache().coldBegin(), cache().coldEnd());
        EXPECT_EQ(std::distance(cache().coldBegin(), cache().coldEnd()), 1);
        EXPECT_EQ(*cache().coldBegin(), "js.foo");

        createdFiles_.clear();
        createFile("source.cpp", R"(
            int main() {}
            // @inline(js, foo)
            let y = () => {};
            // @endinline
        )");

        parallelParse();

        EXPECT_EQ(cache().coldBegin(), cache().coldEnd());
    }

    TEST_F(TestInlineExtractor, ColdSectionsAreRemovedByRemoveColdSections)
    {
        createFile("source.cpp", R"(
            int main() {}
            // @inline(js, foo)
            let x = () => {};
            // @endinline
        )");

        parallelParse();

        EXPECT_EQ(cache().coldBegin(), cache().coldEnd());

        cache().toFile(tempDir_.path() / "cache.json");
        cache() = {};
        cache().fromFile(tempDir_.path() / "cache.json");

        EXPECT_NE(cache().coldBegin(), cache().coldEnd());
        EXPECT_EQ(std::distance(cache().coldBegin(), cache().coldEnd()), 1);
        EXPECT_EQ(*cache().coldBegin(), "js.foo");

        cache().removeColdSections();

        EXPECT_EQ(cache().coldBegin(), cache().coldEnd());
        EXPECT_EQ(cache().begin(), cache().end());
    }

    TEST_F(TestInlineExtractor, ScriptsAreWrittenToFiles)
    {
        createFile("source.cpp", R"(
            int main() {}
            // @inline(js, foo)
            let x = () => {};
            // @endinline
            // @inline(js, bar)
            let y = () => {};
            // @endinline
        )");

        parallelParse();

        cache().produceToDirectory(tempDir_.path(), "", false);

        std::ifstream reader{tempDir_.path() / "js/foo.js"};
        ASSERT_TRUE(reader.is_open());
        std::string content;
        std::getline(reader, content, '\0');
        EXPECT_EQ(toLinuxLineEndings(content), "let x = () => {};\n");

        reader = std::ifstream{tempDir_.path() / "js/bar.js"};
        ASSERT_TRUE(reader.is_open());
        std::getline(reader, content, '\0');
        EXPECT_EQ(toLinuxLineEndings(content), "let y = () => {};\n");

        reader = std::ifstream{tempDir_.path() / "inline_imports.js"};
        ASSERT_TRUE(reader.is_open());
        std::getline(reader, content, '\0');
        bool importsEqualInAnyOrder = content == "import './js/foo.js';\nimport './js/bar.js';\n" ||
            content == "import './js/bar.js';\nimport './js/foo.js';\n";
        EXPECT_TRUE(importsEqualInAnyOrder);
    }

    TEST_F(TestInlineExtractor, StylesAreWrittenToFiles)
    {
        createFile("source.cpp", R"(
            int main() {}
            // @inline(css, foo)
            .foo {
                color: red;
            }
            // @endinline
            // @inline(css, bar)
            .bar {
                color: blue;
            }
            // @endinline
        )");

        parallelParse();

        cache().produceToDirectory(tempDir_.path(), "", false);

        std::ifstream reader{tempDir_.path() / "css/foo.css"};
        ASSERT_TRUE(reader.is_open());
        std::string content;
        std::getline(reader, content, '\0');
        EXPECT_EQ(toLinuxLineEndings(content), ".foo {\ncolor: red;\n}\n");

        reader = std::ifstream{tempDir_.path() / "css/bar.css"};
        ASSERT_TRUE(reader.is_open());
        std::getline(reader, content, '\0');
        EXPECT_EQ(toLinuxLineEndings(content), ".bar {\ncolor: blue;\n}\n");

        reader = std::ifstream{tempDir_.path() / "inline_imports.css"};
        ASSERT_TRUE(reader.is_open());
        std::getline(reader, content, '\0');
        bool importsEqualInAnyOrder = content == "@import \"./css/foo.css\";\n@import \"./css/bar.css\";\n" ||
            content == "@import \"./css/bar.css\";\n@import \"./css/foo.css\";\n";
        EXPECT_TRUE(importsEqualInAnyOrder);
    }

    TEST_F(TestInlineExtractor, AMixOfScriptsAndStylesIsWrittenToFiles)
    {
        createFile("source.cpp", R"(
            int main() {}
            // @inline(js, foo)
            let x = () => {};
            // @endinline
            // @inline(css, bar)
            .bar {
                color: blue;
            }
            // @endinline
        )");

        parallelParse();

        cache().produceToDirectory(tempDir_.path(), "", false);

        std::ifstream reader{tempDir_.path() / "js/foo.js"};
        ASSERT_TRUE(reader.is_open());
        std::string content;
        std::getline(reader, content, '\0');
        EXPECT_EQ(toLinuxLineEndings(content), "let x = () => {};\n");

        reader = std::ifstream{tempDir_.path() / "css/bar.css"};
        ASSERT_TRUE(reader.is_open());
        std::getline(reader, content, '\0');
        EXPECT_EQ(toLinuxLineEndings(content), ".bar {\ncolor: blue;\n}\n");

        reader = std::ifstream{tempDir_.path() / "inline_imports.js"};
        ASSERT_TRUE(reader.is_open());
        std::getline(reader, content, '\0');
        EXPECT_EQ(toLinuxLineEndings(content), "import './js/foo.js';\n");

        reader = std::ifstream{tempDir_.path() / "inline_imports.css"};
        ASSERT_TRUE(reader.is_open());
        std::getline(reader, content, '\0');
        EXPECT_EQ(toLinuxLineEndings(content), "@import \"./css/bar.css\";\n");
    }

    TEST_F(TestInlineExtractor, JavascriptAndTypescriptAreWrittenToFiles)
    {
        createFile("source.cpp", R"(
            int main() {}
            // @inline(js, foo)
            let x = () => {};
            // @endinline
            // @inline(ts, bar)
            let y = () => {};
            // @endinline
        )");

        parallelParse();

        cache().produceToDirectory(tempDir_.path(), "", false);

        std::ifstream reader{tempDir_.path() / "js/foo.js"};
        ASSERT_TRUE(reader.is_open());
        std::string content;
        std::getline(reader, content, '\0');
        EXPECT_EQ(toLinuxLineEndings(content), "let x = () => {};\n");

        reader = std::ifstream{tempDir_.path() / "ts/bar.ts"};
        ASSERT_TRUE(reader.is_open());
        std::getline(reader, content, '\0');
        EXPECT_EQ(toLinuxLineEndings(content), "let y = () => {};\n");

        reader = std::ifstream{tempDir_.path() / "inline_imports.js"};
        ASSERT_TRUE(reader.is_open());
        std::getline(reader, content, '\0');
        bool importsEqualInAnyOrder = content == "import './js/foo.js';\nimport './ts/bar.ts';\n" ||
            content == "import './ts/bar.ts';\nimport './js/foo.js';\n";
        EXPECT_TRUE(importsEqualInAnyOrder);
    }

    TEST_F(TestInlineExtractor, LessSassScssCssAreWrittenToFiles)
    {
        createFile("source.cpp", R"(
            int main() {}
            // @inline(less, foo)
            .foo {
                color: red;
            }
            // @endinline
            // @inline(sass, bar)
            .bar {
                color: blue;
            }
            // @endinline
            // @inline(scss, baz)
            .baz {
                color: green;
            }
            // @endinline
            // @inline(css, qux)
            .qux {
                color: yellow;
            }
            // @endinline
        )");

        parallelParse();

        cache().produceToDirectory(tempDir_.path(), "", false);

        std::ifstream reader{tempDir_.path() / "less/foo.less"};
        ASSERT_TRUE(reader.is_open());
        std::string content;
        std::getline(reader, content, '\0');
        EXPECT_EQ(toLinuxLineEndings(content), ".foo {\ncolor: red;\n}\n");

        reader = std::ifstream{tempDir_.path() / "sass/bar.sass"};
        ASSERT_TRUE(reader.is_open());
        std::getline(reader, content, '\0');
        EXPECT_EQ(toLinuxLineEndings(content), ".bar {\ncolor: blue;\n}\n");

        reader = std::ifstream{tempDir_.path() / "scss/baz.scss"};
        ASSERT_TRUE(reader.is_open());
        std::getline(reader, content, '\0');
        EXPECT_EQ(toLinuxLineEndings(content), ".baz {\ncolor: green;\n}\n");

        reader = std::ifstream{tempDir_.path() / "css/qux.css"};
        ASSERT_TRUE(reader.is_open());
        std::getline(reader, content, '\0');
        EXPECT_EQ(toLinuxLineEndings(content), ".qux {\ncolor: yellow;\n}\n");

        reader = std::ifstream{tempDir_.path() / "inline_imports.css"};
        ASSERT_TRUE(reader.is_open());
        std::getline(reader, content, '\0');

        std::vector<std::string> splitImports;
        std::stringstream ss{content};
        std::string line;
        while (std::getline(ss, line))
        {
            splitImports.push_back(line);
        }
        std::sort(splitImports.begin(), splitImports.end());
        ASSERT_EQ(splitImports.size(), 4);
        EXPECT_EQ(splitImports[0], "@import \"./css/qux.css\";");
        EXPECT_EQ(splitImports[1], "@import \"./less/foo.less\";");
        EXPECT_EQ(splitImports[2], "@import \"./sass/bar.sass\";");
        EXPECT_EQ(splitImports[3], "@import \"./scss/baz.scss\";");
    }
}