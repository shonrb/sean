#ifndef WATCHER_H_
#define WATCHER_H_

#include <string>
#include <tuple>

class FileWatcher {
    std::string path;
    std::string contents;
    std::size_t hash;

public:
    FileWatcher(std::string p);
    void check_for_change(auto&& on_change);
    std::string get_contents() const;

private:
    std::tuple<std::string, std::size_t> read_file() const;
};

void FileWatcher::check_for_change(auto&& on_change)
{
    auto [newcontents, newhash] = read_file();
    if (newhash != hash) {
        on_change(newcontents);
        hash = newhash;
        contents = newcontents;
    }
}

#endif