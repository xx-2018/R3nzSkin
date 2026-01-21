#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <string>

// Anti-detection utilities for China server
namespace AntiDetection {
	// Check if Tencent TP is running
	inline bool CheckTPProcess() noexcept {
		HANDLE snapshot{ ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0) };
		if (snapshot == INVALID_HANDLE_VALUE)
			return false;

		PROCESSENTRY32W pe32{};
		pe32.dwSize = sizeof(PROCESSENTRY32W);

		// Known TP process names
		const wchar_t* tpProcesses[] = {
			L"TenProtect.exe",
			L"TPHelper.exe",
			L"TProtect.exe",
			L"TP.exe"
		};

		bool found{ false };
		if (::Process32FirstW(snapshot, &pe32)) {
			do {
				for (const auto* tpName : tpProcesses) {
					if (::wcscmp(pe32.szExeFile, tpName) == 0) {
						found = true;
						break;
					}
				}
			} while (::Process32NextW(snapshot, &pe32) && !found);
		}

		::CloseHandle(snapshot);
		return found;
	}

	// Check if debugger is present (multiple methods)
	inline bool IsDebuggerPresent() noexcept {
		// Method 1: Windows API
		if (::IsDebuggerPresent())
			return true;

		// Method 2: PEB check
		__try {
			BOOL debuggerPresent{ FALSE };
			::CheckRemoteDebuggerPresent(::GetCurrentProcess(), &debuggerPresent);
			if (debuggerPresent)
				return true;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return true;
		}

		// Method 3: NtQueryInformationProcess
		using NtQueryInformationProcessFunc = NTSTATUS(NTAPI*)(HANDLE, DWORD, PVOID, ULONG, PULONG);
		const auto ntdll{ ::GetModuleHandleW(L"ntdll.dll") };
		if (ntdll) {
			const auto NtQueryInformationProcess{ reinterpret_cast<NtQueryInformationProcessFunc>(::GetProcAddress(ntdll, "NtQueryInformationProcess")) };
			if (NtQueryInformationProcess) {
				DWORD debugPort{ 0 };
				if (NtQueryInformationProcess(::GetCurrentProcess(), 7, &debugPort, sizeof(debugPort), nullptr) >= 0) {
					if (debugPort != 0)
						return true;
				}
			}
		}

		return false;
	}

	// Simple VM detection
	inline bool IsVirtualMachine() noexcept {
		// Check for VMware (x86/x64 compatible)
		// MSVC x64 doesn't support inline asm, using intrinsics or just registry check
#if defined(_M_IX86)
		__try {
			__asm {
				push   edx
				push   ecx
				push   ebx

				mov    eax, 'VMXh'
				mov    ebx, 0
				mov    ecx, 10
				mov    edx, 'VX'
				
				in     eax, dx

				pop    ebx
				pop    ecx
				pop    edx
			}
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
#endif

		// Check registry for VM indicators
		HKEY hKey;
		const wchar_t* vmKeys[] = {
			L"HARDWARE\\DEVICEMAP\\Scsi\\Scsi Port 0\\Scsi Bus 0\\Target Id 0\\Logical Unit Id 0",
			L"HARDWARE\\Description\\System"
		};

		for (const auto* keyPath : vmKeys) {
			if (::RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
				wchar_t data[256]{};
				DWORD dataSize{ sizeof(data) };
				
				if (::RegQueryValueExW(hKey, L"Identifier", nullptr, nullptr, reinterpret_cast<LPBYTE>(data), &dataSize) == ERROR_SUCCESS) {
					const std::wstring identifier{ data };
					if (identifier.find(L"VBOX") != std::wstring::npos ||
					    identifier.find(L"VMware") != std::wstring::npos ||
					    identifier.find(L"Virtual") != std::wstring::npos) {
						::RegCloseKey(hKey);
						return true;
					}
				}
				::RegCloseKey(hKey);
			}
		}

		return false;
	}

	// Comprehensive environment check
	inline bool IsUnderMonitoring() noexcept {
		// Check TP process
		if (CheckTPProcess())
			return true;

		// Check debugger
		if (IsDebuggerPresent())
			return true;

		// Check VM (optional - may cause false positives)
		// if (IsVirtualMachine())
		//     return true;

		return false;
	}

	// Anti-sandbox delay (run before main initialization)
	inline void DelayExecution() noexcept {
		// Sleep with random jitter to avoid detection
		const auto start{ ::GetTickCount64() };
		::Sleep(100 + (start % 200)); // 100-300ms
		
		// Perform some calculations to look legitimate
		volatile std::uint64_t x{ start };
		for (std::uint32_t i = 0; i < 1000; ++i)
			x = (x * 13 + 7) % 1000000;
	}
}
