#ifndef DICT_HPP
#define DICT_HPP

#include "../../include/protocol/resp_parser.hpp"
#include <deque>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

using ListType = std::deque<std::string>;
using HashType = std::unordered_map<std::string, std::string>;

struct StorageObject {
    StorageDataType type;
    std::variant<std::string, ListType, HashType> value;
};

struct DictNode {
    std::string key;
    StorageObject value;
    DictNode *flink;
};

struct HashTable {
    std::vector<DictNode *> table;
    unsigned long size;
    unsigned long sizemask;
    unsigned long used;
};

class Dict {
  public:
    Dict();
    ~Dict();
    Dict(const Dict &) = delete;
    Dict &operator=(const Dict &) = delete;
    void set(const std::string &key, StorageObject value);
    StorageObject *get(const std::string &key);
    bool del(const std::string &key);

  private:
    static constexpr unsigned long INITIAL_SIZE = 4;
    static constexpr unsigned long REHASH_BATCH = 1;

    HashTable ht[2];
    long rehashIndex;

    unsigned long hash_function(const std::string &key) const;
    bool is_rehashing() const;
    DictNode *find_node(const std::string &key, HashTable &table, unsigned long index) const;
    void expand_if_needed();
    void expand(unsigned long size);
    void rehash_step();
    void dict_rehash(int n);
    void free_table(HashTable &table);
};

#endif
