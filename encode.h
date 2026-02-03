#pragma once
#include <fstream>
#include <iostream>
#include <exception>
#include <unordered_map>
#include <queue>
#include <cstdint>

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
        if (HEAD) {
            generateCodes(HEAD, "", codes);
        }
        return codes;
    }

    void encode(std::ifstream& input, const std::string& outputFilename);
    void decode(std::ifstream& binaryInput, const std::string& outputFilename) const;

private:
    Note* HEAD = nullptr;
    std::unordered_map<char, std::string> codeTable;

    void deleteTree(Note* node);
    void generateCodes(Note* node, std::string code, 
                       std::unordered_map<char, std::string>& codes) const;
    
    std::unordered_map<char, long long> readFrequencies(std::ifstream& in);
    void writeHeader(std::ofstream& out, 
                     const std::unordered_map<char, long long>& frequencies);
    std::unordered_map<char, long long> readHeader(std::ifstream& in);
    void writeBits(std::ofstream& out, const std::string& bitString);
    std::string readBits(std::ifstream& in, uint32_t bitCount);
};

huffmanTree::huffmanTree(std::ifstream& input) {
    if (!input.is_open()) {
        throw std::runtime_error("Input file is not open");
    }
    
    auto startPos = input.tellg();
    auto frequencies = readFrequencies(input);
    input.clear();
    input.seekg(startPos);
    
    auto comp = [](Note* a, Note* b) { return a->w_ > b->w_; };
    std::priority_queue<Note*, std::vector<Note*>, decltype(comp)> heap(comp);
    
    for (const auto& [symbol, weight] : frequencies) {
        heap.push(new Note{symbol, weight, nullptr, nullptr});
    }
    
    if (heap.empty()) {
        HEAD = nullptr;
        return;
    }
    
    while (heap.size() > 1) {
        Note* left = heap.top(); heap.pop();
        Note* right = heap.top(); heap.pop();
        heap.push(new Note{'\0', left->w_ + right->w_, left, right});
    }
    
    HEAD = heap.top();
    generateCodes(HEAD, "", codeTable);
}

huffmanTree::huffmanTree(std::ifstream& binaryInput, bool fromBinary) {
    if (!fromBinary) return;
    
    if (!binaryInput.is_open()) {
        throw std::runtime_error("Binary input file is not open");
    }
    
    auto frequencies = readHeader(binaryInput);
    
    auto comp = [](Note* a, Note* b) { return a->w_ > b->w_; };
    std::priority_queue<Note*, std::vector<Note*>, decltype(comp)> heap(comp);
    
    for (const auto& [symbol, weight] : frequencies) {
        heap.push(new Note{symbol, weight, nullptr, nullptr});
    }
    
    if (heap.empty()) {
        HEAD = nullptr;
        return;
    }
    
    while (heap.size() > 1) {
        Note* left = heap.top(); heap.pop();
        Note* right = heap.top(); heap.pop();
        heap.push(new Note{'\0', left->w_ + right->w_, left, right});
    }
    
    HEAD = heap.top();
    generateCodes(HEAD, "", codeTable);
}

huffmanTree::~huffmanTree() {
    deleteTree(HEAD);
}

void huffmanTree::deleteTree(Note* node) {
    if (!node) return;
    deleteTree(node->l_);
    deleteTree(node->r_);
    delete node;
}

void huffmanTree::generateCodes(Note* node, std::string code, std::unordered_map<char, std::string>& codes) const {
    if (!node) return;
    
    if (node->s_ != '\0') {
        codes[node->s_] = code;
    }
    
    if (node->l_) {
        generateCodes(node->l_, code + "0", codes);
    }
    if (node->r_) {
        generateCodes(node->r_, code + "1", codes);
    }
}

std::unordered_map<char, long long> huffmanTree::readFrequencies(std::ifstream& in) {
    std::unordered_map<char, long long> frequencies;
    char ch;
    
    while(in.get(ch)) {
        frequencies[ch]++;
    }
    
    return frequencies;
}

void huffmanTree::writeHeader(std::ofstream& out, const std::unordered_map<char, long long>& frequencies) {
    uint32_t count = frequencies.size();
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));
    
    for (const auto& [symbol, freq] : frequencies) {
        out.put(symbol);
        out.write(reinterpret_cast<const char*>(&freq), sizeof(freq));
    }
}

