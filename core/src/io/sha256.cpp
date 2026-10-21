#include <zpew/io/sha256.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace zpew::io {

namespace {

constexpr std::array<std::uint32_t, 64> kRoundConstants{
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U,
    0xab1c5ed5U, 0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU,
    0x9bdc06a7U, 0xc19bf174U, 0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU,
    0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU, 0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U, 0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU,
    0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U, 0xa2bfe8a1U, 0xa81a664bU,
    0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U, 0x19a4c116U,
    0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U,
    0xc67178f2U};

constexpr std::size_t kLengthFieldSize = 8;
constexpr std::size_t kBitsPerByte = 8;
constexpr std::byte kPaddingMarker{0x80};

std::uint32_t load_big_endian(std::span<const std::byte, 4> bytes) noexcept {
    std::uint32_t value = 0;
    for (const std::byte b : bytes) {
        value = (value << kBitsPerByte) | std::to_integer<std::uint32_t>(b);
    }
    return value;
}

std::uint32_t small_sigma0(std::uint32_t x) noexcept {
    return std::rotr(x, 7) ^ std::rotr(x, 18) ^ (x >> 3U);
}

std::uint32_t small_sigma1(std::uint32_t x) noexcept {
    return std::rotr(x, 17) ^ std::rotr(x, 19) ^ (x >> 10U);
}

std::uint32_t big_sigma0(std::uint32_t x) noexcept {
    return std::rotr(x, 2) ^ std::rotr(x, 13) ^ std::rotr(x, 22);
}

std::uint32_t big_sigma1(std::uint32_t x) noexcept {
    return std::rotr(x, 6) ^ std::rotr(x, 11) ^ std::rotr(x, 25);
}

std::uint32_t choose(std::uint32_t x, std::uint32_t y, std::uint32_t z) noexcept {
    return (x & y) ^ (~x & z);
}

std::uint32_t majority(std::uint32_t x, std::uint32_t y, std::uint32_t z) noexcept {
    return (x & y) ^ (x & z) ^ (y & z);
}

}

void Sha256::process_block(std::span<const std::byte, kSha256BlockSize> block) noexcept {
    std::array<std::uint32_t, 64> schedule{};
    for (std::size_t i = 0; i < 16; ++i) {
        schedule[i] = load_big_endian(block.subspan(i * 4).first<4>());
    }
    for (std::size_t i = 16; i < schedule.size(); ++i) {
        schedule[i] = small_sigma1(schedule[i - 2]) + schedule[i - 7] +
                      small_sigma0(schedule[i - 15]) + schedule[i - 16];
    }

    auto [a, b, c, d, e, f, g, h] = state_;
    for (std::size_t i = 0; i < schedule.size(); ++i) {
        const std::uint32_t t1 =
            h + big_sigma1(e) + choose(e, f, g) + kRoundConstants[i] + schedule[i];
        const std::uint32_t t2 = big_sigma0(a) + majority(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    const std::array<std::uint32_t, 8> working{a, b, c, d, e, f, g, h};
    for (std::size_t i = 0; i < state_.size(); ++i) {
        state_[i] += working[i];
    }
}

void Sha256::absorb(std::span<const std::byte> data) noexcept {
    while (!data.empty()) {
        const std::size_t take = std::min(kSha256BlockSize - buffered_, data.size());
        std::ranges::copy(data.first(take), std::span{buffer_}.subspan(buffered_).begin());
        buffered_ += take;
        data = data.subspan(take);
        if (buffered_ == kSha256BlockSize) {
            process_block(buffer_);
            buffered_ = 0;
        }
    }
}

void Sha256::update(std::span<const std::byte> data) noexcept {
    message_bytes_ += data.size();
    absorb(data);
}

Sha256Digest Sha256::digest() const noexcept {
    Sha256 finished = *this;
    const std::uint64_t message_bits = message_bytes_ * kBitsPerByte;

    const std::array<std::byte, 1> marker{kPaddingMarker};
    finished.absorb(marker);
    const std::array<std::byte, kSha256BlockSize> zeros{};
    const std::size_t free_bytes = kSha256BlockSize - finished.buffered_;
    const std::size_t zero_count = free_bytes >= kLengthFieldSize
                                       ? free_bytes - kLengthFieldSize
                                       : free_bytes + kSha256BlockSize - kLengthFieldSize;
    finished.absorb(std::span{zeros}.first(zero_count));

    std::array<std::byte, kLengthFieldSize> length{};
    for (std::size_t i = 0; i < length.size(); ++i) {
        const auto shift = static_cast<unsigned>((length.size() - 1 - i) * kBitsPerByte);
        length[i] = static_cast<std::byte>((message_bits >> shift) & 0xffU);
    }
    finished.absorb(length);

    Sha256Digest digest{};
    for (std::size_t i = 0; i < finished.state_.size(); ++i) {
        for (std::size_t j = 0; j < 4; ++j) {
            const auto shift = static_cast<unsigned>((3 - j) * kBitsPerByte);
            digest[(i * 4) + j] = static_cast<std::uint8_t>((finished.state_[i] >> shift) & 0xffU);
        }
    }
    return digest;
}

Sha256Digest sha256(std::span<const std::byte> data) noexcept {
    Sha256 hasher;
    hasher.update(data);
    return hasher.digest();
}

Sha256Digest sha256(std::string_view text) noexcept {
    return sha256(std::as_bytes(std::span{text.data(), text.size()}));
}

std::string to_hex(const Sha256Digest& digest) {
    constexpr std::string_view kHexDigits = "0123456789abcdef";
    constexpr unsigned kNibbleBits = 4;
    constexpr unsigned kNibbleMask = 0x0fU;
    std::string hex;
    hex.reserve(digest.size() * 2);
    for (const std::uint8_t byte : digest) {
        const auto value = static_cast<unsigned>(byte);
        hex.push_back(kHexDigits[(value >> kNibbleBits) & kNibbleMask]);
        hex.push_back(kHexDigits[value & kNibbleMask]);
    }
    return hex;
}

}
