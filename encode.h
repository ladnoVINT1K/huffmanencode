#pragma once
#include <fstream>
#include <iostream>
#include <exception>
#include <unordered_map>
#include <queue>
#include <cstdint>
#include <algorithm>

struct Note {
    char s_;
    long long w_ = 0;
    Note* l_ = nullptr;
    Note* r_ = nullptr;
};

class huffmanTree {
public:
    explicit huffmanTree(std::ifstream& input);
    huffmanTree(std::ifstream& binaryInput, bool fromBinary);
    ~huffmanTree();

    std::unordered_map<char, std::string> getCodeTable() const {
        std::unordered_map<char, std::string> codes;
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
    std::unordered_map<char, std::string> codeTable;

    void deleteTree(Note* node);
    void generateCodes(Note* node, std::string code, std::unordered_map<char, std::string>& codes) const;

    std::unordered_map<char, long long> readFrequencies(std::ifstream& in);
    long long size;
    void buildTreeFromCodes();


    void writeBit(std::ofstream& out, unsigned char& buf, int& count, bool bit);
    void flushBits(std::ofstream& out, unsigned char& buf, int& count);
};

huffmanTree::huffmanTree(std::ifstream& input) {
    if (!input.is_open())
        throw std::runtime_error("Input file not open");
    size = 0;
    auto freq = readFrequencies(input);
    input.clear();
    input.seekg(0);

    auto cmp = [](Note* a, Note* b) { return a->w_ > b->w_; };
    std::priority_queue<Note*, std::vector<Note*>, decltype(cmp)> pq(cmp);

    for (auto& [s, w] : freq)
        pq.push(new Note{s, w});

    if (pq.empty())
        return;

    while (pq.size() > 1) {
        Note* l = pq.top(); pq.pop();
        Note* r = pq.top(); pq.pop();
        pq.push(new Note{'\0', l->w_ + r->w_, l, r});
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

void huffmanTree::generateCodes(Note* node, std::string code,
    std::unordered_map<char, std::string>& codes) const {

    if (!node) return;

    if (node->s_ != '\0')
        codes[node->s_] = code.empty() ? "0" : code;

    generateCodes(node->l_, code + "0", codes);
    generateCodes(node->r_, code + "1", codes);
}

std::unordered_map<char, long long> huffmanTree::readFrequencies(std::ifstream& in) {
    std::unordered_map<char, long long> f;
    char c;
    while (in.get(c)) {
        size++;
        f[c]++;
    }
    return f;
}

void huffmanTree::writeHeader(std::ofstream& out) {
    uint32_t count = codeTable.size();
    out.write((char*)&count, sizeof(count));
    out.write((char*)&size, sizeof(size));
    for (auto& [sym, code] : codeTable) {

        uint64_t packed = 1; // leading 1
        for (char b : code)
            packed = (packed << 1) | (b == '1');

        out.put(sym);
        out.write((char*)&packed, sizeof(packed));
    }
}

void huffmanTree::readHeader(std::ifstream& in) {
    uint32_t count;
    in.read((char*)&count, sizeof(count));
    in.read((char*)&size, sizeof(size));
    codeTable.clear();

    for (uint32_t i = 0; i < count; ++i) {
        char sym;
        uint64_t packed;

        in.get(sym);
        in.read((char*)&packed, sizeof(packed));

        std::string code;

        while (packed > 1) {
            code.push_back((packed & 1) ? '1' : '0');
            packed >>= 1;
        }
        std::reverse(code.begin(), code.end());
        codeTable[sym] = code;
    }
}

void huffmanTree::buildTreeFromCodes() {
    HEAD = new Note{'\0',0};

    for (auto& [sym, code] : codeTable) {
        Note* cur = HEAD;
        for (char b : code) {
            if (b == '0') {
                if (!cur->l_) cur->l_ = new Note{'\0',0};
                cur = cur->l_;
            }
            else {
                if (!cur->r_) cur->r_ = new Note{'\0',0};
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
    buf = (buf << 1) | bit;
    count++;

    if (count == 8) {
        out.put(buf);
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
        out.put(buf);
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

    char c;
    input.clear();
    input.seekg(0);

    while (input.get(c)) {
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

    unsigned char buf = 0;
    int bitsLeft = 0;
    Note* cur = HEAD;

    while (size != 0) {
        size--;
        if (bitsLeft == 0) {
            if (!in.get((char&)buf))
                break;
            bitsLeft = 8;
        }

        bool bit = (buf >> (bitsLeft - 1)) & 1;
        bitsLeft--;

        cur = bit ? cur->r_ : cur->l_;

        if (cur->s_ != '\0') {
            out.put(cur->s_);
            cur = HEAD;
        }
    }
}
