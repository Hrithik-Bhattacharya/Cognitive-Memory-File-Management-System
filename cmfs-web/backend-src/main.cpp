#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include<algorithm>

// Include the headers we created in Phase 1 & 2
// Ensure these files are in the same folder
#include "VirtualDisk.h"
#include "CacheManager.h"
#include "DependencyGraph.h"
#include "FilenameTrie.h"
#include "BPlusTreeNode.h" 
#include "MetadataCache.h"

// --- Helper: Simple JSON Builder ---
// Helps us send clean JSON back to Node.js/React
std::string buildJSONResponse(const std::string& status, const std::string& message, const std::string& extra = "") {
    std::string json = "{";
    json += "\"status\": \"" + status + "\",";
    json += "\"message\": \"" + message + "\"";
    if (!extra.empty()) {
        json += "," + extra;
    }
    json += "}";
    return json;
}

// --- The Core File System Controller ---
class CognitiveDFS {
private:
    VirtualDisk* disk;
    CacheManager* cache;
    DependencyGraph* graph;
    FilenameTrie* trie;
    MetadataCache* metadata;
    
    // Simulating B+ Tree Index (Mapping Path -> Block Index)
    // In a full implementation, this would be your full B+ Tree Class
    std::map<std::string, long long> index_map; 
    //reverse index keywords to files
    std::map<std::string, std::vector<std::string>> keyword_index;
    //forward index files to keywords
    std::map<std::string, std::vector<std::string>> file_keywords;
    //system keywords
    std::vector<std::string> system_keywords = {"important", "draft", "source", "config", "data"};

    const int K_MAX_KEYS = 5;

    long long next_free_block = 0;

public:
    CognitiveDFS() {
        // Initialize Components
        disk = new VirtualDisk("cmfs_data.img", 1000); // 1000 blocks (~4MB disk)
        cache = new CacheManager(50); // Cache can hold 50 blocks
        graph = new DependencyGraph();
        trie = new FilenameTrie();
        metadata = new MetadataCache();
        
        std::cerr << "[CMFS] System Initialized.\n";
    }

    ~CognitiveDFS() {
        delete disk; delete cache; delete graph; delete trie; delete metadata;
    }

    // Command: WRITE <filename> <content>
    void writeFile(const std::string& filename, const std::string& content) {
        // 1. Assign a block
        long long block_idx = next_free_block++;
        index_map[filename] = block_idx;

        // 2. Write to Trie
        trie->insert(filename, filename); // Map path to ID (simple 1:1 here)

        // 3. Write to Disk
        std::vector<char> buffer(content.begin(), content.end());
        disk->writeBlock(block_idx, buffer);

        // 4. Update Metadata
        FileMetadata meta;
        meta.file_size = content.size();
        metadata->setMetadata(filename, meta);

        // 5. Update Cache (Write-through)
        cache->putBlock(filename, buffer);

        std::cout << buildJSONResponse("success", "File written successfully", "\"file\": \"" + filename + "\"") << std::endl;
    }

    // Command: READ <filename>
    void readFile(const std::string& filename) {
        // 1. Check Trie for existence
        if (trie->search(filename) == "") {
            std::cout << buildJSONResponse("error", "File not found") << std::endl;
            return;
        }

        // 2. Check Cache (The "Min/Max Heap" Logic)
        CacheBlock cached_block("", 0);
        bool cache_hit = cache->getBlock(filename, cached_block);
        
        std::string content;
        std::string source;

        if (cache_hit) {
            // RAM Hit!
            content = std::string(cached_block.data.begin(), cached_block.data.end());
            source = "CACHE";
        } else {
            // Disk Access (Slow)
            long long block_idx = index_map[filename];
            std::vector<char> buffer;
            disk->readBlock(block_idx, buffer);
            
            content = std::string(buffer.begin(), buffer.end());
            // Trim nulls
            content = content.c_str(); 
            source = "DISK";

            // Add to Cache for next time
            cache->putBlock(filename, buffer);
        }

        // 3. Cognitive Step: Predict next file
        std::vector<Dependency> predictions = graph->getTopDependencies(filename);
        std::string prediction_json = "[";
        for (size_t i = 0; i < predictions.size(); ++i) {
            prediction_json += "\"" + predictions[i].file_id + "\"";
            if (i < predictions.size() - 1) prediction_json += ",";
        }
        prediction_json += "]";

        // Output JSON result
        std::string extra = "\"content\": \"" + content + "\", \"source\": \"" + source + "\", \"predictions\": " + prediction_json;
        std::cout << buildJSONResponse("success", "Read successful", extra) << std::endl;
    }

