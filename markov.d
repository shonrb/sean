/+
 + markov.d
 + a program to create random sentences from
 + a sample text file using markov chains
 +/
import std.file;
import std.random;
import std.stdio;
import std.string;
import std.conv;

void main(string[] argv) 
{
    if (argv.length < 3) 
    {
        writeln("Usage: " ~ argv[0] ~ "[file] [key size] [output size]");
        return;
    }
    auto filename     = argv[1];
    auto words        = (cast(string) read(filename)).split;
    int  keySize      = to!int(argv[2]);
    int  outputLength = to!int(argv[3]);

    // Construct the dictionary
    string[][string] dict;
    foreach (i; 0 .. words.length-keySize) 
    {
        // Extract the word after the current key
        auto wordIndex = i + keySize;
        auto key       = words[i .. wordIndex].join(" ");
        auto value     = words[wordIndex];
        if (key in dict)
            dict[key] ~= value;
        else 
            dict[key] = [value];
    }
    dict = dict.rehash;

    // Construct a new sentence using the dictionary.
    // Start with a random keySize word long phrase
    auto prefix = dict.keys.choice.split;
    auto output = prefix;
    foreach (i; 0 .. outputLength) 
    {   
        auto key = prefix.join(" ");
        if (key !in dict || prefix.length == 0) 
            break; 
        // Pick a random successor of the phrase
        // and add it to the output
        auto newWord = dict[key].choice;
        output ~= newWord;
        // Set the new key phrase to the phrase
        // with the first word removed and the successor 
        // added.
        prefix = prefix[1 .. $] ~ newWord;
    }
    output.join(" ").writeln;
}