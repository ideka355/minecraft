// Injector for the cheat DLL into Minecraft.Windows.exe.
//
// Minecraft Bedrock for Windows runs as a sandboxed UWP (AppContainer) process.
// Two things a plain injector needs beyond normal CreateRemoteThread injection:
//   1. Windows Developer Mode must be enabled on the machine (Settings > Privacy & security
//      > For developers) so the OS allows attaching to / debugging packaged apps at all.
//   2. The DLL file's ACL must grant read+execute to "ALL APPLICATION PACKAGES" (and
//      "ALL RESTRICTED APPLICATION PACKAGES"), because an AppContainer process refuses to
//      LoadLibrary a file its sandbox token has no access to. We do this here with
//      SetNamedSecurityInfoW instead of shelling out to icacls.
//
// Run this injector elevated (as Administrator).

#include <windows.h>
#include <tlhelp32.h>
#include <aclapi.h>
#include <sddl.h>
#include <iostream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

constexpr wchar_t kTargetProcessName[] = L"Minecraft.Windows.exe";
constexpr wchar_t kDllName[] = L"cheat.dll";

DWORD FindProcessId(const wchar_t* processName) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    DWORD pid = 0;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, processName) == 0) {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return pid;
}

// Grants read+execute on the DLL to ALL_APPLICATION_PACKAGES / ALL_RESTRICTED_APPLICATION_PACKAGES
// so the sandboxed (AppContainer) target process is permitted to LoadLibrary it.
bool GrantAppContainerAccess(const std::wstring& path) {
    bool ok = true;

    for (const wchar_t* sidString : {L"S-1-15-2-1" /* ALL_APPLICATION_PACKAGES */,
                                      L"S-1-15-2-2" /* ALL_RESTRICTED_APPLICATION_PACKAGES */}) {
        PSID sid = nullptr;
        if (!ConvertStringSidToSidW(sidString, &sid)) {
            ok = false;
            continue;
        }

        EXPLICIT_ACCESSW ea{};
        ea.grfAccessPermissions = GENERIC_READ | GENERIC_EXECUTE;
        ea.grfAccessMode = GRANT_ACCESS;
        ea.grfInheritance = NO_INHERITANCE;
        ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea.Trustee.TrusteeType = TRUSTEE_IS_WELLKNOWN_GROUP;
        ea.Trustee.ptstrName = reinterpret_cast<LPWSTR>(sid);

        PACL existingAcl = nullptr;
        PSECURITY_DESCRIPTOR sd = nullptr;
        DWORD res = GetNamedSecurityInfoW(path.c_str(), SE_FILE_OBJECT,
                                           DACL_SECURITY_INFORMATION, nullptr, nullptr,
                                           &existingAcl, nullptr, &sd);
        if (res != ERROR_SUCCESS) {
            LocalFree(sid);
            ok = false;
            continue;
        }

        PACL newAcl = nullptr;
        res = SetEntriesInAclW(1, &ea, existingAcl, &newAcl);
        if (res == ERROR_SUCCESS && newAcl) {
            res = SetNamedSecurityInfoW(const_cast<LPWSTR>(path.c_str()), SE_FILE_OBJECT,
                                         DACL_SECURITY_INFORMATION, nullptr, nullptr, newAcl,
                                         nullptr);
            LocalFree(newAcl);
            if (res != ERROR_SUCCESS) {
                ok = false;
            }
        } else {
            ok = false;
        }

        LocalFree(sid);
        LocalFree(sd);
    }

    return ok;
}

bool InjectDll(DWORD pid, const std::wstring& dllPath) {
    HANDLE process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
                                      PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
                                  FALSE, pid);
    if (!process) {
        std::wcerr << L"OpenProcess failed (are you running as Administrator?), error "
                   << GetLastError() << L"\n";
        return false;
    }

    const size_t pathBytes = (dllPath.size() + 1) * sizeof(wchar_t);
    LPVOID remotePath =
        VirtualAllocEx(process, nullptr, pathBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remotePath) {
        std::wcerr << L"VirtualAllocEx failed, error " << GetLastError() << L"\n";
        CloseHandle(process);
        return false;
    }

    if (!WriteProcessMemory(process, remotePath, dllPath.c_str(), pathBytes, nullptr)) {
        std::wcerr << L"WriteProcessMemory failed, error " << GetLastError() << L"\n";
        VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }

    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    auto loadLibraryW =
        reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(kernel32, "LoadLibraryW"));

    HANDLE thread =
        CreateRemoteThread(process, nullptr, 0, loadLibraryW, remotePath, 0, nullptr);
    if (!thread) {
        std::wcerr << L"CreateRemoteThread failed, error " << GetLastError() << L"\n";
        VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }

    WaitForSingleObject(thread, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeThread(thread, &exitCode);

    CloseHandle(thread);
    VirtualFreeEx(process, remotePath, 0, MEM_RELEASE);
    CloseHandle(process);

    if (exitCode == 0) {
        std::wcerr << L"LoadLibraryW returned NULL in the target process - injection failed.\n";
        return false;
    }

    return true;
}

}  // namespace

int wmain() {
    wchar_t exePathBuf[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exePathBuf, MAX_PATH);
    fs::path dllPath = fs::path(exePathBuf).parent_path() / kDllName;
    if (!fs::exists(dllPath)) {
        std::wcerr << L"Could not find " << kDllName << L" next to the injector.\n";
        return 1;
    }

    std::wcout << L"Waiting for " << kTargetProcessName << L"...\n";

    DWORD pid = 0;
    for (int attempt = 0; attempt < 60 && pid == 0; ++attempt) {
        pid = FindProcessId(kTargetProcessName);
        if (pid == 0) {
            Sleep(1000);
        }
    }

    if (pid == 0) {
        std::wcerr << L"Timed out waiting for " << kTargetProcessName << L".\n";
        return 1;
    }

    std::wcout << L"Found process, pid=" << pid << L"\n";

    if (!GrantAppContainerAccess(dllPath.wstring())) {
        std::wcerr << L"Warning: failed to grant AppContainer access to the DLL. "
                       L"Injection into the sandboxed process may fail. Make sure the injector "
                       L"is running as Administrator.\n";
    }

    if (!InjectDll(pid, dllPath.wstring())) {
        return 1;
    }

    std::wcout << L"Injected successfully.\n";
    return 0;
}
