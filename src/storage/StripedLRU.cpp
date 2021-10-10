#include "StripedLRU.h"
#include <iostream>

namespace Afina {
namespace Backend {

bool StripedLRU::Put(const std::string &key, const std::string &value){
    size_t shard_num = hash_func(key) % _stripe_count;
    return shards[shard_num]->Put( key, value );
}

bool StripedLRU::PutIfAbsent( const std::string &key, const std::string &value ){

    size_t shard_num = hash_func(key) % _stripe_count;
    return shards[shard_num]->PutIfAbsent( key, value );
}

bool StripedLRU::Set( const std::string &key, const std::string &value ){

    size_t shard_num = hash_func(key) % _stripe_count;
    return shards[shard_num]->Set( key, value );
}

bool StripedLRU::Delete( const std::string &key ){

    size_t shard_num = hash_func(key) % _stripe_count;
    return shards[shard_num]->Delete( key );
}

bool StripedLRU::Get( const std::string &key, std::string &value ){

    size_t shard_num = hash_func(key) % _stripe_count;
    return shards[shard_num]->Get( key, value );
}


} // namespace Backend
} // namespace Afina