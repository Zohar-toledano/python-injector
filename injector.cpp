#include <windows.h>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <PID> <DLL_PATH>" << std::endl;
        return 1;
    }

    DWORD pid = atoi(argv[1]);
    const char* dllPath = argv[2];
    HMODULE hmod;
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        std::cerr << "OpenProcess failed: " << GetLastError() << std::endl;
        return 1;
    }

    LPVOID remotePath = VirtualAllocEx(hProcess, NULL, strlen(dllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
    if (!remotePath) {
        std::cerr << "VirtualAllocEx failed: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return 1;
    }

    if (!WriteProcessMemory(hProcess, remotePath, dllPath, strlen(dllPath) + 1, NULL)) {
        std::cerr << "WriteProcessMemory failed: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    LPVOID loadLibraryAddr = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, 
        (LPTHREAD_START_ROUTINE)loadLibraryAddr, remotePath, 0, NULL);
    if (!hThread) {
        std::cerr << "CreateRemoteThread failed: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    WaitForSingleObject(hThread, INFINITE);
    GetExitCodeThread(hThread, (LPDWORD)&hmod);

    LPVOID freeLibAddr = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "FreeLibrary");
    HANDLE hThread2 = CreateRemoteThread(hProcess, nullptr, 0,
                                      (LPTHREAD_START_ROUTINE)freeLibAddr,
                                      (LPVOID)hmod, 0, nullptr);
    
    WaitForSingleObject(hThread, INFINITE);
    
    VirtualFreeEx(hProcess, remotePath, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hThread2);

    CloseHandle(hProcess);

    std::cout << "DLL injected successfully." << std::endl;
    return 0;
}