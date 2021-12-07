/* hlight.cpp
 * A c++ syntax highlighter.
 */
#include <fstream>
#include <iostream>
#include <algorithm>
#include <map>
#include <vector>
#include <string>
#include <regex>

enum class HighlightType
{
    HIGHLIGHT_BEGIN,
    NONE,
    TYPENAME,
    KEYWORD,
    OPERATOR,
    COMMENT,
    STRING_LIT,
    NUM_LIT,
    OTHER
};

struct SyntaxRule
{
    HighlightType highlight;
    std::regex pattern;

    SyntaxRule(HighlightType h, std::string p)
    : highlight(h), pattern(std::regex(p)) 
    {}
};


static std::map<HighlightType, std::string> colour_table = {
    {HighlightType::NONE,       "\033[0m"},
    {HighlightType::TYPENAME,   "\033[91m"},
    {HighlightType::KEYWORD,    "\033[91m"},
    {HighlightType::OPERATOR,   "\033[91m"},
    {HighlightType::COMMENT,    "\033[90m"},
    {HighlightType::STRING_LIT, "\033[92m"},
    {HighlightType::NUM_LIT,    "\033[96m"},
    {HighlightType::OTHER,      "\033[92m"}
};

static std::vector<SyntaxRule> cpp_syntax{
    SyntaxRule(HighlightType::KEYWORD,  
               "\\b(if|for|const|static|while|do"
                  "|switch|case|struct|class|enum)\\b"),
    SyntaxRule(HighlightType::TYPENAME, 
               "\\b(void|char|unsigned|int|float|double|long|short|auto)\\b"),
    SyntaxRule(HighlightType::STRING_LIT,
               "([\"\'])(?:(?=(\\\\?))\\2.)*?\\1"),
    SyntaxRule(HighlightType::COMMENT,
               "\\/\\/.*(?=\\n|$)"),
    SyntaxRule(HighlightType::COMMENT,
               "\\/\\*.*\\*\\/"),
    SyntaxRule(HighlightType::NUM_LIT,
               "\\b\\d+(\\.\\d*)*[a-zA-Z]*\\b"),
    SyntaxRule(HighlightType::OTHER,
               "#.*(?=\\n|$)"),
    SyntaxRule(HighlightType::OPERATOR,
               "\\+|\\-|\\*|\\/|=|\\?|%|\\^|<|>|!|~|\\||&"),
};


static void apply_rule(const SyntaxRule&           rule,
                       const std::string&          string_buffer,
                       std::vector<HighlightType>& highlight_buffer)
{
    auto iter = std::sregex_iterator(
        string_buffer.begin(), 
        string_buffer.end(), rule.pattern);

    // For each match, write the rule's highlight to the 
    // buffer between the start and end of the match.
    for (; iter != std::sregex_iterator(); ++iter) {
        int start = iter->position();
        int end   = start + iter->length();

        // Only highlight if the region hasn't been already.
        // This way the first rules applied have higher precedence.
        if (highlight_buffer[start] == HighlightType::NONE)
            std::fill(
                highlight_buffer.begin() + start,
                highlight_buffer.begin() + end, rule.highlight);
    }
}

void highlight_buffer(std::string& string_buffer, 
                      const std::vector<SyntaxRule>& rules)
{
    unsigned len = string_buffer.size();
    std::vector<HighlightType> highlight_buffer(len);
    std::fill(
        highlight_buffer.begin(), 
        highlight_buffer.end(), HighlightType::NONE);

    // Apply each of the rules to the highlight buffer
    for (auto& rule : rules)
        apply_rule(rule, string_buffer, highlight_buffer);
    
    // Step through the highlight buffer, inserting
    // escape codes in the string buffer where it changes
    auto current_hlight = HighlightType::HIGHLIGHT_BEGIN;
    int added_length = 0;

    for (unsigned i = 0; i < len; ++i) {
        auto& hlight = highlight_buffer[i];

        if (hlight != current_hlight) {
            auto code = colour_table[hlight];
            string_buffer.insert(i + added_length, code);
            // Update added_length to compensate for the inserted escape code
            added_length += code.size();
            current_hlight = hlight;
        }
    }
}

static void abort(const char* why)
{
    std::cerr << "Error: " << why << "\n";
    std::exit(1);
}

int main(int argc, char** argv)
{
    if (argc == 1)
        abort("An input file is required");

    std::ifstream file(argv[1]);
    if (!file.good())
        abort("No such file exists");

    std::string contents( 
        (std::istreambuf_iterator<char>(file)),
        (std::istreambuf_iterator<char>(    )));

    highlight_buffer(contents, cpp_syntax);

    std::cout << contents << "\n";
}