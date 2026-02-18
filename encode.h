#pragma once
#include <fstream>
#include <iostream>
#include <exception>
#include <unordered_map>
#include <queue>
#include <cstdint>
#include <algorithm>
#include <vector>

struct Note {
    unsigned char s_;
    long long w_ = 0;
    Note* l_ = nullptr;
    Note* r_ = nullptr;
    
    explicit Note(unsigned char s = '\0', long long w = 0, Note* l = nullptr, Note* r = nullptr)
        : s_(s), w_(w), l_(l), r_(r) {}
};

class huffmanTree {
public:
    explicit huffmanTree(std::ifstream& input);
    huffmanTree(std::ifstream& binaryInput, bool fromBinary);
    ~huffmanTree();

    std::unordered_map<unsigned char, std::string> getCodeTable() const {
        std::unordered_map<unsigned char, std::string> codes;
        if (HEAD)
            generateCodes(HEAD, "", codes);
        return codes;
    }

    void encode(std::ifstream& input, const std::string& outputFilename);
    void decode(std::ifstream& binaryInput, const std::string& outputFilename);
    void writeHeader(std::ofstream& out);
    void readHeader(std::ifstream& in);
    
private:
    Note* HEAD = nullptr;
    std::unordered_map<unsigned char, std::string> codeTable;  // Изменено на unsigned char

    void deleteTree(Note* node);
    void generateCodes(Note* node, std::string code, 
        std::unordered_map<unsigned char, std::string>& codes) const;

    std::unordered_map<unsigned char, long long> readFrequencies(std::ifstream& in);
    unsigned long long size = 0;
    void buildTreeFromCodes();

    void writeBit(std::ofstream& out, unsigned char& buf, int& count, bool bit);
    void flushBits(std::ofstream& out, unsigned char& buf, int& count);
};

huffmanTree::huffmanTree(std::ifstream& input) {
    if (!input.is_open())
        throw std::runtime_error("Input file not open");
    
    size = 0;
    auto freq = readFrequencies(input);
    
    if (freq.empty())
        return;
    
    if (freq.size() == 1) {
        HEAD = new Note{freq.begin()->first, freq.begin()->second};
        HEAD->l_ = new Note{'\0', 0};
        HEAD->r_ = new Note{'\0', 0};
        generateCodes(HEAD, "", codeTable);
        return;
    }

    auto cmp = [](Note* a, Note* b) { return a->w_ > b->w_; };
    std::priority_queue<Note*, std::vector<Note*>, decltype(cmp)> pq(cmp);

    for (auto& [s, w] : freq)
        pq.push(new Note(s, w));

    while (pq.size() > 1) {
        Note* l = pq.top(); pq.pop();
        Note* r = pq.top(); pq.pop();
        pq.push(new Note('\0', l->w_ + r->w_, l, r));
    }

    HEAD = pq.top();
    generateCodes(HEAD, "", codeTable);
}

huffmanTree::huffmanTree(std::ifstream& binaryInput, bool fromBinary) {
    if (!fromBinary) return;
    if (!binaryInput.is_open())
        throw std::runtime_error("Binary file not open");

    readHeader(binaryInput);
    buildTreeFromCodes();
}

huffmanTree::~huffmanTree() {
    deleteTree(HEAD);
}

void huffmanTree::deleteTree(Note* n) {
    if (!n) return;
    deleteTree(n->l_);
    deleteTree(n->r_);
    delete n;
}

void huffmanTree::generateCodes(Note* node, std::string code, std::unordered_map<unsigned char, std::string>& codes) const {

    if (!node) return;
    if (!node->l_ && !node->r_) {
        codes[node->s_] = code.empty() ? "0" : code;
        return;
    }

    generateCodes(node->l_, code + "0", codes);
    generateCodes(node->r_, code + "1", codes);
}

std::unordered_map<unsigned char, long long> huffmanTree::readFrequencies(std::ifstream& in) {
    std::unordered_map<unsigned char, long long> f;
    
    // Читаем как бинарный файл
    in.seekg(0, std::ios::end);
    std::streampos fileSize = in.tellg();
    in.seekg(0);
    
    std::vector<unsigned char> buffer(fileSize);
    in.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    size = fileSize;
    
    for (unsigned char c : buffer) {
        f[c]++;
    }
    
    in.seekg(0);
    return f;
}

