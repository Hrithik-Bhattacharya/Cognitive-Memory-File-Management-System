#include <unordered_map>
#include <string>
#include <chrono>

struct FileMetadata {
    long long file_size = 0;
    int permissions = 0; 
    std::string owner_id;
    // Times are often stored as standard library high-resolution clock types
    std::chrono::system_clock::time_point creation_time;
    std::chrono::system_clock::time_point modification_time;
    // Add other critical metadata as needed
};

class MetadataCache {
private:
    // Key: File Path/ID (string), Value: FileMetadata struct
    std::unordered_map<std::string, FileMetadata> metadata_store;

public:
    // Function to retrieve metadata
    bool getMetadata(const std::string& file_id, FileMetadata& data) {
        // Use find() for efficiency, then check if the key exists
        auto it = metadata_store.find(file_id);
        if (it != metadata_store.end()) {
            data = it->second;
            return true; // Metadata found
        }
        return false; // Metadata not found
    }

    // Function to update or insert metadata
    void setMetadata(const std::string& file_id, const FileMetadata& data) {
        // Insert or overwrite the existing entry
        metadata_store[file_id] = data;
    }

    // Function to remove metadata when a file is deleted
    void removeMetadata(const std::string& file_id) {
        metadata_store.erase(file_id);
    }
};