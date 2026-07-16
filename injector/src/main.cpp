// Single-file loader for the cheat: everything (the DLL and a default config) is embedded
// in this executable as resources (see injector/CMakeLists.txt, which builds cheat.dll and
// embeds it before compiling this file). At startup we extract the DLL to a per-run temp
// directory and inject it into Minecraft.Windows.exe from there -- nothing needs to be
// shipped alongside this .exe.
//
// Minecraft Bedrock for Windows runs as a sandboxed UWP (AppContainer) process.
// Two things a plain injector needs beyond normal CreateRemoteThread injection:
//   1. Windows Developer Mode must be enabled on the machine (Settings > Privacy & security
//      > For developers) so the OS allows attaching to / debugging packaged apps at all.
//      We enable this ourselves via its underlying registry value rather than requiring the
//      user to find the Settings toggle.
//   2. The DLL file's ACL must grant read+execute to "ALL APPLICATION PACKAGES" (and
//      "ALL RESTRICTED APPLICATION PACKAGES"), because an AppContainer process refuses to
//      LoadLibrary a file its sandbox token has no access to. We do this here with
//      SetNamedSecurityInfoW instead of shelling out to icacls.
//
// This loader self-elevates (relaunches itself via UAC) if it isn't already running as
// Administrator, so double-clicking it is enough -- no "Run as administrator" required.

#include <windows.h>
#include <tlhelp32.h>
#include <accctrl.h>
#include <aclapi.h>
#include <sddl.h>
#include <shellapi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

constexpr wchar_t kTargetProcessName[] = L"Minecraft.Windows.exe";

// Names of the RCDATA resources embedded by injector/CMakeLists.txt.
constexpr wchar_t kCheatDllResource[] = L"CHEATDLL";
constexpr wchar_t kOffsetsDefaultResource[] = L"OFFSETSDEFAULT";

bool IsElevated() {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        return false;
    }

    TOKEN_ELEVATION elevation{};
    DWORD size = sizeof(elevation);
    bool elevated = false;
    if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)) {
        elevated = elevation.TokenIsElevated != 0;
    }

    CloseHandle(token);
    return elevated;
}

// Relaunches this exe with a UAC elevation prompt. Returns false only if the relaunch
// itself couldn't be started (e.g. the user cancelled the UAC prompt).
bool RelaunchElevated() {
    wchar_t exePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    SHELLEXECUTEINFOW sei{};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"runas";
    sei.lpFile = exePath;
    sei.nShow = SW_NORMAL;

    if (!ShellExecuteExW(&sei)) {
        return false;
    }
    if (sei.hProcess) {
        CloseHandle(sei.hProcess);
    }
    return true;
}

// Windows Developer Mode is controlled by a single registry value -- this is exactly what
// the Settings > For developers toggle flips. Setting it ourselves (we're elevated by this
// point) means the user never has to find that setting manually. Requires no reboot.
void EnsureDeveloperModeEnabled() {
    HKEY key = nullptr;
    LONG openResult = RegCreateKeyExW(
        HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock", 0,
        nullptr, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, nullptr, &key, nullptr);
    if (openResult != ERROR_SUCCESS) {
        std::wcerr << L"Could not access the Developer Mode registry key (error " << openResult
                   << L") -- enable it manually via Settings > Privacy & security > For "
                      L"developers if injection fails.\n";
        return;
    }

    DWORD value = 0;
    DWORD size = sizeof(value);
    DWORD type = 0;
    LONG queryResult = RegQueryValueExW(key, L"AllowDevelopmentWithoutDevLicense", nullptr,
                                         &type, reinterpret_cast<LPBYTE>(&value), &size);

    if (queryResult != ERROR_SUCCESS || value == 0) {
        DWORD enabled = 1;
        LONG setResult =
            RegSetValueExW(key, L"AllowDevelopmentWithoutDevLicense", 0, REG_DWORD,
                            reinterpret_cast<const BYTE*>(&enabled), sizeof(enabled));
        if (setResult == ERROR_SUCCESS) {
            std::wcout << L"Enabled Windows Developer Mode.\n";
        } else {
            std::wcerr << L"Failed to enable Developer Mode automatically (error " << setResult
                       << L") -- enable it manually via Settings > Privacy & security > For "
                          L"developers if injection fails.\n";
        }
    }

    RegCloseKey(key);
}

