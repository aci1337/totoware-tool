#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <filesystem>
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#endif
#include <regex>
namespace fs = std::filesystem;

std::string remove_spaces(const std::string& str) {
    std::string result;
    for (const auto& ch : str) {
        if (!std::isspace(ch)) {
            result += ch;
        }
    }
    return result;
}

void set_console_color(int color) {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
#else
    std::cout << "\033[" << color << "m";
#endif
}

void reset_console_color() {
#ifdef _WIN32
    set_console_color(15);
#else
    std::cout << "\033[0m";
#endif
}

void search_and_save(const std::string& path, const std::string& keyword, const std::string& save_mode) {
    std::ofstream output("results/results.txt");
    std::set<std::string> unique_entries;
    int save_option;
    std::cout << "Choose the format for saving the results: " << std::endl;
    std::cout << "1. EMAIL:PASS" << std::endl;
    std::cout << "2. USER:PASS" << std::endl;
    std::cin >> save_option;

    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (entry.path().filename() == "Passwords.txt") {
            std::ifstream input(entry.path());
            std::string line, username, password;
            while (std::getline(input, line)) {
                if (line.find(keyword) != std::string::npos) {
                    while (std::getline(input, line)) {
                        if (line.find("Username:") != std::string::npos) {
                            username = remove_spaces(line.substr(line.find("Username:") + 9));
                        }
                        else if (line.find("Password:") != std::string::npos) {
                            password = remove_spaces(line.substr(line.find("Password:") + 9));
                            if (username.length() >= 3 && password.length() >= 3) {
                                bool is_email = username.find('@') != std::string::npos;
                                if ((save_option == 1 && is_email) || (save_option == 2 && !is_email)) {
                                    std::string entry = username + ":" + password;
                                    if (unique_entries.insert(username).second && username != "UNKNOWN" && password != "UNKNOWN") {
                                        output << entry << std::endl;
                                        set_console_color(10);
                                        std::cout << entry << std::endl;
                                        reset_console_color();
                                    }
                                    else {
                                        set_console_color(12);
                                        std::cout << entry << std::endl;
                                        reset_console_color();
                                    }
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
}

void search_cookies(const std::string& path, const std::string& keyword) {
    fs::create_directory("results/cookies");
    int counter = 1;
    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (entry.path().filename() == "Google_[Chrome]_Default Network.txt") {
            std::ifstream input(entry.path());
            std::string line;
            bool found = false;
            while (std::getline(input, line)) {
                if (line.find(keyword) != std::string::npos) {
                    found = true;
                    break;
                }
            }
            if (found) {
                std::ofstream output("results/cookies/" + std::to_string(counter) + ".txt");
                output << entry.path() << std::endl;
                counter++;
            }
        }
    }
}

void search_autofills(const std::string& path, const std::string& keyword) {
    fs::create_directory("results/autofills");
    int counter = 1;
    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (entry.path().extension() == ".txt" && entry.path().parent_path().filename() == "Autofills") {
            std::ifstream input(entry.path());
            std::string line, prev_line;
            bool save = false;
            while (std::getline(input, line)) {
                if (line.find(keyword) != std::string::npos) {
                    save = true;
                }
                if (prev_line.find("Name:") != std::string::npos && line.find("Value:") != std::string::npos) {
                    if (save) {
                        std::ofstream output("results/autofills/" + std::to_string(counter) + ".txt", std::ios::app);
                        output << prev_line << std::endl;
                        output << line << std::endl;
                        counter++;
                    }
                    save = false;
                }
                prev_line = line;
            }
        }
    }
}

std::vector<fs::path> sort_files_by_date(const std::string& path) {
    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(path)) {
        if (fs::is_regular_file(entry.path())) {
            files.push_back(entry.path());
        }
    }

    std::sort(files.begin(), files.end(), [](const fs::path& a, const fs::path& b) {
        return fs::last_write_time(a) < fs::last_write_time(b);
        });

    return files;
}

void search_ftp_connections(const std::string& path) {
    fs::create_directory("results/ftp");
    int counter = 1;
    std::regex ftp_pattern(R"(ftp:\/\/(?:\w+\:\w+@)?[\w.-]+(?:\:\d+)?(?:\/.*)?)", std::regex::icase);

    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (entry.path().extension() == ".txt") {
            std::ifstream input(entry.path());
            std::string line;
            bool found = false;
            while (std::getline(input, line)) {
                if (std::regex_search(line, ftp_pattern)) {
                    found = true;
                    std::ofstream output("results/ftp/" + std::to_string(counter) + ".txt", std::ios::app);
                    output << line << std::endl;
                }
            }
            if (found) {
                counter++;
            }
        }
    }
}

void search_credit_cards(const std::string& path) {
    fs::create_directory("results/cc");
    int counter = 1;
    std::regex cc_pattern(R"((\b(?:\d{4}[\s\-]{0,1}){3}\d{4}\b))");
    std::regex exp_pattern(R"((?:0[1-9]|1[0-2])\/?([0-9]{2}))");
    std::regex cvv_pattern(R"((\b\d{3,4}\b))");

    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (entry.path().extension() == ".txt") {
            std::ifstream input(entry.path());
            std::string line;
            bool found = false;
            while (std::getline(input, line)) {
                std::smatch cc_match, exp_match, cvv_match;
                if (std::regex_search(line, cc_match, cc_pattern) &&
                    std::regex_search(line, exp_match, exp_pattern) &&
                    std::regex_search(line, cvv_match, cvv_pattern)) {
                    found = true;
                    std::ofstream output("results/cc/" + std::to_string(counter) + ".txt", std::ios::app);
                    output << "CC: " << cc_match[0] << ", Expiry: " << exp_match[0] << ", CVV: " << cvv_match[0] << std::endl;
                }
            }
            if (found) {
                counter++;
            }
        }
    }
}

void search_discord_tokens(const std::string& path) {
    fs::create_directory("results/discord");
    int counter = 1;
    std::regex token_pattern(R"(^[a-zA-Z0-9._-]{24}\.[a-zA-Z0-9._-]{6}\.[a-zA-Z0-9._-]{27}$)");

    for (const auto& entry : fs::recursive_directory_iterator(path)) {
        if (entry.path().extension() == ".txt" && entry.path().parent_path().filename() == "Discord") {
            std::ifstream input(entry.path());
            std::string line;
            bool found = false;
            while (std::getline(input, line)) {
                if (std::regex_search(line, token_pattern)) {
                    found = true;
                    std::ofstream output("results/discord/" + std::to_string(counter) + ".txt", std::ios::app);
                    output << "Token: " << line << std::endl;
                }
            }
            if (found) {
                counter++;
            }
        }
    }
}


int main() {
    int choice;
    std::string path, keyword;
    SetConsoleTitleA("Totoware Tool - created by aci25#7689");
    std::cout << "Launching Totoware tool, make sure you have ur logs ready\n";
    Sleep(500);
    do {
        system("color 4");
        system("cls");
        std::cout << "[1] - Search Accounts" << std::endl;
        std::cout << "[2] - Search Cookies" << std::endl;
        std::cout << "[3] - Search Autofills" << std::endl;
        std::cout << "[4] - Search Credit Cards" << std::endl;
        std::cout << "[5] - Search FTPs" << std::endl;
        std::cout << "[6] - Sort Files By Date" << std::endl;
        std::cout << "[7] - Find Discord Token" << std::endl;
        std::cout << "[0] - Exit" << std::endl;
        std::cout << "Enter your choice: ";
        std::cin >> choice;
        std::string save_mode;

        switch (choice) {
        case 1:
            system("cls");
            std::cin.ignore();
            std::cout << "Enter the path: ";
            std::getline(std::cin, path);
            if (path.empty()) {
                std::cout << "Invalid path selected. Please try again." << std::endl;
                break;
            }
            std::cout << "Enter the keyword: ";
            std::getline(std::cin, keyword);

            while (save_mode != "email" && save_mode != "user") {
                std::cout << "Choose saving mode (email/user): ";
                std::cin >> save_mode;
                if (save_mode != "email" && save_mode != "user") {
                    std::cout << "Invalid choice. Please enter either 'email' or 'user'." << std::endl;
                }
            }

            fs::create_directory("results");
            search_and_save(path, keyword, save_mode);
            break;
        case 2:
            system("cls");
            std::cin.ignore();
            std::cout << "Enter the path: ";
            std::getline(std::cin, path);
            if (path.empty()) {
                std::cout << "Invalid path selected. Please try again." << std::endl;
                break;
            }
            std::cout << "Enter the keyword (e.g., facebook.com/facebook): ";
            std::getline(std::cin, keyword);

            fs::create_directory("results");
            search_cookies(path, keyword);
            break;
        case 3:
            system("cls");
            std::cin.ignore();
            std::cout << "Enter the path: ";
            std::getline(std::cin, path);
            if (path.empty()) {
                std::cout << "Invalid path selected. Please try again." << std::endl;
                break;
            }
            std::cout << "Enter the keyword: ";
            std::getline(std::cin, keyword);

            fs::create_directory("results");
            search_autofills(path, keyword);
            break;
        case 4:
            system("cls");
            std::cin.ignore();
            std::cout << "Enter the path: ";
            std::getline(std::cin, path);
            if (path.empty()) {
                std::cout << "Invalid path selected. Please try again." << std::endl;
                break;
            }

            fs::create_directory("results");
            search_credit_cards(path);
            break;
        case 5:
            system("cls");
            std::cin.ignore();
            std::cout << "Enter the path: ";
            std::getline(std::cin, path);
            if (path.empty()) {
                std::cout << "Invalid path selected. Please try again." << std::endl;
                break;
            }
      

            fs::create_directory("results");
            search_ftp_connections(path);
            break;
        case 6:
            system("cls");
            std::cin.ignore();
            std::cout << "Enter the path: ";
            std::getline(std::cin, path);
            if (path.empty()) {
                std::cout << "Invalid path selected. Please try again." << std::endl;
                break;
            }
        

            fs::create_directory("results");
            sort_files_by_date(path);
            break;
        case 7:
            system("cls");
            std::cin.ignore();
            std::cout << "Enter the path: ";
            std::getline(std::cin, path);
            if (path.empty()) {
                std::cout << "Invalid path selected. Please try again." << std::endl;
                break;
            }


            fs::create_directory("results");
            search_discord_tokens(path);
            break;
        case 0:
            std::cout << "Exiting the application." << std::endl;
            break;
        default:
            std::cout << "Invalid choice. Please try again." << std::endl;
            break;
        }

    } while (choice != 0);

    return 0;
}