#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <fstream>
#include <filesystem>

// Include the headers we created in Phase 1 & 2
// Ensure these files are in the same folder
#include "DependencyGraph.h"
#include "FilenameTrie.h"
#include "BPlusTreeNode.h" 
#include "MetadataCache.h"

namespace fs = std::filesystem;
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
    std::string storage_path;
    DependencyGraph* graph;
    FilenameTrie* trie;
    MetadataCache* metadata;
    
    //reverse index keywords to files
    std::map<std::string, std::vector<std::string>> keyword_index;
    //forward index files to keywords
    std::map<std::string, std::vector<std::string>> file_keywords;
    //system keywords
    std::vector<std::string> system_keywords = {"important", "draft", "source", "config", "data"};

    const int K_MAX_KEYS = 5;

public:
    CognitiveDFS() {
        storage_path = "C:/cmfs_storage/";
        if (!fs::exists(storage_path)) {
            fs::create_directories(storage_path);
        }
        
        graph = new DependencyGraph();
        trie = new FilenameTrie();
        metadata = new MetadataCache();
        
        for (const auto& entry : fs::directory_iterator(storage_path)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                trie->insert(filename, filename);
            }
        }
        
        std::cerr << "[CMFS] System Initialized. Storage: " << storage_path << "\n";
    }

    ~CognitiveDFS() {
        delete graph; delete trie; delete metadata;
    }


    // Command: WRITE <filename> <content>
    void writeFile(const std::string& filename, const std::string& content) {
        trie->insert(filename, filename); 
        std::string filepath = storage_path + filename;
        std::ofstream outfile(filepath);
        if (!outfile.is_open()) {
            std::cout << buildJSONResponse("error", "Failed to create file") << std::endl;
            return;
        }
        outfile << content;
        outfile.close();

        FileMetadata meta;
        meta.file_size = content.size();
        metadata->setMetadata(filename, meta);

        std::cout << buildJSONResponse("success", "File written successfully", "\"file\": \"" + filename + "\"") << std::endl;
    }


    // Command: READ <filename>
    void readFile(const std::string& filename) {
        std::string filepath = storage_path + filename;
        
        if (!fs::exists(filepath)) {
            std::cout << buildJSONResponse("error", "File not found") << std::endl;
            return;
        }

        std::ifstream infile(filepath);
        if (!infile.is_open()) {
            std::cout << buildJSONResponse("error", "Failed to open file") << std::endl;
            return;
        }
        
        std::stringstream buffer;
        buffer << infile.rdbuf();
        std::string content = buffer.str();
        infile.close();

        std::vector<Dependency> predictions = graph->getTopDependencies(filename);
        std::string prediction_json = "[";
        for (size_t i = 0; i < predictions.size(); ++i) {
            prediction_json += "\"" + predictions[i].file_id + "\"";
            if (i < predictions.size() - 1) prediction_json += ",";
        }
        prediction_json += "]";

        std::string extra = "\"content\": \"" + content + "\", \"source\": \"DISK\", \"predictions\": " + prediction_json;
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
        std::vector<std::string> files;
        for (const auto& entry : fs::directory_iterator(storage_path)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (prefix.empty() || filename.find(prefix) == 0) {
                    files.push_back(filename);
                }
            }
        }
        
        std::string file_list_json = "[";
        for (size_t i = 0; i < files.size(); ++i) {
            std::string filename = files[i];
            file_list_json += "{\"name\":\"" + filename + "\",\"tags\":[";
            
            if (file_keywords.count(filename)) {
                const auto& tags = file_keywords[filename];
                for (size_t j = 0; j < tags.size(); ++j) {
                    file_list_json += "\"" + tags[j] + "\"";
                    if (j < tags.size() - 1) file_list_json += ",";
                }
            }
            
            file_list_json += "]}";
            if (i < files.size() - 1) file_list_json += ",";
        }
        file_list_json += "]";
        
        std::cout << buildJSONResponse("success", "Directory listed", "\"files\": " + file_list_json) << std::endl;
    }


    // Update this inside your CognitiveDFS class in main.cpp
    void deleteFile(const std::string& filename) {
        std::string filepath = storage_path + filename;
        if (!fs::exists(filepath)) {
            std::cout << buildJSONResponse("error", "File not found") << std::endl;
            return;
        }


        if (file_keywords.count(filename)) {
            for (const std::string& key : file_keywords[filename]) {
                if (keyword_index.count(key)) {
                    std::vector<std::string>& file_list = keyword_index[key];
                    file_list.erase(
                        std::remove(file_list.begin(), file_list.end(), filename), 
                        file_list.end()
                    );
                    if (file_list.empty()) {
                        keyword_index.erase(key);
                    }
                }
            }
            file_keywords.erase(filename);
        }

        trie->remove(filename);
        metadata->removeMetadata(filename);
        fs::remove(filepath);

        std::cout << buildJSONResponse("success", "File '" + filename + "' deleted") << std::endl;
    }

    void tagFile(const std::string& filename, const std::string& keyword) {
        std::string filepath = storage_path + filename;
        if (!fs::exists(filepath)) {
            std::cout << buildJSONResponse("error", "Cannot tag: File does not exist") << std::endl;
            return;
        }

        if (file_keywords[filename].size() >= K_MAX_KEYS) {
            std::cout << buildJSONResponse("error", "Limit reached: Maximum " + std::to_string(K_MAX_KEYS) + " keys per file") << std::endl;
            return;
        }

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

    void suggestKeywords(const std::string& prefix) {
        std::string suggestions_json = "[";
        std::vector<std::string> matches;

        // Compatibility fix: use standard iterator instead of structured bindings [key, val]
        for (auto it = keyword_index.begin(); it != keyword_index.end(); ++it) {
            const std::string& current_keyword = it->first; 
            
            // Safety check: ensure prefix isn't longer than the keyword itself
            if (prefix.size() <= current_keyword.size()) {
                if (current_keyword.substr(0, prefix.size()) == prefix) {
                    matches.push_back(current_keyword);
                }
            }
        }

        for (size_t i = 0; i < matches.size(); ++i) {
            suggestions_json += "\"" + matches[i] + "\"";
            if (i < matches.size() - 1) suggestions_json += ",";
        }
        suggestions_json += "]";

        std::cout << buildJSONResponse("success", "Suggestions fetched", "\"suggestions\": " + suggestions_json) << std::endl;
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

// ... inside main loop ...

else if (line.find("SUGGEST_KEYS") != std::string::npos) {
    // Find "prefix":" and jump past it (+10 characters)
    size_t k_start = line.find("\"prefix\":\"");
    if (k_start != std::string::npos) {
        k_start += 10;
        size_t k_end = line.find("\"", k_start);
        if (k_end != std::string::npos) {
            std::string prefix = line.substr(k_start, k_end - k_start);
            fs.suggestKeywords(prefix);
        }
    }
}
        else {
            // This tells us exactly what the C++ received so we can fix it
            std::cout << "{\"status\":\"error\",\"message\":\"Unknown command received: " << line << "\"}" << std::endl;
        }
    }
    return 0;
}
