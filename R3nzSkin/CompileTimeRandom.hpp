#pragma once

#include <cstdint>
#include <chrono>
#include <thread>

// Compile-time randomization for anti-detection
// Generates different binary signatures on each compilation

namespace AntiDetect {
	// Generate seed from __TIME__ macro
	constexpr std::uint32_t compile_time_seed() noexcept {
		return (__TIME__[7] - '0') * 1 +
		       (__TIME__[6] - '0') * 10 +
		       (__TIME__[4] - '0') * 60 +
		       (__TIME__[3] - '0') * 600 +
		       (__TIME__[1] - '0') * 3600 +
		       (__TIME__[0] - '0') * 36000;
	}

	// Generate compile-time random ID
	constexpr std::uint32_t random_id(std::uint32_t line) noexcept {
		return compile_time_seed() ^ line;
	}

	// Runtime random delay for anti-sandbox
	inline void random_delay(std::uint32_t seed) noexcept {
		const auto delay_ms{ (seed % 500) + 100 }; // 100-600ms
		std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
	}

	// XOR encryption for runtime strings
	template<std::size_t N>
	class EncryptedString {
	public:
		constexpr EncryptedString(const char(&str)[N]) noexcept : key{ static_cast<char>(compile_time_seed() & 0xFF) } {
			for (std::size_t i = 0; i < N; ++i)
				encrypted[i] = str[i] ^ key;
		}

		inline const char* decrypt() noexcept {
			for (std::size_t i = 0; i < N; ++i)
				decrypted[i] = encrypted[i] ^ key;
			return decrypted;
		}

	private:
		char encrypted[N]{};
		char decrypted[N]{};
		char key;
	};
}

// Macros for easy usage
#define RANDOM_ID AntiDetect::random_id(__LINE__)
#define RANDOM_DELAY() AntiDetect::random_delay(RANDOM_ID)
#define ENCRYPT_STRING(str) AntiDetect::EncryptedString(str).decrypt()
