#include "SimpleLRU.h"
#include <iostream>
namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
    bool SimpleLRU::Put(const std::string &key, const std::string &value) {
        // If there is a key: we will delete it, because we have to put node in head
        auto it = _lru_index.find(key);
        if(it != _lru_index.end()){
            _cur_size -= key.size() + it->second.get().value.size();
            Delete(key);
        }
        // Clearing tail until correct size
        while(_cur_size + key.size() + value.size() > _max_size && _lru_head){
            lru_node *node = _lru_tail.get();
            _lru_index.erase(node->key);
            _cur_size -= node->key.size() + node->value.size();
            _lru_tail = _lru_tail->prev;
            _lru_tail->next = nullptr;
        }
        // new node
        std::shared_ptr<lru_node> node(new lru_node{key, value,{}, {}});
        _cur_size += value.size() + key.size();
        _lru_index.emplace( std::cref(node->key), std::ref(*node));
        if(_lru_head){
            _lru_head->prev = node;
            node->next = _lru_head;
            _lru_head = node;
        }
        else{
            _lru_head = node;
            _lru_tail = node;
        }
        return true;
    }

// See MapBasedGlobalLockImpl.h
    bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
        // Key check
        auto it = _lru_index.find(key);
        if(it != _lru_index.end()){
            return false;
        }
        //Clearing the tail
        while(_cur_size + key.size() + value.size() > _max_size && _lru_head){
            lru_node *node = _lru_tail.get();
            _lru_index.erase(node->key);
            _cur_size -= node->key.size() + node->value.size();
            _lru_tail = _lru_tail->prev;
            _lru_tail->next = nullptr;
        }
        //New node
        _cur_size += value.size() + key.size();
        std::shared_ptr<lru_node> node(new lru_node{key, value,{}, {}});
        _lru_index.emplace( std::cref(node->key), std::ref(*node));
        if(_lru_head){
            _lru_head->prev = node;
            node->next = _lru_head;
            _lru_head = node;
        }
        else{
            _lru_head = node;
            _lru_tail = node;
        }
        return true;
    }


// See MapBasedGlobalLockImpl.h
    bool SimpleLRU::Set(const std::string &key, const std::string &value) {
        auto it = _lru_index.find(key);
        if(it == _lru_index.end()){
            return false;
        }
        // Put in the head with Delete + Put
        Delete(key);
        Put(key, value);
        return true;
    }

// See MapBasedGlobalLockImpl.h
    bool SimpleLRU::Delete(const std::string &key) {
        auto it = _lru_index.find(key);
        if(it == _lru_index.end()){
            return false;
        }
        lru_node *node = &it->second.get();
        _cur_size -= node->value.size() + node->key.size();
        _lru_index.erase(key);
        //if Head
        if(node == _lru_head.get()){
            _lru_head = _lru_head->next;
            if(_lru_head){
                _lru_head->prev = nullptr;
            }
            return true;
        }
        //if Tail
        if(node == _lru_tail.get()){
            _lru_tail = _lru_tail->prev;
            _lru_tail->next = nullptr;
            return true;
        }
        //central node
        node->prev->next = node->next;
        node->next->prev = node->prev;
        return true;
    }

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) {
    auto it = _lru_index.find(key);
    if(it == _lru_index.end()){
        return false;
    }
    lru_node* node = &it->second.get();
    value = node->value;
    // Place in the head with Delete + Put
     Delete(key);
    Put(key, value);
    //shift(node);
    return true;
}

//TODO не работает :(
void SimpleLRU::shift(lru_node *node){
    if(!node->prev){
        return;
    }
    if(node->prev == _lru_head){
        _lru_head.swap(_lru_tail);
    }
}

} // namespace Backend
} // namespace Afina
