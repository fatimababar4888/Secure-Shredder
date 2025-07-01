#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <random>
#include <thread>
#include <filesystem>
#include <iomanip>
#include <windows.h>
#include <mutex>
#include <limits>
#include <sstream>
#include <windows.h>
#include <ntddscsi.h>
#include <winioctl.h>

#undef max

std::mutex wipe_mutex;

// Utility function to convert std::string to std::wstring
std::wstring string_to_wstring(const std::string& str) {
    return std::wstring(str.begin(), str.end());
}

// Utility to set console text color (Windows)
void set_console_color(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

// Utility to reset console color to default
void reset_console_color() {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15); // Default color
}

// Clear the console screen
void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Display a loading screen with a progress bar
void loading_screen() {
    clear_screen();

    set_console_color(10); // Green text for loading
    std::cout << "\n\n";
    std::cout << "  ██████████████████████████████████████████████████████████████\n";
    std::cout << "  █                                                            █\n";
    std::cout << "  █      Secure Shredder Tool is loading, please wait...       █\n";
    std::cout << "  █                                                            █\n";
    std::cout << "  ██████████████████████████████████████████████████████████████\n\n";

    set_console_color(11); // Bright blue for progress bar
    for (int i = 0; i <= 100; i += 5) {
        std::cout << "\r[";
        for (int j = 0; j < 20; ++j) {
            if (j < i / 5)
                std::cout << "█";
            else
                std::cout << " ";
        }
        std::cout << "] " << i << "%";

        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // Pulsating effect
        if (i % 20 == 0) {
            set_console_color(14); // Yellow pulsation
        }
        else {
            set_console_color(11); // Reset to blue
        }
    }

    std::cout << "\n\n";
    reset_console_color();
    set_console_color(10); // Green for success message
    std::cout << "Loading complete!\n";
    reset_console_color();
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

// Confirm action with the user
bool confirm_action(const std::string& action) {
    set_console_color(14); // Yellow for warnings
    std::cout << "Are you sure you want to " << action << "? (y/n): ";
    reset_console_color();
    std::string confirmation;
    std::getline(std::cin, confirmation);
    return (confirmation == "y" || confirmation == "Y");
}

// Wipe metadata
void wipe_metadata(const std::string& file_path) {
    std::wstring wide_file_path = string_to_wstring(file_path);

    HANDLE hFile = CreateFile(
        wide_file_path.c_str(), FILE_WRITE_ATTRIBUTES, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        set_console_color(12); // Red for error
        std::cerr << "Error: Unable to open file for metadata wiping. Error code: " << GetLastError() << "\n";
        reset_console_color();
        return;
    }

    FILETIME ft;
    GetSystemTimeAsFileTime(&ft); // Get current system time
    if (!SetFileTime(hFile, &ft, &ft, &ft)) {
        set_console_color(12); // Red for error
        std::cerr << "Error: Unable to modify file times. Error code: " << GetLastError() << "\n";
        reset_console_color();
    }

    CloseHandle(hFile);
}

void wipe_file(const std::string& file_path, int passes = 3, bool xor_overwrite = false) {
    try {
        // Open the file in binary mode
        std::ifstream file(file_path, std::ios::binary | std::ios::ate);
        if (!file) throw std::ios_base::failure("Error opening file");

        size_t file_size = file.tellg();
        file.close();  // Close the input file stream

        // Open the file again in read-write mode to overwrite its contents
        std::ofstream out(file_path, std::ios::binary | std::ios::in);
        if (!out) throw std::ios_base::failure("Error opening file for overwriting");

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);

        // Perform the overwrite operation for the specified number of passes
        for (int i = 0; i < passes; ++i) {
            set_console_color(11); // Bright blue for progress
            std::cout << "Overwriting file with random data (pass " << i + 1 << "/" << passes << ")...\n";
            reset_console_color();

            out.seekp(0);
            for (size_t j = 0; j < file_size; ++j) {
                unsigned char byte = dis(gen);
                if (xor_overwrite) {
                    byte ^= 0xFF; // XOR-based overwrite
                }
                out.put(byte);
            }
            std::cout << "\n";
        }

        set_console_color(10); // Green for success
        std::cout << "File wiped securely.\n";
        reset_console_color();

        out.close();  // Close the output file stream

        // Now delete the file
        if (std::remove(file_path.c_str()) == 0) {
            set_console_color(10); // Green for success
            std::cout << "File deleted successfully.\n";
            reset_console_color();
        }
        else {
            set_console_color(12); // Red for error
            std::cerr << "Error: Unable to delete the file. It might still be in use.\n";
            reset_console_color();
        }
    }
    catch (const std::exception& e) {
        set_console_color(12); // Red for errors
        std::cerr << "Error: " << e.what() << "\n";
        reset_console_color();
    }
}


// Wipe a folder securely (Delete files in the folder)
void wipe_folder(const std::string& folder_path) {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(folder_path)) {
            if (std::filesystem::is_regular_file(entry.status())) {
                wipe_file(entry.path().string());
                std::filesystem::remove(entry.path());
            }
        }
        set_console_color(10); // Green for success
        std::cout << "Folder wiped securely.\n";
        reset_console_color();
    }
    catch (const std::exception& e) {
        set_console_color(12); // Red for errors
        std::cerr << "Error: " << e.what() << "\n";
        reset_console_color();
    }
}

