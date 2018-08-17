# Index-Hash-Table
Pseudo Hash-Table for efficiently indexing items. This uses a basic "perfect hash," where the input key produces a collision-free entry in the hash table. Essentially the input key is the index and the output hash is the same index value.

The `index_table` holds a `std::vector<>` of buckets, each bucket holds a range of indices from `N to N+X` where the bucket's index is `INT32_T(N/X)`. Where `N` is the lower range index and `N+X` is the upper range index where `X` represents the total number of items in the bucket.

So if given the a bucket size of `X = 16` and an index of `168` our bucket index will be `168/16 = 10` and the index of the item in the bucket is `168%16 = 8`.

## How It Works
Because every item we put in should have an index, we don't necessarily need to manually assign an index to each item as a "key." Instead the `index_table` will assign each item an index in the first bucket with an index not populated by another item. In this case `keys` are automatically determined by the `index_table` and handed back to you. When we add new items to the `index_table` a new bucket will be created when all other buckets are full--likewise when a bucket is empty it will be deallocted to save memory/space.

```C++
template<typename T, size_t S>
class index_table {
    std::vector<index_bucket<T,S> buckets;
    
    index_table(int32_t cache);         // Creates an index_table and pre-allocates a # of buckets.
    size_t count();                     // Returns the number of items in the index_table.
    size_t sizeb();                     // Returns the max number of items in a bucket <S>.
    size_t sizei();                     // Returns the constant size of each item <T>.
    int32_t insert(T item);             // Inserts a new item in the table.
    int32_t removet(T item);            // Removes an existing item from the table.
    T removei(int32_t index);           // Removes an item at the specified index in the table.
    int32_t gett(T item);               // Gets the index of the item or -1 if the item is not in the table.
    T geti(int32_t index);              // Gets the item in the table at the specified index, else nullptr.
}

template<typename T, size_t S>
class index_bucket {
    T items[S]
    int32_t filled;
    int32_t bucket_index;
    
    index_bucket(size_t bucket_index);  // Creates a bucket and assigns it a bucket index.
    int32_t get_index();                // Get the first empty index in the bucket.
    int32_t item(T item);               // Get the index of the item in the bucket, else -1 if it does not exist.
    int32_t insert(T item);             // Inserts a new item into the bucket and returns the item's index.
    int32_t remove(T item);             // Removes the item from the bucket and returns the index it was removed from.
}
```

## Performance
In worst case creating a new bucket will be `O(n)` where `n` is the number of buckets and best case `O(1)` where we have a free bucket index available on the stack to allocate to a new bucket--a bucket's index is thrown on a stack when that bucket is deallocated.

Let's define `b = # of buckets` and `n = # of items` and `s = size of bucket`.
- Item Inserts: `O(b + s)`
- Item Removes: `O(n)` (remove by item) or `O(b)` (remove by index).
- Item Searchs: `O(n)` (remove by item) or `O(b)` (remove by index).
- Item Counts: `O(b + s)`
