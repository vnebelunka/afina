#include "StripedLRU.h"
#include <iostream>

namespace Afina {
namespace Backend {

bool StripedLRU::Put(const std::string &key, const std::string &value){
    return get_shard(key)->Put( key, value );
}

bool StripedLRU::PutIfAbsent( const std::string &key, const std::string &value ){

    return get_shard(key)->PutIfAbsent( key, value );
}

bool StripedLRU::Set( const std::string &key, const std::string &value ){

    return get_shard(key)->Set( key, value );
}

bool StripedLRU::Delete( const std::string &key ){

    return get_shard(key)->Delete( key );
}

bool StripedLRU::Get( const std::string &key, std::string &value ){
    return get_shard(key)->Get(key, value);
}

std::shared_ptr<ThreadSafeSimplLRU> StripedLRU::get_shard(const std::string &key) {
    size_t shard_num = hash_func(key) % _stripe_count;
    return shards[shard_num];
}


} // namespace Backend
} // namespace Afina