// Function to securely wipe a partition (WARNING: This is a dangerous operation)
void wipe_partition(const std::string& drive_letter) {
    // Format the partition by issuing a low-level disk write operation (this is a dangerous operation)
    std::string command = "format " + drive_letter + ": /fs:NTFS /p:5 /x /q";

    if (system(command.c_str()) != 0) {
        set_console_color(12); // Red for errors
        std::cerr << "Error: Unable to wipe the partition.\n";
        reset_console_color();
    }
    else {
        set_console_color(10); // Green for success
        std::cout << "Partition wiped securely.\n";
        reset_console_color();
    }
}

// View file contents in hexadecimal format
void view_file_in_hex(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        set_console_color(12); // Red for errors
        std::cerr << "Error: Unable to open file.\n";
        reset_console_color();
        return;
    }

    std::cout << "File contents in hex:\n";
    unsigned char byte;
    while (file.read(reinterpret_cast<char*>(&byte), sizeof(byte))) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
    }
    std::cout << std::dec << "\n"; // Reset back to decimal formatting
    file.close();
}

// Ask the user if they want to return to the main menu or exit
void ask_to_continue() {
    char choice;
    while (true) {
        set_console_color(14); // Yellow for prompts
        std::cout << "\nDo you want to return to the main menu or exit? (m = Menu, e = Exit): ";
        reset_console_color();
        std::cin >> choice;
        std::cin.ignore(); // Clear the newline

        if (choice == 'm' || choice == 'M') {
            return;  // Return to main menu
        }
        else if (choice == 'e' || choice == 'E') {
            set_console_color(10); // Green for exit
            std::cout << "Exiting tool. Stay secure!\n";
            reset_console_color();
            exit(0); // Exit the program
        }
        else {
            set_console_color(12); // Red for invalid input
            std::cout << "Invalid choice. Please try again.\n";
            reset_console_color();
        }
    }
}

// Main menu for the tool
void main_menu() {
    while (true) {
        clear_screen();
        set_console_color(13); // Magenta for header
        std::cout << "\n";
        std::cout << "  ██████████████████████████████████████████████████████████████\n";
        std::cout << "  █                                                            █\n";
        std::cout << "  █                    Secure Shredder Tool                    █\n";
        std::cout << "  █                                                            █\n";
        std::cout << "  ██████████████████████████████████████████████████████████████\n";
        reset_console_color();

        set_console_color(11); // Blue for menu
        std::cout << "\n";
        std::cout << "  1. Securely Overwrite File\n";
        std::cout << "  2. Securely Delete File\n";
        std::cout << "  3. Securely Delete Directory\n";
        std::cout << "  4. View File in Hex\n";
        std::cout << "  5. Securely Delete Partition\n";
        std::cout << "  6. Exit\n";
        reset_console_color();

        set_console_color(14); // Yellow for prompts
        std::cout << "Select an option: ";
        reset_console_color();

        std::string choice;
        std::getline(std::cin, choice);

        if (choice == "1") {
            std::string path;
            std::cout << "Enter file path: ";
            std::getline(std::cin, path);

            bool xor_overwrite;
            std::cout << "Use XOR-based overwrite for faster performance? (1 = Yes, 0 = No): ";
            std::cin >> xor_overwrite;
            std::cin.ignore(); // Clear the newline

            int passes;
            std::cout << "Enter the number of passes (1-10): ";
            std::cin >> passes;
            std::cin.ignore(); // Clear the newline
            std::cout << "The number of passes ensures that the data is overwritten multiple times, making it more difficult to recover. More passes increase security but take more time.\n";

            if (!confirm_action("overwrite this file")) {
                std::cout << "Operation cancelled.\n";
                continue;
            }

            wipe_file(path, passes, xor_overwrite);
            ask_to_continue();
        }
        else if (choice == "2") {
            std::string path;
            std::cout << "Enter file path: ";
            std::getline(std::cin, path);

            if (!confirm_action("delete this file")) {
                std::cout << "Operation cancelled.\n";
                continue;
            }

            wipe_file(path);
            ask_to_continue();
        }
        else if (choice == "3") {
            std::string path;
            std::cout << "Enter folder path: ";
            std::getline(std::cin, path);

            if (!confirm_action("wipe this folder")) {
                std::cout << "Operation cancelled.\n";
                continue;
            }

            wipe_folder(path);
            ask_to_continue();
        }
        else if (choice == "4") {
            std::string path;
            std::cout << "Enter file path to view in hex: ";
            std::getline(std::cin, path);

            view_file_in_hex(path);
            ask_to_continue();
        }
        else if (choice == "5") {
            std::string drive_letter;
            std::cout << "Enter the drive letter of the partition to delete (e.g., C, D): ";
            std::getline(std::cin, drive_letter);

            if (!confirm_action("delete the partition")) {
                std::cout << "Operation cancelled.\n";
                continue;
            }

            wipe_partition(drive_letter);
            ask_to_continue();
        }
        else if (choice == "6") {
            set_console_color(10); // Green for exit
            std::cout << "Exiting tool. Stay secure!\n";
            reset_console_color();
            break;
        }
        else {
            set_console_color(12); // Red for invalid input
            std::cout << "Invalid option. Please try again.\n";
            reset_console_color();
        }
    }
}

int main() {
    SetConsoleOutputCP(CP_UTF8); // Set console to UTF-8 to fix encoding issues
    loading_screen();
    main_menu();
    return 0;
}
