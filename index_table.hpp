#ifndef INDEX_TABLE
#define INDEX_TABLE
/*
    This library provides an index lookup table without wasting memory space.
    So what we do is create a hash table with a perfect hash, where the input index is the hash.
    This results in a zero-collision perfect hash table (technically a pseudo hash-table).

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

/*
    Used via index_table<T, S>  to store items at their position in the index_table.
    The bucket can only hold <S> number of items according to it's index_table definition.

    T: Type of data you want to store.
    S: Size of each bucket's cache for storing items.
*/
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

    // Gets the index of the item if it exists, else null.
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
        return index;
    }

    // Returns the index of the item that was removed, else -1.
    int32_t remove(T item) {
        int32_t index = std::distance(items, std::find_if(std::begin(items), std::end(items), [&item](T src) { return (src == item); }));
        if (index >= 0) {
            items[index] = T();
            filled--;
        }
        return (index != S) ? index : -1;
    }
};

/*
    Simple form of a hash table to create key-value pairs between items <T> and indices without
    using red-black trees or other typical hash-table methods.

    T: Type of data you want to store.
    S: Size of each bucket's cache for storing items.
*/
template<typename T, size_t S>
class index_table {
    public:
    // Vector containing all unordered buckets and their items.
    std::vector<index_bucket<T, S>*> buckets;
    private:
    // Keeps track of all bucket indices of delete buckets so that the indices can be re-queued for new buckets.
    std::stack<int32_t> empty;
    // Tracks the last bucket index for creating buckets when all buckets from 0 to N are full.
    // This is because the buckets vector can have buckets out of order, so we use this to keep from searching for the highest bucket index via the vector.
    int32_t bucket_hiindex;
    
    // Creates a new bucket with the next freely available range of indices on the stack.
    index_bucket<T, S>* bucket() {
        int32_t bindex = -1;
        index_bucket<T,S>* bckt;

        if (empty.size() > 0) {
            // This means not all buckets are present and ones have been deleted, freeing up bucket ranges.
            bindex = empty.top();
            empty.pop();
        } else {
            if (buckets.size() > 0) {
                // This means all bucket ranges are present, so just create a new bucket at the end range.
                bindex = bucket_hiindex + 1;
                bucket_hiindex = bindex;
            } else {
                // No buckets are present, so create a new bucket starting at the [0 ... S] range.
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
        // Set the highest found bucket index in the table to 0 (no buckets).
        bucket_hiindex = 0;
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

        return bckt->insert(item) + (bckt->bucket_index * S);
    }

    // Removes the specified item from the index table.
    int32_t removet(T item) {
        // Find any bucket that contains the item as an iterator.
        auto iter = std::find_if(buckets.begin(), buckets.end(), [&item](index_bucket<T, S>* bckt) { return bckt->item(item) != -1; });
        if (iter == buckets.end())
            return -1;
        // Get the bucket from the iterator found.
        index_bucket<T, S>* bckt = buckets[iter - buckets.begin()];

        // If the bucket exists then remove the item.
        if (bckt != nullptr) {
            int32_t index = bckt->remove(item) + (bckt->bucket_index * S);

            // If the bucket has no items, delete it.
            if (bckt->filled <= 0) {
                buckets.erase(iter);
                empty.push(bckt->bucket_index);
                delete bckt;
            }

            return index;
        }

        return -1;
    }

    // Removes the item at the specified index from the index table.
    T removei(int32_t index) {
        // Find any bucket that contains the item as an iterator.
        auto iter = std::find_if(buckets.begin(), buckets.end(), [&index](index_bucket<T, S>* bck) { return bck->bucket_index == (index / S); });
        if (iter == buckets.end())
            return T();
        // Get the bucket from the iterator found.
        index_bucket<T, S>* bckt = buckets[iter - buckets.begin()];
        
        // If the bucket exists then remove the item.
        if (bckt != nullptr) {
            T item = bckt->items[index % S];
            bckt->remove(item);

            // If the bucket has no items, delete it.
            if (bckt->filled <= 0) {
                buckets.erase(iter);
                empty.push(bckt->bucket_index);
                delete bckt;
            }

            return item;
        }

        return T();
    }

    // Gets the index of the specified item.
    int32_t gett(T item) {
        // Find any bucket that contains the item as an iterator.
        auto iter = std::find_if(buckets.begin(), buckets.end(), [&item](index_bucket<T, S>* bckt) { return bckt->item(item) != -1; });
        if (iter == buckets.end())
            return -1;
        // Get the bucket from the iterator found.
        index_bucket<T, S>* bckt = buckets[iter - buckets.begin()];
        return bckt->item(item) + (bckt->bucket_index * S);
    }

    // Gets the item at the specified index.
    T geti(int32_t index) {
        auto iter = std::find_if(buckets.begin(), buckets.end(), [&index](index_bucket<T, S>* bck) { return bck->bucket_index == (index / S); });
        if (iter == buckets.end())
            return T();
        // Get the bucket from the iterator found.
        index_bucket<T, S>* bckt = buckets[iter - buckets.begin()];
        return bckt->items[index % S];
    }
};

#endif
