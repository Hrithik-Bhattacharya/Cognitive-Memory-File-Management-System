#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

// Represents a connection to a related file
struct Dependency {
    std::string file_id;
    int weight; // Strength of the relationship

    // Operator for sorting (descending order of weight)
    bool operator>(const Dependency& other) const {
        return weight > other.weight;
    }
};

class DependencyGraph {
private:
    // Adjacency Map: 
    // Key: Source File ID
    // Value: map<Target File ID, Weight>
    // We use a nested map for O(1) lookups when updating weights.
    std::unordered_map<std::string, std::unordered_map<std::string, int>> adj_map;

public:
    // 1. "Learn" a pattern: Record that 'target' was accessed after 'source'
    void updateConnection(const std::string& source, const std::string& target) {
        if (source == target) return; // Ignore self-loops

        // Increment the weight of the edge (Source -> Target)
        // If it doesn't exist, this creates it with value 0 then increments to 1
        adj_map[source][target]++;

        // Optional: Debug log
        // std::cout << "[Graph] strengthened link: " << source << " -> " << target 
        //           << " (Weight: " << adj_map[source][target] << ")\n";
    }

    // 2. "Predict": Get a list of files related to 'source', sorted by probability
    std::vector<Dependency> getTopDependencies(const std::string& source, int limit = 3) {
        std::vector<Dependency> predictions;

        // Check if source exists in graph
        auto it = adj_map.find(source);
        if (it == adj_map.end()) {
            return predictions; // No history for this file
        }

        // Convert the inner map to a vector for sorting
        for (const auto& pair : it->second) {
            predictions.push_back({pair.first, pair.second});
        }

        // Sort by weight (highest frequency first)
        std::sort(predictions.begin(), predictions.end(), 
                  [](const Dependency& a, const Dependency& b) {
                      return a.weight > b.weight; 
                  });

        // Limit the results (e.g., return only top 3 candidates for prefetching)
        if (predictions.size() > limit) {
            predictions.resize(limit);
        }

        return predictions;
    }
    
    // 3. Forgiving Logic: Periodically decay weights so old patterns don't stick forever
    void decayWeights() {
        for (auto& src_pair : adj_map) {
            auto& edges = src_pair.second;
            // Iterate safely and remove weak links
            for (auto it = edges.begin(); it != edges.end(); ) {
                it->second--; // Decrease weight
                if (it->second <= 0) {
                    it = edges.erase(it); // Remove link if weight drops to 0
                } else {
                    ++it;
                }
            }
        }
    }
};