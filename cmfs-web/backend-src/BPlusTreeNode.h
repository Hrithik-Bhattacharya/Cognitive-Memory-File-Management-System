#include <vector>
#include <map>
#include <algorithm>
#include <string>

// Define the order of the B+ Tree (max keys per node)
const int ORDER = 4;

// Forward declaration
class BPlusTreeNode;

struct FileIndexData {
    // Unique ID for the file
    std::string file_id; 
    // Pointer to the start block on the simulated disk
    long long disk_block_address; 

    // Constructor for convenience
    FileIndexData(const std::string& id, long long addr) 
        : file_id(id), disk_block_address(addr) {}
};

class BPlusTreeNode {
public:
    // Keys: Stores the file path or name (our search key)
    std::vector<std::string> keys;

    // Children: Pointers to child nodes (internal nodes only)
    std::vector<BPlusTreeNode*> children;

    // Values: Stores the actual FileIndexData (leaf nodes only)
    // We use a map to keep key-value pairs sorted in the leaf
    std::map<std::string, FileIndexData> values;

    // Pointers for linked list structure (leaf nodes only)
    BPlusTreeNode* next = nullptr; 
    BPlusTreeNode* prev = nullptr;

    // Metadata
    bool is_leaf;
    BPlusTreeNode* parent = nullptr;

    BPlusTreeNode(bool leaf = false) : is_leaf(leaf) {}
    ~BPlusTreeNode() {
        if (!is_leaf) {
            for (BPlusTreeNode* child : children) {
                delete child;
            }
        }
    }

    // A placeholder function to find the index of the first key >= target_key
    int findKeyIndex(const std::string& key) {
        return std::upper_bound(keys.begin(), keys.end(), key) - keys.begin();
    }
};