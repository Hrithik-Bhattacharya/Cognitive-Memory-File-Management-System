#include <iostream>
#include <fstream>
#include <vector>
#include <string>

// We simulate a disk with 4KB blocks
const int BLOCK_SIZE = 4096; 

class VirtualDisk {
private:
    std::string disk_filename;
    std::fstream disk_stream;
    long long total_blocks;

public:
    VirtualDisk(const std::string& filename, long long num_blocks) 
        : disk_filename(filename), total_blocks(num_blocks) {
        
        // Check if disk file exists, if not create it
        disk_stream.open(disk_filename, std::ios::in | std::ios::out | std::ios::binary);
        if (!disk_stream.is_open()) {
            std::cout << "[Disk] Creating new virtual disk: " << filename << std::endl;
            // Create file
            std::ofstream outfile(disk_filename, std::ios::binary);
            // Initialize with zeros (sparse file usually)
            // For simplicity, we just write a byte at the end to set size
            outfile.seekp((num_blocks * BLOCK_SIZE) - 1);
            outfile.write("", 1);
            outfile.close();

            // Re-open in read/write mode
            disk_stream.open(disk_filename, std::ios::in | std::ios::out | std::ios::binary);
        }
    }

    ~VirtualDisk() {
        if (disk_stream.is_open()) {
            disk_stream.close();
        }
    }

    // Write a block of data to a specific index
    bool writeBlock(long long block_index, const std::vector<char>& data) {
        if (block_index >= total_blocks || data.size() > BLOCK_SIZE) {
            std::cerr << "[Disk Error] Invalid write operation" << std::endl;
            return false;
        }

        disk_stream.clear(); // Clear any error flags
        disk_stream.seekp(block_index * BLOCK_SIZE);
        disk_stream.write(data.data(), data.size());
        disk_stream.flush(); // Ensure it hits the physical disk
        return true;
    }

    // Read a block of data from a specific index
    bool readBlock(long long block_index, std::vector<char>& buffer) {
        if (block_index >= total_blocks) {
            std::cerr << "[Disk Error] Block index out of bounds" << std::endl;
            return false;
        }

        disk_stream.clear();
        disk_stream.seekg(block_index * BLOCK_SIZE);
        
        buffer.resize(BLOCK_SIZE);
        disk_stream.read(buffer.data(), BLOCK_SIZE);

        return true;
    }
    
    long long getCapacity() const {
        return total_blocks * BLOCK_SIZE;
    }
};