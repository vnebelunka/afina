#include "SimpleLRU.h"
namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
    bool SimpleLRU::Put(const std::string &key, const std::string &value) {
        // If there is a key: we will delete it, because we have to put node in head
        auto it = _lru_index.find(key);
        if(it != _lru_index.end()){
            Set(key, value);
        }
        // Clearing tail until correct size
        tail_cut(key, value);
        // new node
        auto *rnode = new lru_node{key, value,{}, {}};
        std::unique_ptr<lru_node> node(rnode);
        _cur_size += value.size() + key.size();
        _lru_index.emplace( std::cref(node->key), std::ref(*node));
        if(_lru_head){
            _lru_head->prev = rnode;
            node->next = std::move(_lru_head);
            _lru_head = std::move(node);
        }
        else{ // empty cache
            _lru_head = std::move(node);
            _lru_tail = rnode;
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
        // Clearing tail until correct size
        tail_cut(key, value);
        // new node
        auto *rnode = new lru_node{key, value,{}, {}};
        std::unique_ptr<lru_node> node(rnode);
        _cur_size += value.size() + key.size();
        _lru_index.emplace( std::cref(node->key), std::ref(*node));
        if(_lru_head){
            _lru_head->prev = rnode;
            node->next = std::move(_lru_head);
            _lru_head = std::move(node);
        }
        else{
            _lru_head = std::move(node);
            _lru_tail = rnode;
        }
        return true;
    }


// See MapBasedGlobalLockImpl.h
    bool SimpleLRU::Set(const std::string &key, const std::string &value) {
        auto it = _lru_index.find(key);
        if(it == _lru_index.end()){
            return false;
        }
        lru_node *node = &it->second.get();
        shift(node);
        _cur_size -= node->value.size();
        tail_cut(key, value);
        node->value = value;
        _cur_size += value.size();
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
            _lru_head = std::move(_lru_head->next);
            if(_lru_head){
                _lru_head->prev = nullptr;
            }
            return true;
        }
        //if Tail
        if(node == _lru_tail){
            _lru_tail = _lru_tail->prev;
            _lru_tail->next.reset();
            return true;
        }
        //central node
        node->next->prev = node->prev;
        node->prev->next = std::move(node->next);
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
    shift(node);
    return true;
}

void  SimpleLRU::shift(lru_node *node) {
    if(node == _lru_head.get()){ //tail
        return;
    }
    if(_lru_tail->prev == _lru_head.get()){ //tail near head
        std::swap(_lru_head, _lru_head->next);
        _lru_head->next.reset(_lru_head->prev);
        _lru_head->prev = nullptr;
        _lru_tail = _lru_head->next.get();
        _lru_tail->prev = _lru_head.get();
        _lru_tail->next.release();
    }
    else if(node != _lru_tail){ //cental
        // Connect neighbors
        std::unique_ptr<lru_node> temp;
        std::swap(temp, node->prev->next); // free node->prev->next
        node->prev->next.reset(node->next.get()); // connect prev to next
        node->next->prev = node->prev; // connect next to prev
        // connect to head
        temp->next.release();
        temp->next.reset(_lru_head.get());
        temp.release();
        // renew head
        std::swap(_lru_head, temp);
        _lru_head.reset(node);
        // disconect head->prev, connect next
        _lru_head->prev = nullptr;
        _lru_head->next->prev = _lru_head.get();
        temp.release();
    }
    else{// Tail
        //connect head to tail:=new head. head-> tail; tail->head
        std::unique_ptr<lru_node> temp;
        std::swap(_lru_head, temp);
        _lru_head.reset(_lru_tail);
        // connect new head to other list
        std::swap(temp, _lru_tail->next);
        _lru_head->next->prev = _lru_head.get();
        _lru_tail = _lru_head->prev;
        _lru_head->prev = nullptr;
        _lru_tail->next.release();

    }
}

void SimpleLRU::tail_cut(const std::string &key,const std::string &value){
    while(_cur_size + key.size() + value.size() > _max_size && _lru_head){
        _lru_index.erase(_lru_tail->key);
        _cur_size -= _lru_tail->key.size() + _lru_tail->value.size();
        _lru_tail = _lru_tail->prev;
        _lru_tail->next.reset();
    }
}
} // namespace Backend
} // namespace Afina
