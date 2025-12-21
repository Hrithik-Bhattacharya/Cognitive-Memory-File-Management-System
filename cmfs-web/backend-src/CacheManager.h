#include <list>
#include <vector>
#include <unordered_map>
#include <string>
#include <iostream>
#include <algorithm>

// Structure to hold data blocks in the cache
struct CacheBlock {
    std::string block_id; // Key (e.g., file_id + block_number)
    std::vector<char> data; // The actual file data content
    long long access_count = 0; // The priority metric

    CacheBlock(const std::string& id, size_t size) : block_id(id), data(size) {}
};

// Typedef for readability: a list of CacheBlock objects
using BlockList = std::list<CacheBlock>;
// Typedef for the mapping: Key -> Iterator to the list element
using BlockMap = std::unordered_map<std::string, BlockList::iterator>;

class CacheManager {
private:
    BlockList lru_list; // Doubly Linked List for LRU eviction (Min-Heap logic)
    BlockMap block_map; // Hash Table for O(1) lookup
    const size_t MAX_SIZE;

public:
    CacheManager(size_t max_size) : MAX_SIZE(max_size) {}

    // 1. Get a block from the cache
    bool getBlock(const std::string& block_id, CacheBlock& block_out) {
        auto it_map = block_map.find(block_id);
        
        if (it_map == block_map.end()) {
            // Cache Miss
            return false;
        }

        // Cache Hit: Increment Priority and Promote to Front (LRU policy)
        
        // 1. Get the iterator to the block in the list
        BlockList::iterator it_list = it_map->second;
        
        // 2. Increment the priority count
        it_list->access_count++; 
        
        // 3. Promote the block to the front of the list (Most Recently Used)
        // This is the LRU part: The front of the list is high priority/most recently used.
        lru_list.splice(lru_list.begin(), lru_list, it_list);
        
        block_out = *lru_list.begin(); // Copy data out
        return true;
    }

    // 2. Put a block into the cache
    void putBlock(const std::string& block_id, const std::vector<char>& data) {
        // If block already exists (we hit the 'get' first, but for completeness):
        if (block_map.count(block_id)) {
            // Already handled by getBlock and update
            return; 
        }

        // Check for eviction (Min-Heap/LRU logic)
        if (lru_list.size() >= MAX_SIZE) {
            // Evict the Least Recently Used item (at the back of the list)
            
            // In a *pure* Min-Heap, we'd evict the item with the lowest access_count.
            // In this LRU-based system, we evict the least recently used, 
            // simulating the Min-Heap's role as the eviction candidate tracker.
            
            std::string evicted_id = lru_list.back().block_id;
            lru_list.pop_back(); // Remove from list
            block_map.erase(evicted_id); // Remove from map
            std::cout << "[CMFS Cache] Evicting block: " << evicted_id << std::endl;
        }

        // Insert new block at the front (Most Recently Used)
        CacheBlock new_block(block_id, data.size());
        new_block.data = data;
        new_block.access_count = 1; // Initial access
        
        lru_list.push_front(new_block);
        
        // Update the map to point to the new head of the list
        block_map[block_id] = lru_list.begin();
    }
};