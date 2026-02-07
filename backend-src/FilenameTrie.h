#ifndef FILENAMETRIE_H
#define FILENAMETRIE_H

#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

class TrieNode {
public:
    std::unordered_map<char, TrieNode*> children;
    std::string file_id; 
    bool is_end_of_file;

    TrieNode() : is_end_of_file(false) {}
    
    ~TrieNode() {
        for (auto& pair : children) {
            delete pair.second;
        }
    }
};

class FilenameTrie {
private:
    TrieNode* root;

    // --- HELPER 1: For Deletion ---
    bool removeHelper(TrieNode* current, const std::string& filename, int depth) {
        if (!current) return false;

        if (depth == filename.size()) {
            if (!current->is_end_of_file) return false;
            current->is_end_of_file = false;
            current->file_id = "";
            return current->children.empty();
        }

        char ch = filename[depth];
        if (current->children.find(ch) == current->children.end()) return false;

        bool shouldDeleteChild = removeHelper(current->children[ch], filename, depth + 1);

        if (shouldDeleteChild) {
            delete current->children[ch];
            current->children.erase(ch);
            return !current->is_end_of_file && current->children.empty();
        }
        return false;
    }

    // --- HELPER 2: For Listing/Prefix Search ---
    void collectAll(TrieNode* node, std::vector<std::string>& results) {
        if (node->is_end_of_file) {
            results.push_back(node->file_id);
        }
        for (auto& pair : node->children) {
            collectAll(pair.second, results);
        }
    }

public:
    FilenameTrie() { root = new TrieNode(); }
    ~FilenameTrie() { delete root; }

    void insert(const std::string& filename, const std::string& id) {
        TrieNode* current = root;
        for (char ch : filename) {
            if (current->children.find(ch) == current->children.end()) {
                current->children[ch] = new TrieNode();
            }
            current = current->children[ch];
        }
        current->is_end_of_file = true;
        current->file_id = id;
    }

    std::string search(const std::string& filename) {
        TrieNode* current = root;
        for (char ch : filename) {
            if (current->children.find(ch) == current->children.end()) return "";
            current = current->children[ch];
        }
        return (current->is_end_of_file) ? current->file_id : "";
    }

    // This is the function giving you trouble in main.cpp
    std::vector<std::string> findWithPrefix(const std::string& prefix) {
        std::vector<std::string> results;
        TrieNode* current = root;
        for (char ch : prefix) {
            if (current->children.find(ch) == current->children.end()) return results;
            current = current->children[ch];
        }
        collectAll(current, results);
        return results;
    }

    // NEW: The Delete function for main.cpp to call
    bool remove(const std::string& filename) {
        return removeHelper(root, filename, 0);
    }
};

#endif