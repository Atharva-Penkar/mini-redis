#include "../../include/storage/dict.hpp"
#include <functional>

namespace {
unsigned long next_power_of_two(unsigned long size) {
    unsigned long i = 4;
    while (i < size)
        i <<= 1;
    return i;
}
} // namespace

Dict::Dict() : rehashIndex(-1) {
    ht[0] = HashTable{std::vector<DictNode *>(INITIAL_SIZE, nullptr), INITIAL_SIZE, INITIAL_SIZE - 1, 0};
    ht[1] = HashTable{std::vector<DictNode *>(), 0, 0, 0};
}

Dict::~Dict() {
    free_table(ht[0]);
    free_table(ht[1]);
}

void Dict::free_table(HashTable &h) {
    for (unsigned long i = 0; i < h.size; i++) {
        DictNode *h_element = h.table[i];
        while (h_element) {
            DictNode *next_h_element = h_element->flink;
            delete h_element;
            h_element = next_h_element;
        }
    }
    h.table.clear();
    h.size = 0;
    h.sizemask = 0;
    h.used = 0;
}

unsigned long Dict::hash_function(const std::string &key) const { return std::hash<std::string>{}(key); }

bool Dict::is_rehashing() const { return rehashIndex != -1; }

DictNode *Dict::find_node(const std::string &key, HashTable &table, unsigned long index) const {
    DictNode *h_element = table.table[index];
    while (h_element) {
        if (h_element->key == key)
            return h_element;
        h_element = h_element->flink;
    }
    return nullptr;
}

void Dict::rehash_step() {
    if (is_rehashing())
        dict_rehash(REHASH_BATCH);
}

void Dict::dict_rehash(int n) {
    if (!is_rehashing())
        return;
    int empty_visits = n * 10;
    while (n > 0 && ht[0].used != 0) {
        while (ht[0].table[rehashIndex] == nullptr) {
            rehashIndex++;
            if (--empty_visits == 0)
                return;
        }
        DictNode *h_element = ht[0].table[rehashIndex];
        while (h_element) {
            DictNode *next_h_element = h_element->flink;
            unsigned long index = hash_function(h_element->key) & ht[1].sizemask;
            h_element->flink = ht[1].table[index];
            ht[1].table[index] = h_element;
            ht[0].used--;
            ht[1].used++;
            h_element = next_h_element;
        }
        ht[0].table[rehashIndex] = nullptr;
        rehashIndex++;
        n--;
    }
    if (ht[0].used == 0) {
        ht[0] = ht[1];
        ht[1] = HashTable{std::vector<DictNode *>(), 0, 0, 0};
        rehashIndex = -1;
    }
}

void Dict::expand(unsigned long size) {
    if (is_rehashing() || ht[0].used > size)
        return;
    unsigned long real_size = next_power_of_two(size);
    ht[1] = HashTable{std::vector<DictNode *>(real_size, nullptr), real_size, real_size - 1, 0};
    rehashIndex = 0;
}

void Dict::expand_if_needed() {
    if (is_rehashing())
        return;
    if (ht[0].used >= ht[0].size)
        expand(ht[0].used * 2);
}

void Dict::set(const std::string &key, StorageObject value) {
    if (is_rehashing())
        rehash_step();
    unsigned long index0 = hash_function(key) & ht[0].sizemask;
    DictNode *existing = find_node(key, ht[0], index0);
    if (existing) {
        existing->value = std::move(value);
        return;
    }
    if (is_rehashing()) {
        unsigned long index1 = hash_function(key) & ht[1].sizemask;
        DictNode *existing1 = find_node(key, ht[1], index1);
        if (existing1) {
            existing1->value = std::move(value);
            return;
        }
    }

    expand_if_needed();

    HashTable &target = is_rehashing() ? ht[1] : ht[0];
    unsigned long index = hash_function(key) & target.sizemask;
    DictNode *node = new DictNode{key, std::move(value), target.table[index]};
    target.table[index] = node;
    target.used++;
}

StorageObject *Dict::get(const std::string &key) {
    if (is_rehashing())
        rehash_step();
    unsigned long index0 = hash_function(key) & ht[0].sizemask;
    DictNode *h_element = find_node(key, ht[0], index0);
    if (h_element)
        return &h_element->value;
    if (is_rehashing()) {
        unsigned long index1 = hash_function(key) & ht[1].sizemask;
        h_element = find_node(key, ht[1], index1);
        if (h_element)
            return &h_element->value;
    }
    return nullptr;
}

bool Dict::del(const std::string &key) {
    if (is_rehashing())
        rehash_step();
    unsigned long index0 = hash_function(key) & ht[0].sizemask;
    DictNode *h_element = ht[0].table[index0];
    DictNode *previous = nullptr;
    while (h_element) {
        if (h_element->key == key) {
            if (previous)
                previous->flink = h_element->flink;
            else
                ht[0].table[index0] = h_element->flink;
            delete h_element;
            ht[0].used--;
            return true;
        }
        previous = h_element;
        h_element = h_element->flink;
    }
    if (is_rehashing()) {
        unsigned long index1 = hash_function(key) & ht[1].sizemask;
        h_element = ht[1].table[index1];
        previous = nullptr;
        while (h_element) {
            if (h_element->key == key) {
                if (previous)
                    previous->flink = h_element->flink;
                else
                    ht[1].table[index1] = h_element->flink;
                delete h_element;
                ht[1].used--;
                return true;
            }
            previous = h_element;
            h_element = h_element->flink;
        }
    }
    return false;
}
