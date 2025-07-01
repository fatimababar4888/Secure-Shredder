# 🔐 SecurShredder – Advanced File & Partition Data Wiper

A C++ based secure data deletion utility for Windows that ensures complete file removal, metadata wiping, and irreversible partition cleaning. Designed with multi-pass overwrite logic, optional XOR-based wiping, and a colorful, intuitive terminal interface.

---

## 🚀 Features

- ✅ **Secure File Wiping**
  - Overwrites files with random or XOR-encoded data for up to 10 passes
  - Deletes file after secure overwrite

- 📁 **Directory Cleaner**
  - Securely deletes all files in a given folder

- 🔍 **Hex Viewer**
  - Displays raw hex content of any file before deletion

- ⚠️ **Partition Formatter**
  - Performs a fast format with overwrite on any specified drive

- 🕵️‍♂️ **Metadata Scrubbing**
  - Wipes timestamps (creation, modification, access)

- 🎨 **User-Friendly CLI**
  - Windows console color codes
  - Progress bar animation
  - Confirmation prompts to prevent accidental deletions

---

## 🖥️ Screenshots

> Coming soon: animated preview of loading screen, hex viewer, and wipe confirmations

---

## 🛠️ How to Build

> **Platform:** Windows  
> **Dependencies:** None (uses `<windows.h>` and standard C++17)

1. Open the `.cpp` file in Visual Studio (or any C++ IDE with Windows SDK)
2. Enable **C++17 standard**
3. Build and run — no external libraries required.

---

## ⚠️ Warning

- **This tool permanently deletes files and partitions.**
- Use responsibly. We are not liable for data loss due to misuse.

---

## 📂 Menu Options

1. Securely Overwrite File
2. Securely Delete File
3. Securely Delete Directory
4. View File in Hex
5. Securely Delete Partition
6. Exit

Each option includes:
- Prompts for confirmation
- Colored terminal output
- Explanation of impact (e.g. number of passes)

---

## 📄 License

This project is intended for educational use only.

---

Stay safe. Shred responsibly. 🗑️✨