void huffmanTree::writeHeader(std::ofstream& out) {
    uint32_t count = codeTable.size();
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));
    
    for (auto& [sym, code] : codeTable) {
        uint64_t packed = 1;
        for (char b : code)
            packed = (packed << 1) | (b == '1');
        
        out.put(static_cast<char>(sym));
        out.write(reinterpret_cast<const char*>(&packed), sizeof(packed));
    }
    out.write(reinterpret_cast<const char*>(&size), sizeof(size));
}

void huffmanTree::readHeader(std::ifstream& in) {
    uint32_t count;
    in.read(reinterpret_cast<char*>(&count), sizeof(count));
    codeTable.clear();

    for (uint32_t i = 0; i < count; ++i) {
        char symChar;
        uint64_t packed;

        in.get(symChar);
        unsigned char sym = static_cast<unsigned char>(symChar);
        in.read(reinterpret_cast<char*>(&packed), sizeof(packed));

        std::string code;
        while (packed > 1) {
            code.push_back((packed & 1) ? '1' : '0');
            packed >>= 1;
        }
        std::reverse(code.begin(), code.end());
        codeTable[sym] = code;
    }
    in.read(reinterpret_cast<char*>(&size), sizeof(size));
}

void huffmanTree::buildTreeFromCodes() {
    HEAD = new Note('\0', 0);

    for (auto& [sym, code] : codeTable) {
        Note* cur = HEAD;
        
        if (code == "0") {
            cur->l_ = new Note(sym, 0);
            continue;
        }
        
        for (char b : code) {
            if (b == '0') {
                if (!cur->l_) cur->l_ = new Note('\0', 0);
                cur = cur->l_;
            } else {
                if (!cur->r_) cur->r_ = new Note('\0', 0);
                cur = cur->r_;
            }
        }
        cur->s_ = sym;
    }
}

void huffmanTree::writeBit(std::ofstream& out,
    unsigned char& buf,
    int& count,
    bool bit)
{
    buf = (buf << 1) | (bit ? 1 : 0);
    count++;

    if (count == 8) {
        out.put(static_cast<char>(buf));
        buf = 0;
        count = 0;
    }
}

void huffmanTree::flushBits(std::ofstream& out,
    unsigned char& buf,
    int& count)
{
    if (count > 0) {
        buf <<= (8 - count);
        out.put(static_cast<char>(buf));
        buf = 0;
        count = 0;
    }
}

void huffmanTree::encode(std::ifstream& input,
    const std::string& outputFilename)
{
    if (!input.is_open())
        throw std::runtime_error("Input not open");

    std::ofstream out(outputFilename, std::ios::binary);
    if (!out)
        throw std::runtime_error("Cannot open output");

    writeHeader(out);

    unsigned char buf = 0;
    int count = 0;

    input.seekg(0, std::ios::end);
    std::streampos fileSize = input.tellg();
    input.seekg(0);
    
    std::vector<unsigned char> buffer(fileSize);
    input.read(reinterpret_cast<char*>(buffer.data()), fileSize);

    for (unsigned char c : buffer) {
        const std::string& code = codeTable[c];
        for (char b : code)
            writeBit(out, buf, count, b == '1');
    }

    flushBits(out, buf, count);
}

void huffmanTree::decode(std::ifstream& in,
    const std::string& outputFilename) {
    if (!HEAD)
        throw std::runtime_error("Tree empty");

    std::ofstream out(outputFilename, std::ios::binary);
    if (!out)
        throw std::runtime_error("Cannot open output file");

    unsigned char buf = 0;
    int bitsLeft = 0;
    Note* cur = HEAD;
    unsigned long long bytesWritten = 0;

    while (bytesWritten < size) {
        if (bitsLeft == 0) {
            char byte;
            if (!in.get(byte))
                break;
            buf = static_cast<unsigned char>(byte);
            bitsLeft = 8;
        }

        bool bit = (buf >> (bitsLeft - 1)) & 1;
        bitsLeft--;

        cur = bit ? cur->r_ : cur->l_;
        if (!cur->l_ && !cur->r_) {
            out.put(static_cast<char>(cur->s_));
            bytesWritten++;
            cur = HEAD;
        }
    }
}