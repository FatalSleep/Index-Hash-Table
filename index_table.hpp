#ifndef INDEX_TABLE
#define INDEX_TABLE
/*
    This library provides an index lookup table without wasting memory space.
    So what we do is create a hash table with a perfect hash, where the input index is the has.
    This results in a zero-collision perfect hash table.

    Each bucket will be numbered according to it's index range. For example if a bucket
    size is 8 indices, and we have index 169, then the bucket index is floor(169/8).

    NOTE: Whenever an item of a bucket is inserted or removed from a bucket it will NOT
    be deallocated, but returned through the removal functions.
*/
#include <vector>
#include <stack>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <functional>
#include <iostream>

template<typename T, size_t S>
class index_bucket {
    public:
    T items[S];
    size_t filled;
    int32_t bucket_index = -1;
    
    // Creates a new index_bucket and fills all of it's items with T().
    index_bucket(size_t bucket_index) {
        this->bucket_index = bucket_index;
        std::fill(items, items + S, T());
        filled = 0;
    }

    // Gets the first freely available index in the bucket, else null.
    int32_t get_index() {
        int32_t index = std::distance(items, std::find_if(std::begin(items), std::end(items), [](T itm) { return (itm == T()); }));
        return (index != S)? index : -1;
    }

    // Gest the idnex of the item if it exists, else null.
    int32_t item(T item) {
        int32_t index = std::distance(items, std::find_if(std::begin(items), std::end(items), [item](T itm) { return itm == item; }));
        return (index != S) ? index : -1;
    }
    
    // Inserts a new item and returns it's index, else -1.
    int32_t insert(T item) {
        int32_t index = get_index();
        if (index >= 0 && index < S) {
            items[index] = item;
            filled++;
        }
        return index + (bucket_index * S);
    }

    // Returns the index of the item that was removed, else -1.
    int32_t remove(T item) {
        int32_t index = std::distance(items, std::find_if(std::begin(items), std::end(items), [&item](T src) { return (src == item); }));
        if (index >= 0) {
            items[index] = T();
            filled--;
        }
        return (index != S) ? index + (bucket_index * S) : -1;
    }
};

template<typename T, size_t S>
class index_table {
    public:
    std::vector<index_bucket<T, S>*> buckets;
    private:
    std::stack<int32_t> empty;
    
    // Creates a new bucket with the next freely available range of indices on the stack.
    index_bucket<T, S>* bucket() {
        int32_t bindex = -1;
        index_bucket<T,S>* bckt;

        if (empty.size() > 0) {
            bindex = empty.top();
            empty.pop();
        } else {
            if (buckets.size() > 0) {
                bindex = std::find_if(buckets.begin(), buckets.end(), [&bindex](index_bucket<T, S>* b) {
                    if (b->bucket_index > bindex) {
                        bindex = b->bucket_index;
                        return true;
                    }
                    
                    return false;
                }) - buckets.begin() + 1;
            } else {
                bindex = 0;
            }
        }

        bckt = new index_bucket<T, S>(bindex);
        buckets.push_back(bckt);
        return bckt;
    }

    // Returns the first bucket with a free index.
    index_bucket<T, S>* first() {
        auto iter = std::find_if(buckets.begin(), buckets.end(), [](index_bucket<T, S>* bckt) { return bckt->filled < S; });
        if (iter == buckets.end())
            return nullptr;
        return buckets[iter - buckets.begin()];
    }

    public:
    // Create a table with a number of pre-existing buckets as "cache."
    index_table(int32_t cache) {
        for(int32_t i = 0; i < cache; i++)
            bucket();
    }

    // Returns the number of allocated items in all buckets.
    size_t count() { return std::accumulate(buckets.begin(), buckets.end(), 0, [](size_t val, index_bucket<T, S>* bckt) { return val + bckt->filled; }); }

    // Returns the static bucket size.
    size_t sizeb() { return S; }

    // Returns the size the items <T>.
    size_t sizei() { return sizeof(T); }

    // Inserts a new item into the index table.
    int32_t insert(T item) {
        index_bucket<T, S>* bckt = first();

        if (bckt == nullptr)
            bckt = bucket();

        return bckt->insert(item);
    }

    // Removes the specified item from the index table.
    int32_t removet(T item) {
        auto iter = std::find_if(buckets.begin(), buckets.end(), [&item](index_bucket<T, S>* bckt) { return bckt->item(item) != -1; });
        if (iter == buckets.end())
            return -1;
        index_bucket<T, S>* bckt = buckets[iter - buckets.begin()];

        if (bckt != nullptr) {
            int32_t index = bckt->remove(item);

            if (bckt->filled <= 0) {
                buckets.erase(iter);
                delete bckt;
            }

            return index;
        }

        return -1;
    }

    // Removes the item at the specified index from the index table.
    T removei(int32_t index) {
        auto iter = std::find_if(buckets.begin(), buckets.end(), [&index](index_bucket<T, S>* bck) { return bck->items[index % S] != T(); });
        if (iter == buckets.end())
            return T();
        index_bucket<T, S>* bckt = buckets[iter - buckets.begin()];

        if (bckt != T()) {
            T item = bckt->items[index % S];
            bckt->remove(item);

            if (bckt->filled <= 0) {
                buckets.erase(iter);
                delete bckt;
                return item;
            }
        }

        return T();
    }

    // Gets the index of the specified item.
    int32_t gett(T item) {
        auto iter = std::find_if(buckets.begin(), buckets.end(), [&item](index_bucket<T, S>* bckt) { return bckt->item(item) != -1; });
        if (iter == buckets.end())
            return -1;
        index_bucket<T, S>* bckt = buckets[iter - buckets.begin()];
        return bckt->item(item);
    }

    // Gets the item at the specified index.
    T geti(int32_t index) {
        auto iter = std::find_if(buckets.begin(), buckets.end(), [&index](index_bucket<T, S>* bck) { return bck->bucket_index == (index / S); });
        if (iter == buckets.end())
            return T();
        index_bucket<T, S>* bckt = buckets[iter - buckets.begin()];
        return bckt->items[index % S];
    }
};

#endif
