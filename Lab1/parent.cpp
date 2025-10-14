#include <windows.h>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

#define BUFFER_SIZE 256

int main() {
    HANDLE hPipe1Read, hPipe1Write;
    HANDLE hPipe2Read, hPipe2Write;
    PROCESS_INFORMATION pi;
    STARTUPINFOA si;
    SECURITY_ATTRIBUTES sa;
    std::string filename;

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hPipe1Read, &hPipe1Write, &sa, 0) ||
        !CreatePipe(&hPipe2Read, &hPipe2Write, &sa, 0)) {
        std::cerr << "CreatePipe failed: " << GetLastError() << std::endl;
        return 1;
    }

    SetHandleInformation(hPipe1Write, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hPipe2Read, HANDLE_FLAG_INHERIT, 0);

    std::cout << "Enter the file name: ";
    std::getline(std::cin, filename);

    ZeroMemory(&si, sizeof(STARTUPINFOA));
    si.cb = sizeof(STARTUPINFOA);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hPipe1Read;
    si.hStdOutput = hPipe2Write;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    std::string commandLine = "child.exe " + filename;

    std::vector<char> cmdLineBuf(commandLine.begin(), commandLine.end());
    cmdLineBuf.push_back('\0');

    if (!CreateProcessA(
        NULL,
        cmdLineBuf.data(),
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &si,
        &pi)) {
        std::cerr << "CreateProcess failed: " << GetLastError() << std::endl;
        CloseHandle(hPipe1Read);
        CloseHandle(hPipe1Write);
        CloseHandle(hPipe2Read);
        CloseHandle(hPipe2Write);
        return 1;
    }

    CloseHandle(hPipe1Read);
    CloseHandle(hPipe2Write);
    CloseHandle(pi.hThread);

    Sleep(100);
    
    std::cout << "Enter commands (numbers). To exit, enter 'exit':" << std::endl;
    
    char responseBuffer[BUFFER_SIZE];

    while (true) {
        std::cout << "> ";
        std::string input;
        std::getline(std::cin, input);

        if (input == "exit") {
            break;
        }

        if (input.empty()) {
            continue;
        }

        input += "\n";
        DWORD written;
        WriteFile(hPipe1Write, input.c_str(), input.length(), &written, NULL);
        
        Sleep(50);
        
        DWORD bytesRead;
        std::string response;
        
        while (true) {
            if (ReadFile(hPipe2Read, responseBuffer, BUFFER_SIZE - 1, &bytesRead, NULL) && bytesRead > 0) {
                responseBuffer[bytesRead] = '\0';
                response += responseBuffer;
                if (response.find('\n') != std::string::npos) {
                    break;
                }
            } else {
                break;
            }
            Sleep(10);
        }
        
        if (!response.empty()) {
            std::cout << "Answer: " << response;
        }
    }

    CloseHandle(hPipe1Write);
    CloseHandle(hPipe2Read);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    
    std::cout << "The parent process is completed." << std::endl;
    
    return 0;
}