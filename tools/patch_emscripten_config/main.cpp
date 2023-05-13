#include <string>
#include <iostream>
#include <fstream>

void patchKey(std::string& config, std::string const& key, std::string const& value)
{
    const auto pos = config.find(key);
    if (pos == std::string::npos)
    {
        std::cout << "Could not find " << key << "\n";
        std::exit(1);
    }

    // find equal sign:
    const auto pos2 = config.find('=', pos);
    if (pos2 == std::string::npos)
	{
		std::cout << "Could not find equal sign after " << key << "\n";
		std::exit(1);
	}

    // find single quote:
    const auto pos3 = config.find('\'', pos2);
    if (pos3 == std::string::npos)
    {
        std::cout << "Could not find single quote after " << key << "\n";
		std::exit(1);
    }

    // find closing single quote:
    std::size_t pos4;
    do {
        pos4 = config.find('\'', pos3 + 1);
        const auto nextEndline = config.find('\n', pos3 + 1);
        if (nextEndline != std::string::npos && nextEndline < pos4)
		{
			std::cout << "Line ended prematurely after " << key << "\n";
			std::exit(1);
		}
        if (pos4 == std::string::npos)
	    {
		    std::cout << "Could not find closing single quote after " << key << "\n";
            std::exit(1);
        }
    } while (config[pos4 - 1] == '\\');

    // replace quoted value:
    config.replace(pos3 + 1, pos4 - pos3 - 1, value);
}

int main(int argc, char const* const* argv)
{
    std::string emscriptenConfigPath;
    std::string binaryenRoot;
    std::string javaExec;
    std::string nodeExec;
    std::string llvmRoot;

    if (argc < 3)
    {
        std::cout << "Expected at least 2 arguments.\n";
        std::cout << "Usage: " << argv[0] << " <.emscripten config path> <binaryen root> <java exec> <node exec> <llvm root>"
                  << "\n";
        return 1;
    }

    emscriptenConfigPath = argv[1];
    binaryenRoot = argv[2];

    if (argc >= 4)
        javaExec = argv[3];

    if (argc >= 5)
        nodeExec = argv[4];

    if (argc >= 6)
        llvmRoot = argv[5];

    std::string fileContent;
    {
        std::ifstream ifs{emscriptenConfigPath, std::ios_base::binary};
        if (!ifs)
        {
            std::cout << "Failed to open file: " << argv[1] << "\n";
            return 1;
        }
        ifs.seekg(0, std::ios_base::end);
        fileContent = std::string(ifs.tellg(), '\0');
        ifs.seekg(0);
        ifs.read(fileContent.data(), fileContent.size());
    }
    {
        std::ofstream ofs{emscriptenConfigPath + ".orig", std::ios_base::binary};
        ofs.write(fileContent.data(), fileContent.size());
    }

    if (!binaryenRoot.empty())
        patchKey(fileContent, "BINARYEN_ROOT", binaryenRoot);

    if (!javaExec.empty())
        patchKey(fileContent, "JAVA", javaExec);

    if (!nodeExec.empty())
        patchKey(fileContent, "NODE_JS", nodeExec);

    if (!llvmRoot.empty())
        patchKey(fileContent, "LLVM_ROOT", llvmRoot);

	std::ofstream ofs{emscriptenConfigPath, std::ios_base::binary};
	ofs.write(fileContent.data(), fileContent.size());
}