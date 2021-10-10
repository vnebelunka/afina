#ifndef AFINA_STORAGE_STRIPED_LOCK_SIMPLE_LRU_H
#define AFINA_STORAGE_STRIPED_LOCK_SIMPLE_LRU_H

#include <map>
#include <vector>
#include <string>
#include "ThreadSafeSimpleLRU.h"

#include <functional>
#include <mutex>
#include <unistd.h>
#include <afina/Storage.h>

namespace Afina {
namespace Backend {

/**
 * # SimpleLRU thread striped lock version
 *
 *
 */
class StripedLRU : public Afina::Storage {
public:
    StripedLRU( size_t max_size = 16 * 1024 * 1024, size_t st_cnt = 4) : max_size(max_size), _stripe_count(st_cnt) {
        size_t shard_size = max_size / _stripe_count;
        for(int i = 0; i < shard_size; i++) {
            shards.emplace_back(new ThreadSafeSimplLRU(shard_size));
        }
    }

    static std::shared_ptr<StripedLRU> Build_StripedLRU(size_t max_size = 16 * 1024 * 1024, size_t st_cnt = 4){
        size_t shard_size = max_size / st_cnt;
        // Max 1MB for one key, 1MB for value
        if(shard_size < 2u << 20 * sizeof(char)){
            throw std::runtime_error( "Too small shard size: " + std::to_string(shard_size) );
        }
        else{
            return std::make_shared<StripedLRU>(max_size, st_cnt);
        }
    }

    ~StripedLRU() {}

    // see SimpleLRU.h
    bool Put(const std::string &key, const std::string &value) override;

    // see SimpleLRU.h
    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // see SimpleLRU.h
    bool Set(const std::string &key, const std::string &value) override;

    // see SimpleLRU.h
    bool Delete(const std::string &key) override;

    // see SimpleLRU.h
    bool Get(const std::string &key, std::string &value) override;

private:

    // Max size for StripedLRU
    size_t max_size;
    
    // Number of stripes
    size_t _stripe_count;

    // Vector of storages
    std::vector<std::unique_ptr<ThreadSafeSimplLRU>> shards;

    // Hash functor
    std::hash<std::string> hash_func;
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_STRIPED_LOCK_SIMPLE_LRU_H