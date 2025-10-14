#include <windows.h>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>

bool is_prime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) {
            return false;
        }
    }
    return true;
}


int main(int argc, char *argv[]) {
    std::string filename;
    
    if (argc != 2) {
        std::cerr << "Usage: child <filename>" << std::endl;
        return 1;
    }
    
    filename = argv[1];

    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return 1;
    }
    
    std::cout << "The child process is running. The file has been created." << std::endl;
    std::cout.flush();
    
    while (true) {
        std::string input;
        std::getline(std::cin, input);
        
        if (std::cin.eof()) {
            break;
        }

        if (input.empty()) {
            continue;
        }

        std::stringstream ss(input);
        long number;
        ss >> number;
        
        if (ss.fail() || !ss.eof()) {
            std::cout << "Error: enter an integer" << std::endl;
            continue;
        }
        
        if (number < 0) {
            std::cout << "A negative number. Completion of work." << std::endl;
            break;
        }
        
        if (is_prime(static_cast<int>(number))) {
            std::cout << "Number " << number << " prime" << std::endl;
        } else {
            file << number << std::endl;
            file.flush();
            std::cout << "Number " << number << " composite - written to a file" << std::endl;
        }
        
        std::cout.flush();
    }
    
    file.close();
    return 0;
}