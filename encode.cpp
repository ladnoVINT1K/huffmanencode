#include "encode.h"
#include <locale>
#include <codecvt>
#include <string>

int main(int argc, char** argv) {
    std::locale::global(std::locale("ru_RU.UTF-8"));
    std::string t = argv[argc - 1];
    if (t == "-decode") {
        std::string filename;
        std::cin >> filename;
        try {
            std::ifstream in;
            in.open(filename, std::ifstream::in | std::ifstream::binary);
            huffmanTree T(in, true);
            T.decode(in, filename + "-decoded");
            std::cout << "sucsessfull!\n";
            for (auto i : T.getCodeTable()) std::cout << i.first << ' ' << i.second << '\n';
            in.close();
        } catch (std::exception e) {
            std::cerr << e.what();
            return 1;
        }
    } else if (t == "-encode") {
        std::string filename;
        std::cin >> filename;
        try {
            std::ifstream in;
            in.open(filename, std::ifstream::in | std::ifstream::binary);
            huffmanTree T(in);
            T.encode(in, filename + "-encoded");
            std::cout << "sucsessfull!";
            in.close();
        } catch (std::exception e) {
            std::cerr << e.what();
            return 1;
        }
    } else if (t == "-codetable") {
        std::string filename;
        std::cin >> filename;
        try {
            std::ifstream in;
            in.open(filename, std::ifstream::in | std::ifstream::binary);
            std::ofstream out;
            out.open(filename + "-codetable", std::ofstream::out | std::ifstream::binary);
            huffmanTree T(in);
            T.writeHeader(out);
            in.close();
            out.close();
        } catch (std::exception e) {
            std::cerr << e.what();
            return 1;
        }
    } else {
        std::cerr << "Invalid argument!";
    }
    return 0;
}