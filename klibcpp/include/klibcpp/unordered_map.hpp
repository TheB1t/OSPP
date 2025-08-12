#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/cstring.hpp>
#include <klibcpp/type_traits.hpp>
#include <klibcpp/hash.hpp>

namespace kstd {
    template<typename K, typename V>
    class unordered_map {
        public:
            struct Node {
                K key;
                V value;
                Node* next;
                uint32_t hash32;

                Node(const K& in_key, const V& in_val, Node* in_next)
                    : value(in_val), next(in_next)
                {
                    if constexpr (kstd::is_same_v<K, const char*> || kstd::is_same_v<K, char*>) {
                        if (!in_key) { 
                            key = nullptr; 
                            hash32 = 0; 
                            return;
                        }

                        const uint32_t len = strlen(in_key);
                        char* dup = new char[len + 1];
                        memcpy((uint8_t*)dup, (uint8_t*)in_key, len + 1);
                        key = dup;
                        hash32 = hash_fold32_cstr(dup);
                    } else {
                        key    = in_key;
                        hash32 = hash_fold32_trivial(in_key);
                    }
                }

                ~Node() {
                    if constexpr (kstd::is_same_v<K, const char*> || kstd::is_same_v<K, char*>)
                        delete [] const_cast<char*>(key);
                }

                static bool equals(const K& a, const K& b) {
                    if constexpr (kstd::is_same_v<K, const char*> || kstd::is_same_v<K, char*>) {
                        if (!a || !b) 
                            return a == b;

                        return strcmp(a, b) == 0;
                    } else {
                        return a == b;
                    }
                }
            };

            class iterator {
                public:
                    iterator(Node** b, size_t cap, size_t idx, Node* n)
                        : buckets(b), cap_(cap), i(idx), node(n) { advance(); }
                    iterator& operator++() { if (node) node = node->next; advance(); return *this; }
                    bool operator!=(const iterator& o) const { return node != o.node; }
                    Node& operator*()  const { return *node; }
                    Node* operator->() const { return node; }
                private:
                    Node** buckets; size_t cap_; size_t i; Node* node;
                    void advance() {
                        while (!node && (i + 1) < cap_) node = buckets[++i];
                    }
            };

            iterator begin() { return iterator(buckets_, capacity_, size_t(-1), nullptr); }
            iterator end()   { return iterator(buckets_, capacity_, capacity_, nullptr); }

            explicit unordered_map(size_t capacity = 64)
                : capacity_(capacity), size_(0)
            {
                if (capacity_ == 0) 
                    capacity_ = 1;

                buckets_ = new Node*[capacity_];
                for (size_t i = 0; i < capacity_; ++i) 
                    buckets_[i] = nullptr;
            }

            ~unordered_map() {
                clear();
                delete[] buckets_;
            }

            bool insert(const K& key, const V& value) {
                Node* head;
                uint32_t h, idx;
                compute_bucket_(key, h, idx, head);

                for (Node* n = head; n; n = n->next) {
                    if (n->hash32 == h && Node::equals(n->key, key)) {
                        n->value = value;
                        return true;
                    }
                }

                Node* nn = new Node(key, value, head);
                buckets_[idx] = nn;
                ++size_;
                return true;
            }

            bool find(const K& key, V& out_value) const {
                Node* head;
                uint32_t h, idx;
                compute_bucket_const_(key, h, idx, head);

                for (Node* n = head; n; n = n->next) {
                    if (n->hash32 == h && Node::equals(n->key, key)) {
                        out_value = n->value;
                        return true;
                    }
                }
                return false;
            }

            bool erase(const K& key) {
                Node* head;
                uint32_t h, idx;
                compute_bucket_(key, h, idx, head);

                Node* prev = nullptr;
                for (Node* n = head; n; n = n->next) {
                    if (n->hash32 == h && Node::equals(n->key, key)) {
                        if (prev) prev->next = n->next;
                        else buckets_[idx] = n->next;
                        delete n;
                        --size_;
                        return true;
                    }
                    prev = n;
                }
                return false;
            }

            void clear() {
                for (size_t i = 0; i < capacity_; ++i) {
                    Node* n = buckets_[i];
                    while (n) {
                        Node* next = n->next;
                        delete n;
                        n = next;
                    }
                    buckets_[i] = nullptr;
                }
                size_ = 0;
            }

            size_t size() const { return size_; }
            size_t capacity() const { return capacity_; }

        private:
            Node**   buckets_;
            size_t   capacity_;
            size_t   size_;

            void compute_bucket_(const K& key, uint32_t& h, uint32_t& idx, Node*& head) {
                if constexpr (kstd::is_same_v<K, const char*> || kstd::is_same_v<K, char*>) {
                    h = Node::hash_fold32_cstr(key ? key : "");
                } else {
                    h = Node::hash_fold32_trivial(key);
                }
                idx  = (capacity_ ? (h % (uint32_t)capacity_) : 0);
                head = buckets_[idx];
            }

            void compute_bucket_const_(const K& key, uint32_t& h, uint32_t& idx, Node*& head) const {
                if constexpr (kstd::is_same_v<K, const char*> || kstd::is_same_v<K, char*>) {
                    h = Node::hash_fold32_cstr(key ? key : "");
                } else {
                    h = Node::hash_fold32_trivial(key);
                }
                idx  = (capacity_ ? (h % (uint32_t)capacity_) : 0);
                head = buckets_[idx];
            }
    };
}
