#include "watcher.h"

#include <unordered_map>
#include <fstream>
#include <sstream>

FileWatcher::FileWatcher(std::string p) : path(p)
{
    std::tie(contents, hash) = read_file();
}

std::string FileWatcher::get_contents() const
{
    return contents;
}

std::tuple<std::string, std::size_t> 
FileWatcher::read_file() const
{
    static const std::hash<std::string> hasher;
    std::string file_contents;
    {
        std::ifstream file; 
        file.open(path);
        std::stringstream stream;
        stream << file.rdbuf();
        file.close();
        file_contents = stream.str();
    }
    std::size_t file_hash = hasher(file_contents);

    return std::make_tuple(file_contents, file_hash);
}
