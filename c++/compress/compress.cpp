/* compress.cpp
 * A file compresser/decompresser
 */
#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <map>
#include <string>

//
// Contains a value that can either be read as
// a 4-byte integer or 4 seperate bytes
//
union IntBytesPair {
    int as_int;
    std::array<char, 4> as_bytes;
};

//
// A node in a huffman tree
//
struct Node {
    unsigned total;
    std::string chars;
    Node *next_zero;
    Node *next_one;

    Node(char c, unsigned t) 
    : chars(&c), total(t) {}

    Node(Node *z, Node *o) 
    : next_zero(z), next_one(o) 
    {
        total = z->total + o->total;
        chars = z->chars + o->chars;
    }

    bool is_inode() const
    {
        return chars.size() != 1;
    }
};

//
// Takes a table of characters to their frequencies and builds a
// huffman tree.
//
Node *build_tree(const std::map<char, unsigned>& char_frequencies)
{
    std::vector<Node*> nodes;
    
    // Create a node for each character in the string
    for (auto pair : char_frequencies) {
        auto [chr, total] = pair;
        Node *n = new Node(chr, total);
        nodes.push_back(n);
    }

    // Helper function to get and pop in one call
    auto pop_from_nodes = [&]() {
        auto r = nodes[0];
        nodes.erase(nodes.begin());
        return r;
    };

    // Repeatedly merge the two nodes with the lowest counts,
    // until only one node remains in the vec.
    while (nodes.size() > 1) {
        std::sort(nodes.begin(), nodes.end(), [](Node *a, Node *b) {
            return a->total < b->total;
        });
        Node *a = pop_from_nodes();
        Node *b = pop_from_nodes();

        Node *n = new Node(a, b);
        nodes.push_back(n);
    }

    return nodes[0];
}

//
// Encodes the path through a huffman tree to get to each of its leaves as
// strings of '0's and '1's.
//
std::map<char, std::string> get_paths(const Node *node, std::string prefix = "") 
{
    std::map<char, std::string> ret;

    if (node->is_inode()) {
        auto zero = get_paths(node->next_zero, prefix + "0");
        auto one  = get_paths(node->next_one,  prefix + "1");
        zero.merge(one);
        ret = zero;
    } else {
        char c = node->chars[0];
        ret = {{ c, prefix }};
    }
    return ret;
}

//
// Joins two vectors together.
//
template<typename T>
void concat_vectors(std::vector<T>& dest, std::vector<T>&& src)
{
    dest.insert(
        dest.end(),
        std::make_move_iterator(src.begin()),
        std::make_move_iterator(src.end())
    );
}

//
// Takes the contents of a file (buffer) and replaces it with a compressed
// version.
//
void compress(std::vector<char>& buffer)
{   
    // First we need to count the occurences of each character in the buffer
    std::map<char, unsigned> char_frequencies;

    // Count the occurences of each char in the string
    for (auto c : buffer) {
        if (char_frequencies.contains(c)) {
            auto count = char_frequencies[c];
            char_frequencies.insert_or_assign(c, count+1);
            continue;
        } 
        char_frequencies.insert({c, 1});
    }

    // Next we construct a huffman tree of characters
    Node *root = build_tree(char_frequencies);

    // Next we get the paths that must be taken through the tree to get to each
    // character as a string of '0' and '1's.
    auto paths = get_paths(root);

    // Create a new version of the buffer with 
    // characters encoded as their path in binary.
    std::vector<char> encoded_bytes;
    unsigned char cur        = 0; // Current byte being worked on
    unsigned      cur_length = 0; // Number of bits pushed to the current byte.

    for (auto c : buffer) {
        auto pattern = paths[c];

        for (char p : pattern) {
            int bit = p == '1';
            
            cur = (cur << 1) | bit;
            ++cur_length;

            if (cur_length == 8) {
                encoded_bytes.push_back(cur);
                cur = 0;
                cur_length = 0;
            }
        }   
    }

    // If we reached the end of the buffer before the current byte was 
    // finished, shift it into place and append it. This means cur_length
    // also doubles as the number of bits to read from the last byte when decoding.
    if (cur_length > 0) {
        cur <<= 8 - cur_length;
        encoded_bytes.push_back(cur);
    }

    // Now we need to serialise the character frequency table, so the
    // tree can be reconstructed later for decompression
    std::vector<char> freq_bytes;

    for (auto pair : char_frequencies) {
        // Bytes 1-3 contain the total, byte 4 contains the character
        auto [chr, total] = pair;

        // Pack the values in an int and read it back as a char array.
        IntBytesPair val;
        val.as_int = (total & 0xfff) | (chr << 24);

        for (auto c : val.as_bytes)
            freq_bytes.push_back(c);
    }

    // Build the new file
    buffer.clear();

    buffer.push_back((unsigned char) cur_length);              // Bits to read from the last encoded bytes
    buffer.push_back((unsigned char) char_frequencies.size()); // Number of characters in the frequency map
    concat_vectors(buffer, std::move(freq_bytes));             // Serialised frequency table
    concat_vectors(buffer, std::move(encoded_bytes));          // Encoded contents of the file
}