    // Command: ACCESS_PAIR <source> <target>
    // Frontend tells us: "User opened A, then immediately opened B"
    void learnRelationship(const std::string& source, const std::string& target) {
        graph->updateConnection(source, target);
        std::cout << buildJSONResponse("success", "Relationship learned") << std::endl;
    }

    // Command: LIST <prefix>
    void listFiles(const std::string& prefix) {
        std::vector<std::string> files = trie->findWithPrefix(prefix);
        std::string file_list_json = "[";
        for (size_t i = 0; i < files.size(); ++i) {
            file_list_json += "\"" + files[i] + "\"";
            if (i < files.size() - 1) file_list_json += ",";
        }
        file_list_json += "]";
        
        std::cout << buildJSONResponse("success", "Directory listed", "\"files\": " + file_list_json) << std::endl;
    }
    // Update this inside your CognitiveDFS class in main.cpp
    void deleteFile(const std::string& filename) {
    // 1. Verify existence in our index map (Keep your original check)
    if (index_map.find(filename) == index_map.end()) {
        std::cout << buildJSONResponse("error", "File not found in index") << std::endl;
        return;
    }

    // 2. NEW: CLEANUP KEYWORDS (Reverse Index Cleanup)
    // Use .count() to check if the file has any tags before trying to clean up
    if (file_keywords.count(filename)) {
        for (const std::string& key : file_keywords[filename]) {
            // Find the list of files that share this keyword
            if (keyword_index.count(key)) {
                std::vector<std::string>& file_list = keyword_index[key];
                
                // Remove this specific filename from that keyword's vector
                // We use std::remove from <algorithm> here
                file_list.erase(
                    std::remove(file_list.begin(), file_list.end(), filename), 
                    file_list.end()
                );

                // If that was the only file with that keyword, clean up the map
                if (file_list.empty()) {
                    keyword_index.erase(key);
                }
            }
        }
        // Remove the file from the Forward Index (filename -> keywords mapping)
        file_keywords.erase(filename);
    }

    // 3. Remove from Filename Trie 
    bool trieRemoved = trie->remove(filename);

    // 4. Remove from Metadata and Memory Cache
    metadata->removeMetadata(filename);
    
    // 5. Remove from the Index Map (The Block mapping)
    index_map.erase(filename);

    // 6. Success response
    std::cout << buildJSONResponse("success", "File '" + filename + "' and associated tags deleted") << std::endl;
}

    void tagFile(const std::string& filename, const std::string& keyword) {
        // 1. Check if file exists
        if (index_map.find(filename) == index_map.end()) {
            std::cout << buildJSONResponse("error", "Cannot tag: File does not exist") << std::endl;
            return;
        }

        // 2. Enforce k limit
        if (file_keywords[filename].size() >= K_MAX_KEYS) {
            std::cout << buildJSONResponse("error", "Limit reached: Maximum " + std::to_string(K_MAX_KEYS) + " keys per file") << std::endl;
            return;
        }

        // 3. Add to Forward and Reverse Index
        file_keywords[filename].push_back(keyword);
        keyword_index[keyword].push_back(filename);

        std::cout << buildJSONResponse("success", "Keyword '" + keyword + "' associated with " + filename) << std::endl;
    }

