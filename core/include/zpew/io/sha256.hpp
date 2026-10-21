#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace zpew::io {

inline constexpr std::size_t kSha256DigestSize = 32;
inline constexpr std::size_t kSha256BlockSize = 64;

using Sha256Digest = std::array<std::uint8_t, kSha256DigestSize>;

class Sha256 {
public:
    void update(std::span<const std::byte> data) noexcept;
    [[nodiscard]] Sha256Digest digest() const noexcept;

private:
    void process_block(std::span<const std::byte, kSha256BlockSize> block) noexcept;
    void absorb(std::span<const std::byte> data) noexcept;

    std::array<std::uint32_t, 8> state_{0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
                                        0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U};
    std::array<std::byte, kSha256BlockSize> buffer_{};
    std::size_t buffered_ = 0;
    std::uint64_t message_bytes_ = 0;
};

[[nodiscard]] Sha256Digest sha256(std::span<const std::byte> data) noexcept;
[[nodiscard]] Sha256Digest sha256(std::string_view text) noexcept;
[[nodiscard]] std::string to_hex(const Sha256Digest& digest);

}