//
// Takes the contents of an already compressed file (buffer) 
// and replaces it with it's decompressed version.
//
void decompress(std::vector<char>& buffer)
{
    unsigned bits_in_last_byte = buffer[0];
    unsigned freq_count = buffer[1];

    // Get the location of the frequency table in the buffer
    unsigned freq_start = 2;
    unsigned freq_end = freq_count * 4 + freq_start;

    // Extract the character frequencies back
    std::map<char, unsigned> char_frequencies;

    for (unsigned i = freq_start; i < freq_end; i += 4) {
        char chr = buffer[i+3];

        IntBytesPair val;
        val.as_bytes = { 
            buffer[i],   buffer[i+1], 
            buffer[i+2], 0 
        };
        unsigned count = val.as_int;

        char_frequencies.insert({chr, count});
    }

    // Reconstruct the tree from the frequency table
    Node *root = build_tree(char_frequencies);

    // Everything following the frequency table is encoded data
    std::vector<char> output;
    auto current = root;

    auto deconstruct_byte = [](unsigned char x, unsigned bit_count) {
        std::vector<int> bits;
        for (int i = 0; i < bit_count; ++i) 
            bits.push_back((x >> 7 - i) & 1);
        return bits;
    };

    // For each bit in the data, walk through the tree. When a character
    // is found, append it to the output and start again from the root.
    for (int i = freq_end; i < buffer.size(); ++i) {
        bool last_byte = i == buffer.size()-1;

        // Only read what is required from the last byte
        unsigned bit_count = last_byte ? bits_in_last_byte+1 : 8;

        for (auto bit : deconstruct_byte(buffer[i], bit_count)) {
            if (bit) current = current->next_one;
            else     current = current->next_zero;

            if (!current->is_inode()) {
                output.push_back(current->chars[0]);
                current = root;
            }
        }
    }

    buffer.clear();
    concat_vectors(buffer, std::move(output));
}

int main(int argc, const char **argv) 
{
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " [text file / .huff file]\n";
        return 1;
    }

    std::string filename(argv[1]);


    std::ifstream input(filename);
    std::string new_filename;

    std::vector<char> contents( 
        (std::istreambuf_iterator<char>(input) ),
        (std::istreambuf_iterator<char>()      ));
    unsigned orig_size = contents.size();

    std::string action;
  
    // If the file has an extension, check if it is .huff
    int pos = filename.rfind('.');

    if (pos >= 0 && filename.substr(pos) == ".huff") {
        decompress(contents);
        new_filename = filename.substr(0, pos); // Remove the .huff
        action = "decompressed";
    } else {
        compress(contents);
        new_filename = filename + ".huff";
        action = "compressed";
    }    

    std::ofstream output;
    output.open(new_filename);
    for (auto b : contents) output << b;

    std::cout << filename   << " "          << action          << " from " 
              << orig_size  << " bytes to " << contents.size() << " bytes.\n";
}