    // Command: SEARCH_KEY <keyword>
    void searchByKeyword(const std::string& keyword) {
        if (keyword_index.find(keyword) == keyword_index.end() || keyword_index[keyword].empty()) {
            std::cout << buildJSONResponse("success", "No files found for this key", "\"files\": []") << std::endl;
            return;
        }

        std::string files_json = "[";
        auto& list = keyword_index[keyword];
        for (size_t i = 0; i < list.size(); ++i) {
            files_json += "\"" + list[i] + "\"";
            if (i < list.size() - 1) files_json += ",";
        }
        files_json += "]";

        std::cout << buildJSONResponse("success", "Search complete", "\"files\": " + files_json) << std::endl;
    }

    // List the system pre-defined keywords for the UI
    void getSystemKeywords() {
        std::string keys = "[";
        for (size_t i = 0; i < system_keywords.size(); ++i) {
            keys += "\"" + system_keywords[i] + "\"";
            if (i < system_keywords.size() - 1) keys += ",";
        }
        keys += "]";
        std::cout << buildJSONResponse("success", "System keywords fetched", "\"keywords\": " + keys) << std::endl;
    }

};

// --- Main Loop ---
// Replace your existing main loop with this structured version
int main() {
    CognitiveDFS fs;
    std::string line;

    // Force output to flush immediately so Node.js doesn't wait
    std::cout << std::unitbuf;

    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        // Clean the line: remove any carriage returns (\r) from Windows
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

        // 1. Check for WRITE (Looking for the keyword anywhere in the JSON)
        if (line.find("WRITE") != std::string::npos) {
            size_t f_start = line.find("\"file\":\"") + 8;
            size_t f_end = line.find("\"", f_start);
            std::string filename = line.substr(f_start, f_end - f_start);

            size_t d_start = line.find("\"data\":\"") + 8;
            size_t d_end = line.find("\"", d_start);
            std::string content = line.substr(d_start, d_end - d_start);

            fs.writeFile(filename, content);
        } 
        // 2. Check for READ
        else if (line.find("READ") != std::string::npos) {
            size_t f_start = line.find("\"file\":\"") + 8;
            size_t f_end = line.find("\"", f_start);
            std::string filename = line.substr(f_start, f_end - f_start);
            
            fs.readFile(filename);
        }
        // 3. Check for LIST
        else if (line.find("LIST") != std::string::npos) {
            fs.listFiles("");
        }
        else if (line.find("\"action\":\"DELETE\"") != std::string::npos) {
    size_t f_start = line.find("\"file\":\"");
    if (f_start != std::string::npos) {
        f_start += 8;
        size_t f_end = line.find("\"", f_start);
        std::string filename = line.substr(f_start, f_end - f_start);

        fs.deleteFile(filename); 
        // DELETE THE LINE BELOW IF IT EXISTS:
        // fs.listFiles(""); 
    }
}
        else if (line.find("\"action\":\"TAG\"") != std::string::npos) {
    // Extract file and key
    size_t f_start = line.find("\"file\":\"") + 8;
    size_t f_end = line.find("\"", f_start);
    std::string filename = line.substr(f_start, f_end - f_start);

    size_t k_start = line.find("\"key\":\"") + 7;
    size_t k_end = line.find("\"", k_start);
    std::string keyword = line.substr(k_start, k_end - k_start);

    fs.tagFile(filename, keyword);
}
else if (line.find("\"action\":\"SEARCH_KEY\"") != std::string::npos) {
    size_t k_start = line.find("\"key\":\"") + 7;
    size_t k_end = line.find("\"", k_start);
    std::string keyword = line.substr(k_start, k_end - k_start);

    fs.searchByKeyword(keyword);
}
        else {
            // This tells us exactly what the C++ received so we can fix it
            std::cout << "{\"status\":\"error\",\"message\":\"Unknown command received: " << line << "\"}" << std::endl;
        }
    }
    return 0;
}