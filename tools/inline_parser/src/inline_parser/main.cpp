#include <iostream>

#include <inline_parser/inline_extractor.hpp>

int main(int argc, char** argv)
{
    if (argc < 5)
    {
        std::cerr << "Usage: " << argv[0] << " <cache> <base_dir> <unpack_dir> <files>..." << std::endl;
        return 1;
    }

    std::filesystem::path cachePath{argv[1]};
    std::filesystem::path baseDir{argv[2]};
    std::filesystem::path unpackDir{argv[3]};

    std::vector<std::filesystem::path> paths;
    for (int i = 4; i < argc; ++i)
        paths.emplace_back(argv[i]);

    InlineExtractor extractor{[](std::string const& warning) {
        std::cerr << warning << std::endl;
    }};
    extractor.sectionCache().fromFile(cachePath);
    extractor.parallelParseFilesForSections(paths);

    // save:
    extractor.sectionCache().removeColdSections();
    extractor.sectionCache().toFile(cachePath);
    extractor.sectionCache().produceToDirectory(baseDir, unpackDir);
}