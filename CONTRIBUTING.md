---

# Contributing to CMFS

Thanks for wanting to contribute! This project is a mix of C++, Node.js, and React, so there’s plenty of room to help out.

To keep things organized, please follow these standard steps:

## 🔄 The Workflow

1. **Find or Create an Issue:** Before starting, check the [Issues](https://www.google.com/search?q=https://github.com/Hrithik-Bhattacharya/Cognitive-Memory-File-Management-System/issues) tab. If you find a bug or have an idea, open an issue so we can discuss it.
2. **Fork the Repo:** Click the **Fork** button at the top right of this page to create your own copy of the project.
3. **Clone your Fork:**
```bash
git clone https://github.com/YOUR_USERNAME/Cognitive-Memory-File-Management-System.git

```


4. **Create a Branch:** Keep your changes organized by creating a new branch.
```bash
git checkout -b feature/your-feature-name

```


5. **Make your Changes:** Commit your work with clear messages.
```bash
git commit -m "Add keyword ranking logic to the engine"

```


6. **Push and Pull Request:** Push to your fork and submit a Pull Request (PR) to the `main` branch of the original repo.

---

## 🛠 Project Specifics

Since this system interacts with hardware, keep these few things in mind:

* **Testing Safety:** If you’re modifying the C++ disk logic, **do not test on your C: drive**. Use a USB stick or a Virtual Hard Disk (VHD).
* **Admin Rights:** You will likely need to run your terminal as Administrator for the C++ backend to access physical sectors.
* **JSON Communication:** The C++ and React parts talk to each other using JSON. If you change the data format, make sure both sides are updated so the "bridge" doesn't break.
* **Sector Alignment:** Remember that real disk I/O requires 512-byte alignment.

## 💡 What can you work on?

* **Bug fixes:** Any logic errors in the ranking engine or UI.
* **UI/UX:** Improving the disk visualization grid or the search interface.
* **New Features:** Adding support for more file types or basic directory nesting, improve ranking engine efficiency etc.

---
