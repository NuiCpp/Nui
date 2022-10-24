#include <fstream>
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cout << "Expected 1 argument: <acorn_optimizer.js path>, but got " << argc - 1 << "\n";
        std::cout << "Usage: " << argv[0] << " <acorn_optimizer.js path>"
                  << "\n";
        return 1;
    }

    std::string fileContent;
    {
        std::ifstream ifs{argv[1], std::ios_base::binary};
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

    auto pos = fileContent.find("exportES6 = false");
    if (pos == std::string::npos)
    {
        std::cout << "Could not find exportES6 = false in " << argv[1] << "\n";
        return 1;
    }
    // replace with true:
    fileContent.replace(pos, 17, "exportES6 = true");
    {
        std::ofstream ofs{argv[1], std::ios_base::binary};
        ofs.write(fileContent.data(), fileContent.size());
    }
}