// Writes an embedded RCDATA resource out to `destPath`. Returns false if the resource is
// missing or the file couldn't be written.
bool ExtractResource(const wchar_t* resourceName, const fs::path& destPath) {
    HMODULE self = GetModuleHandleW(nullptr);
    HRSRC res = FindResourceW(self, resourceName, RT_RCDATA);
    if (!res) {
        std::wcerr << L"Missing embedded resource " << resourceName << L"\n";
        return false;
    }

    HGLOBAL data = LoadResource(self, res);
    if (!data) {
        return false;
    }

    void* bytes = LockResource(data);
    DWORD size = SizeofResource(self, res);
    if (!bytes || size == 0) {
        return false;
    }

    HANDLE file = CreateFileW(destPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD written = 0;
    bool ok = WriteFile(file, bytes, size, &written, nullptr) && written == size;
    CloseHandle(file);
    return ok;
}

// The user-editable offsets.json lives next to this .exe and persists across runs/updates.
// On first launch it's materialized from the embedded default (all zeros).
fs::path EnsurePersistentOffsetsConfig(const fs::path& exeDir) {
    fs::path path = exeDir / L"offsets.json";
    if (!fs::exists(path)) {
        std::wcout << L"No offsets.json next to the exe - creating one from the built-in "
                       L"template. Fill it in before enabling movement modules.\n";
        ExtractResource(kOffsetsDefaultResource, path);
    }
    return path;
}

fs::path CreateWorkDir() {
    wchar_t tempDir[MAX_PATH]{};
    GetTempPathW(MAX_PATH, tempDir);

    fs::path workDir = fs::path(tempDir) / (L"mcbc-" + std::to_wstring(GetCurrentProcessId()));
    fs::create_directories(workDir);
    return workDir;
}

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
        ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
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
    if (!IsElevated()) {
        std::wcout << L"Not running elevated -- requesting administrator privileges...\n";
        if (!RelaunchElevated()) {
            std::wcerr << L"Elevation was cancelled or failed. This loader needs to run as "
                          L"Administrator to inject into Minecraft.\n";
            return 1;
        }
        return 0;  // The elevated relaunch continues in a new process/console.
    }

    EnsureDeveloperModeEnabled();

    wchar_t exePathBuf[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exePathBuf, MAX_PATH);
    fs::path exeDir = fs::path(exePathBuf).parent_path();

    fs::path persistentOffsets = EnsurePersistentOffsetsConfig(exeDir);

    fs::path workDir = CreateWorkDir();
    fs::path dllPath = workDir / L"cheat.dll";

    if (!ExtractResource(kCheatDllResource, dllPath)) {
        std::wcerr << L"Failed to extract the embedded cheat DLL.\n";
        return 1;
    }

    // Copy the (user-editable) offsets.json into the same directory as the extracted DLL --
    // the DLL looks for it next to itself, wherever that ends up being.
    std::error_code ec;
    fs::copy_file(persistentOffsets, workDir / L"offsets.json",
                   fs::copy_options::overwrite_existing, ec);

    // Leave a marker pointing back at the persistent (exe-adjacent) offsets.json, in UTF-8,
    // so the offset auto-finder inside the DLL can save its results somewhere that
    // survives after this run's temp directory is gone.
    std::ofstream configPointer(workDir / L"config_path.txt", std::ios::binary);
    if (configPointer.is_open()) {
        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, persistentOffsets.c_str(), -1, nullptr,
                                           0, nullptr, nullptr);
        std::string utf8Path(utf8Len > 0 ? utf8Len - 1 : 0, '\0');
        if (utf8Len > 0) {
            WideCharToMultiByte(CP_UTF8, 0, persistentOffsets.c_str(), -1, utf8Path.data(),
                                 utf8Len, nullptr, nullptr);
        }
        configPointer << utf8Path;
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
                       L"Injection into the sandboxed process may fail. Make sure this "
                       L"loader is running as Administrator.\n";
    }

    if (!InjectDll(pid, dllPath.wstring())) {
        return 1;
    }

    std::wcout << L"Injected successfully.\n";
    return 0;
}
