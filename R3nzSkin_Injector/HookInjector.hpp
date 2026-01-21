#pragma once

#include <Windows.h>
#include <cstdint>
#include <string>

// New injector using SetWindowsHookEx for stealth
// More difficult to detect than CreateRemoteThread

class HookInjector {
public:
	// Inject DLL using WH_CBT hook
	static bool InjectViaCBT(DWORD processId, const std::wstring& dllPath) noexcept;
	
	// Inject DLL using WH_GETMESSAGE hook
	static bool InjectViaGetMessage(DWORD processId, const std::wstring& dllPath) noexcept;
	
	// Find game window by process ID
	static HWND FindGameWindow(DWORD processId) noexcept;
	
	// Get thread ID of window
	static DWORD GetWindowThreadId(HWND window) noexcept;

private:
	// Hook procedures (must be exported from DLL)
	static LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam);
	
	// Helper to trigger hook
	static bool TriggerHook(HWND window) noexcept;
};
