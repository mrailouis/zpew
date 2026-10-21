#include <zpew/io/sha256.hpp>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <random>
#include <span>
#include <string>
#include <string_view>

namespace {

std::string hex_of(std::string_view text) {
    return zpew::io::to_hex(zpew::io::sha256(text));
}

std::span<const std::byte> bytes_of(std::string_view text) {
    return std::as_bytes(std::span{text.data(), text.size()});
}

}

TEST_CASE("sha256 matches the FIPS 180-4 example vectors", "[io][sha256]") {
    REQUIRE(hex_of("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    REQUIRE(hex_of("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    REQUIRE(hex_of("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") ==
            "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    REQUIRE(hex_of("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmno"
                   "pqklmnopqrlmnopqrsmnopqrstnopqrstu") ==
            "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1");
}

TEST_CASE("sha256 of one million 'a' characters matches FIPS 180-4", "[io][sha256]") {
    const std::string million(1'000'000, 'a');
    REQUIRE(hex_of(million) == "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
}

TEST_CASE("sha256 is correct on both sides of every padding boundary", "[io][sha256]") {
    struct Vector {
        std::size_t length;
        std::string_view digest;
    };
    const auto vector =
        GENERATE(Vector{55, "9f4390f8d30c2dd92ec9f095b65e2b9ae9b0a925a5258e241c9f1e910f734318"},
                 Vector{56, "b35439a4ac6f0948b6d6f9e3c6af0f5f590ce20f1bde7090ef7970686ec6738a"},
                 Vector{57, "f13b2d724659eb3bf47f2dd6af1accc87b81f09f59f2b75e5c0bed6589dfe8c6"},
                 Vector{63, "7d3e74a05d7db15bce4ad9ec0658ea98e3f06eeecf16b4c6fff2da457ddc2f34"},
                 Vector{64, "ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb"},
                 Vector{65, "635361c48bb9eab14198e76ea8ab7f1a41685d6ad62aa9146d301d4f17eb0ae0"},
                 Vector{119, "31eba51c313a5c08226adf18d4a359cfdfd8d2e816b13f4af952f7ea6584dcfb"},
                 Vector{120, "2f3d335432c70b580af0e8e1b3674a7c020d683aa5f73aaaedfdc55af904c21c"},
                 Vector{127, "c57e9278af78fa3cab38667bef4ce29d783787a2f731d4e12200270f0c32320a"},
                 Vector{128, "6836cf13bac400e9105071cd6af47084dfacad4e5e302c94bfed24e013afb73e"});
    CAPTURE(vector.length);
    REQUIRE(hex_of(std::string(vector.length, 'a')) == vector.digest);
}

TEST_CASE("sha256 hashes every byte value", "[io][sha256]") {
    std::array<std::byte, 256> all_bytes{};
    for (std::size_t i = 0; i < all_bytes.size(); ++i) {
        all_bytes.at(i) = static_cast<std::byte>(i);
    }
    REQUIRE(zpew::io::to_hex(zpew::io::sha256(all_bytes)) ==
            "40aff2e9d2d8922e47afd4648e6967497158785fbd1da870e7110266bf944880");
}

TEST_CASE("incremental updates in random chunks give the one-shot digest", "[io][sha256]") {
    const std::string message(1000, 'z');
    const zpew::io::Sha256Digest expected = zpew::io::sha256(message);

    std::mt19937 rng{20261021U};
    std::uniform_int_distribution<std::size_t> chunk_size{0, 150};
    for (int trial = 0; trial < 50; ++trial) {
        zpew::io::Sha256 hasher;
        std::span<const std::byte> remaining = bytes_of(message);
        while (!remaining.empty()) {
            const std::size_t take = std::min(chunk_size(rng), remaining.size());
            hasher.update(remaining.first(take));
            remaining = remaining.subspan(take);
        }
        REQUIRE(hasher.digest() == expected);
    }
}

TEST_CASE("digest does not consume the hasher state", "[io][sha256]") {
    zpew::io::Sha256 hasher;
    hasher.update(bytes_of("ab"));
    const zpew::io::Sha256Digest partial = hasher.digest();
    REQUIRE(partial == zpew::io::sha256("ab"));
    hasher.update(bytes_of("c"));
    REQUIRE(hasher.digest() == zpew::io::sha256("abc"));
}
