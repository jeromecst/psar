#pragma once

#include <cstdint>
#include <limits>
#include <random>

namespace psar {

class Random {
  public:
	using uint64_t = std::uint64_t;
	using uint32_t = std::uint32_t;

	// UniformRandomBitGenerator
	using result_type = uint64_t;
	constexpr result_type operator()() { return generate_u64(); }
	static constexpr result_type min() {
		return std::numeric_limits<result_type>::min();
	}
	static constexpr result_type max() {
		return std::numeric_limits<result_type>::max();
	}

	constexpr explicit Random(uint64_t seed) { init(seed); }

	Random() {
		std::random_device device;
		init(uint64_t(device()) << 32 | device());
	}

	constexpr void init(uint64_t seed) {
		splitmix64 smstate{seed};
		for (auto &x : s)
			x = smstate.generate();
	}

	// xoshiro256**
	constexpr uint64_t generate_u64() {
		uint64_t const result = rol64(s[1] * 5, 7) * 9;
		uint64_t const t = s[1] << 17;

		s[2] ^= s[0];
		s[3] ^= s[1];
		s[1] ^= s[2];
		s[0] ^= s[3];

		s[2] ^= t;
		s[3] = rol64(s[3], 45);

		return result;
	}

	constexpr uint32_t generate_u32() { return generate_u64() >> 32; }
	constexpr bool generate_bool() { return generate_u64() & (1ul << 63); }

  private:
	static constexpr uint64_t rol64(uint64_t x, int k) {
		return (x << k) | (x >> (64 - k));
	}

	struct splitmix64 {
		constexpr uint64_t generate() {
			uint64_t result = (s += 0x9E3779B97f4A7C15);
			result = (result ^ (result >> 30)) * 0xBF58476D1CE4E5B9;
			result = (result ^ (result >> 27)) * 0x94D049BB133111EB;
			return result ^ (result >> 31);
		}

		uint64_t s{};
	};

	uint64_t s[4]{};
};

} // namespace psar
