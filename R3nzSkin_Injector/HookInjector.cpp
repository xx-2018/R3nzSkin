#define _CRT_SECURE_NO_WARNINGS
#include "HookInjector.hpp"
#include "xorstr.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
// #include <msclr/marshal_cppstd.h> // Removed: native compilation only

// Define macro if not present, but Injector.cpp suggests _XorStrW is available.
// We'll use the same pattern as Injector.cpp: static_cast<std::wstring>(_XorStrW(L"..."))

// Dummy implementations to satisfy linker if referenced locally
// (The actual injection uses the function exported from the DLL)
LRESULT CALLBACK HookInjector::CBTProc(int nCode, WPARAM wParam, LPARAM lParam) {
	return ::CallNextHookEx(nullptr, nCode, wParam, lParam);
}

LRESULT CALLBACK HookInjector::GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam) {
	return ::CallNextHookEx(nullptr, nCode, wParam, lParam);
}

// Helper struct for EnumWindows
struct WindowFinder {
	struct EnumData {
		DWORD pid;
		HWND hWnd;
	};

	static BOOL CALLBACK EnumProc(HWND hWnd, LPARAM lParam) {
		auto* pData = reinterpret_cast<EnumData*>(lParam);
		DWORD pid = 0;
		::GetWindowThreadProcessId(hWnd, &pid);
		if (pid == pData->pid) {
			wchar_t className[256];
			if (::GetClassNameW(hWnd, className, 256)) {
				// Decrypt "RiotWindowClass"
				// Injector.cpp uses: static_cast<std::wstring>(_XorStrW(x))
				// We'll use that pattern.
				if (::wcscmp(className, static_cast<std::wstring>(_XorStrW(L"RiotWindowClass")).c_str()) == 0) {
					pData->hWnd = hWnd;
					return FALSE;
				}
			}
		}
		return TRUE;
	}
};

HWND HookInjector::FindGameWindow(DWORD processId) noexcept {
	WindowFinder::EnumData data = { processId, nullptr };
	::EnumWindows(WindowFinder::EnumProc, reinterpret_cast<LPARAM>(&data));
	return data.hWnd;
}

DWORD HookInjector::GetWindowThreadId(HWND window) noexcept {
	return ::GetWindowThreadProcessId(window, nullptr);
}

bool HookInjector::InjectViaCBT(DWORD processId, const std::wstring& dllPath) noexcept {
	const auto hGameWindow{ FindGameWindow(processId) };
	if (!hGameWindow) return false;

	const auto threadId{ GetWindowThreadId(hGameWindow) };
	if (!threadId) return false;

	// Load DLL into our process first to get the export address
	const auto hDll{ ::LoadLibraryW(dllPath.c_str()) };
	if (!hDll) return false;

	// Get address of CBTProc
	// We use "CBTProc" plain text here as it's an export name found in PE headers anyway
	const auto hProc{ reinterpret_cast<HOOKPROC>(::GetProcAddress(hDll, "CBTProc")) };
	if (!hProc) {
		::FreeLibrary(hDll);
		return false;
	}

	// Install global hook (WH_CBT) targeting the specific thread
	const auto hHook{ ::SetWindowsHookExW(WH_CBT, hProc, hDll, threadId) };
	if (!hHook) {
		::FreeLibrary(hDll);
		return false;
	}

	// Trigger the hook by sending a null message
	::SendMessageW(hGameWindow, WM_NULL, 0, 0);
	
	// Wait a bit for the injection to propagate
	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	// Uninstall hook and free our local library handle
	::UnhookWindowsHookEx(hHook);
	::FreeLibrary(hDll);

	return true;
}

bool HookInjector::InjectViaGetMessage(DWORD processId, const std::wstring& dllPath) noexcept {
	const auto hGameWindow{ FindGameWindow(processId) };
	if (!hGameWindow) return false;

	const auto threadId{ GetWindowThreadId(hGameWindow) };
	if (!threadId) return false;

	const auto hDll{ ::LoadLibraryW(dllPath.c_str()) };
	if (!hDll) return false;

	const auto hProc{ reinterpret_cast<HOOKPROC>(::GetProcAddress(hDll, "GetMsgProc")) };
	if (!hProc) {
		::FreeLibrary(hDll);
		return false;
	}

	const auto hHook{ ::SetWindowsHookExW(WH_GETMESSAGE, hProc, hDll, threadId) };
	if (!hHook) {
		::FreeLibrary(hDll);
		return false;
	}

	// Trigger via PostMessage for GetMessage hook
	::PostMessageW(hGameWindow, WM_NULL, 0, 0);
	
	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	::UnhookWindowsHookEx(hHook);
	::FreeLibrary(hDll);

	return true;
}

// Unused helper
bool HookInjector::TriggerHook(HWND window) noexcept {
    return ::SendMessageW(window, WM_NULL, 0, 0) != 0;
}
