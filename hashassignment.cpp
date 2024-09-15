/* hashassignment.cpp
 * An implementation of a specific 
 * hash set data structure for an assignment.
 */
#include <array>
#include <string>
#include <vector>
#include <iostream>
#include <optional>

class SpecificSet {
public:
    template<typename ...T>
    void process_input(T&&... args);

private:
    int                hash_key   (const std::string&) const;
    std::optional<int> search     (const std::string&) const;
    void               insert_key (const std::string&);
    void               delete_key (const std::string&);

    enum SlotStatus {
        NEVER_USED, 
        TOMBSTONE, 
        OCCUPIED
    };

    struct Slot {
        SlotStatus  status = NEVER_USED;
        std::string key    = "";
    };

    static constexpr int NUM_SLOTS = 26;

    std::array<Slot, NUM_SLOTS> slots;
};

int SpecificSet::hash_key(const std::string& key) const {
    char last = std::tolower(key.back());

    // Get the index of the character in the alphabet
    int index = last - 'a';
    return index;
}

std::optional<int> 
SpecificSet::search(const std::string& key) const {
    int index = hash_key(key);
    std::optional<int> ret;

    // No match if the hash hasn't been used
    if (slots[index].status == NEVER_USED) {
        return ret;
    }

    // Loop over the rest of the slots until
    // a match is found.
    for (int _ = 0; _ < NUM_SLOTS; ++_) {
        const Slot& slot = slots[index];

        if (slot.status == OCCUPIED && slot.key == key) {
            ret = index;
            break;
        }
        
        // Wrap around when reaching the end of the slots array.
        index = (index + 1) % NUM_SLOTS;
    }
    
    return ret;
}

void SpecificSet::insert_key(const std::string& key) {
    constexpr int MAX_KEY_SIZE = 10;

    // Search for the key.
    auto res   = search(key);
    bool found = res.has_value();

    // Do nothing if the key is already in the array.
    if (found || key.size() > MAX_KEY_SIZE) {
        return;
    }

    int index = hash_key(key);

    // Find the first unoccupied slot to store the key
    for (int _ = 0; _ < NUM_SLOTS; ++_) {
        Slot& slot = slots[index];

        if (slot.status == NEVER_USED || slot.status == TOMBSTONE) {
            slot.status = OCCUPIED;
            slot.key = key;
            return;
        }

        index = (index + 1) % NUM_SLOTS;
    }
}

void SpecificSet::delete_key(const std::string& key) {
    auto res   = search(key);
    bool found = res.has_value();

    if (found) {
        int index = res.value();
        Slot& slot = slots[index];
        slot.status = TOMBSTONE;
    }
}

template<typename ...T>
void SpecificSet::process_input(T&&... args) {
    std::vector<std::string> inputs({ args... });

    for (const auto& input : inputs) {
        // Take the first char as the operation, and the rest as the key
        char first = input[0];
        auto key = input.substr(1);

        if      (first == 'A') insert_key(key);
        else if (first == 'D') delete_key(key);
    }

    for (const auto& slot : slots) {
        if (slot.status == OCCUPIED) {
            std::cout << slot.key << "\n";
        }
    }
}

int main() {
    SpecificSet s;

    s.process_input(
        "Aapple", 
        "Aorange", 
        "Dapple", 
        "Astrawberry"
    );
}