#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>

using namespace std;

const char symbolChars[4] = {'A', 'T', 'G', 'C'};

struct Nucleotide {
    char symbol;
    size_t position;
    int chromosome_num;
    int gene_num;
    bool is_marked;
    char service_info;
};

struct CompactNucleotide {
    uint32_t position;
    uint32_t chromosome_num : 6;
    uint32_t gene_num : 15;
    uint32_t symbol : 2;
    uint32_t service_info : 8;
    uint32_t is_marked : 1;
};

CompactNucleotide Compress(const Nucleotide& n) {
    CompactNucleotide result;
    result.position = n.position;
    result.chromosome_num = n.chromosome_num;
    result.gene_num = n.gene_num;
    switch (n.symbol) {
    case 'A':
        result.symbol = 0;
        break;
    case 'T':
        result.symbol = 1;
        break;
    case 'G':
        result.symbol = 2;
        break;
    case 'C':
        result.symbol = 3;
        break;
    default:
        result.symbol = 0;
        break;
    }
    result.service_info = n.service_info;
    result.is_marked = n.is_marked;
    return result;
}

Nucleotide Decompress(const CompactNucleotide& cn) {
    Nucleotide result;
    result.position = cn.position;
    result.chromosome_num = cn.chromosome_num;
    result.gene_num = cn.gene_num;
    result.symbol = symbolChars[cn.symbol];
    result.service_info = cn.service_info;
    result.is_marked = cn.is_marked;
    return result;
}

static_assert(sizeof(CompactNucleotide) <= 8, "Your CompactNucleotide is not compact enough"s);
static_assert(alignof(CompactNucleotide) == 4, "Don't use '#pragma pack'!"s);
bool operator==(const Nucleotide& lhs, const Nucleotide& rhs) {
    return (lhs.symbol == rhs.symbol) && (lhs.position == rhs.position) && (lhs.chromosome_num == rhs.chromosome_num) &&
           (lhs.gene_num == rhs.gene_num) && (lhs.is_marked == rhs.is_marked) && (lhs.service_info == rhs.service_info);
}
void TestSize() {
    assert(sizeof(CompactNucleotide) <= 8);
}
void TestCompressDecompress() {
    Nucleotide source;
    source.symbol = 'T';
    source.position = 1'000'000'000;
    source.chromosome_num = 48;
    source.gene_num = 1'000;
    source.is_marked = true;
    source.service_info = '!';

    CompactNucleotide compressedSource = Compress(source);
    Nucleotide decompressedSource = Decompress(compressedSource);

    assert(source == decompressedSource);
}

int main() {
    TestSize();
    TestCompressDecompress();
}