std::unordered_map<char, long long> huffmanTree::readHeader(std::ifstream& in) {
    std::unordered_map<char, long long> frequencies;
    
    uint32_t count;
    in.read(reinterpret_cast<char*>(&count), sizeof(count));
    
    for (uint32_t i = 0; i < count; i++) {
        char symbol;
        long long freq;
        
        symbol = in.get();
        in.read(reinterpret_cast<char*>(&freq), sizeof(freq));
        
        frequencies[symbol] = freq;
    }
    
    return frequencies;
}

void huffmanTree::writeBits(std::ofstream& out, const std::string& bitString) {
    unsigned char buffer = 0;
    int bitCount = 0;
    
    for (char bitChar : bitString) {
        buffer = (buffer << 1) | (bitChar == '1' ? 1 : 0);
        bitCount++;
        
        if (bitCount == 8) {
            out.put(buffer);
            buffer = 0;
            bitCount = 0;
        }
    }
    
    if (bitCount > 0) {
        buffer <<= (8 - bitCount);
        out.put(buffer);
    }
}

std::string huffmanTree::readBits(std::ifstream& in, uint32_t bitCount) {
    std::string result;
    if (bitCount == 0) return result;
    
    unsigned char buffer;
    int bitsRead = 0;
    
    while (bitsRead < bitCount) {
        if (!in.get(reinterpret_cast<char&>(buffer))) {
            break;
        }
        
        for (int i = 7; i >= 0 && bitsRead < bitCount; i--) {
            bool bit = (buffer >> i) & 1;
            result += bit ? '1' : '0';
            bitsRead++;
        }
    }
    
    return result;
}

void huffmanTree::encode(std::ifstream& input, const std::string& outputFilename) {
    if (!input.is_open()) {
        throw std::runtime_error("Input file is not open");
    }
    
    auto startPos = input.tellg();
    
    std::ofstream out(outputFilename, std::ios::binary);
    if (!out.is_open()) {
        throw std::runtime_error("Cannot open output file");
    }
    
    std::unordered_map<char, long long> frequencies;
    char ch;
    input.clear();
    input.seekg(0);
    
    while(input.get(ch)) {
        frequencies[ch]++;
    }
    
    writeHeader(out, frequencies);
    
    input.clear();
    input.seekg(startPos);
    
    std::string allBits;
    while(input.get(ch)) {
        allBits += codeTable[ch];
    }
    
    uint32_t totalBits = allBits.length();
    out.write(reinterpret_cast<const char*>(&totalBits), sizeof(totalBits));
    
    writeBits(out, allBits);
    
    out.close();
}

void huffmanTree::decode(std::ifstream& binaryInput, const std::string& outputFilename) const {
    if (!binaryInput.is_open()) {
        throw std::runtime_error("Binary input file is not open");
    }
    
    if (!HEAD) {
        throw std::runtime_error("Huffman tree is empty");
    }
    
    std::ofstream out(outputFilename, std::ios::binary);
    if (!out.is_open()) {
        throw std::runtime_error("Cannot open output file");
    }
    
    uint32_t totalBits;
    binaryInput.read(reinterpret_cast<char*>(&totalBits), sizeof(totalBits));
    
    Note* current = HEAD;
    unsigned char buffer;
    int bitsProcessed = 0;
    int bitsInBuffer = 0;
    int bufferPos = 0;
    
    while (bitsProcessed < totalBits) {
        if (bitsInBuffer == 0) {
            if (!binaryInput.get(reinterpret_cast<char&>(buffer))) {
                break;
            }
            bitsInBuffer = 8;
            bufferPos = 0;
        }
        
        bool bit = (buffer >> (7 - bufferPos)) & 1;
        bufferPos++;
        bitsInBuffer--;
        bitsProcessed++;
        
        if (bit) {
            current = current->r_;
        } else {
            current = current->l_;
        }
        
        if (!current) {
            throw std::runtime_error("Invalid code in bitstream");
        }
        
        if (current->s_ != '\0') {
            out.put(current->s_);
            current = HEAD;
        }
    }
    
    if (current != HEAD) {
        throw std::runtime_error("Incomplete code at end of file");
    }
    
    out.close();
}