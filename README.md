This README is designed to give your repository a professional, high-level "System Architecture" feel. It highlights your technical accomplishments (like the real disk I/O and data structures) while clearly defining the scope for future development.

---

# CMFS: Cognitive Memory File Management System

**CMFS** is a low-level file management engine designed to bridge the gap between physical hardware storage and human cognitive patterns. Instead of forcing users to remember complex directory paths, CMFS utilizes a **Semantic Tagging & Ranking Engine** to retrieve files based on keywords and relevance.

This project was built as a demonstration of high-performance data structures interacting with real-world Windows hardware volumes.

---

## 🚀 Project Philosophy

Human memory is associative, not hierarchical. We rarely remember that a file is in `C:/Users/Docs/Work/2026/Project/draft.txt`, but we remember it is a **#Draft** related to **#Work**. CMFS prioritizes these associations, making retrieval instantaneous via an Inverted Indexing architecture.

---

## 🛠️ Current Feature Set (MVP)

* **Physical Volume Interface:** Bypasses standard OS file-tree limitations to interact with local storage/logical volumes via Win32 API.
* **Full CRUD Operations:** Support for Creating, Reading, Updating, and Deleting files directly within the managed environment.
* **Semantic Tagging:** Assign up to 5 custom keywords per file to build a searchable metadata layer.
* **Inverted Index Search:** High-speed,  search complexity for `.txt` files based on user-defined tags.
* **Real-time System Logs:** A live telemetry stream in the UI showing backend C++ actions (e.g., `READ_PHYSICAL`, `WRITE_PHYSICAL`, sector offsets).
* **Keyword Ranking Engine:** A priority-based system that associates keywords with files for faster semantic access.

---

## 🏗️ Technical Architecture

CMFS utilizes a hybrid data structure approach to ensure efficiency:

1. **Inverted Index (Hash Map):** Maps keywords directly to file pointers for near-instant search results.
2. **Max-Heap (Ranking Engine):** Prioritizes file results based on access frequency and tag relevance.
3. **Virtual/Physical Disk Manager:** Handles raw sector I/O and 512-byte block alignment for local storage management.

---

## 🖥️ Tech Stack

* **Backend:** C++ (Core Engine & Win32 Hardware Interface)
* **Bridge:** Node.js (WebSocket Server for IPC)
* **Frontend:** React (System Dashboard & Visualization UI)

---

## 🚧 Current Limitations & Roadmap

As this is a work-in-progress exploration of data structures:

* [ ] **Extended File Support:** Tag-based content searching is currently optimized for `.txt` files.
* [ ] **Multi-Volume Support:** Expansion from single logical volumes to multi-drive spanning.

---

## 🚦 Getting Started

### Prerequisites

* Windows OS (Required for Win32 Physical Disk API)
* C++ Compiler (MSVC recommended)
* Node.js & NPM
* **Administrative Privileges** (Required to access raw disk sectors)

### Installation

1. **Clone the repository:**
```bash
git clone https://github.com/Hrithik-Bhattacharya/Cognitive-Memory-File-Management-System/

```


2. **Compile the Backend:**
Navigate to `/backend` and compile the C++ source.
```bash
g++ main.cpp -o cmfs.exe

```

4. **Run the Bridge:**
```bash
npm install
node server.js

```


4. **Launch the UI:**
```bash
cd client && npm install && npm start

```
