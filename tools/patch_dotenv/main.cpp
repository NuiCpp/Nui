#include <fstream>
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 4)
    {
        std::cout << "Expected 3 arguments: <.env file source> <.env file target> <nui-root-dir>, but got " << argc - 1
                  << "\n";
        std::cout << "Usage: " << argv[0] << " <.env file source> <.env file target> <nui-root-dir>" << "\n";
        return 1;
    }

    const std::string source = argv[1];
    const std::string target = argv[2];
    const std::string nuiRootDir = argv[3];

    std::ofstream ofs{target, std::ios_base::binary};
    if (!ofs)
    {
        std::cout << "Failed to open file: " << target << "\n";
        return 1;
    }

    std::ifstream ifs{source, std::ios_base::binary};
    if (!ifs)
    {
        ofs << "NUI_PROJECT_ROOT=" << nuiRootDir << "\n";
        return 0;
    }

    ofs << "NUI_PROJECT_ROOT=" << nuiRootDir << "\n";
    ofs << ifs.rdbuf();
    return 0;
}