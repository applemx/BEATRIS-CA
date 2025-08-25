#pragma once

#include <array>
#include <algorithm>
#include <cstdint>
#include <random>
#include <vector>
#include <iterator>

namespace ai::common {

// ------------------------------------------------------
// Seven‑bag RNG (stateful)
// ------------------------------------------------------
class SevenBag {
    std::array<char,7> bag_{'I','O','T','S','Z','J','L'};
    std::size_t index_ = 7;             // 7 == exhausted so we reshuffle on first call
    std::mt19937 rng_;
public:
    explicit SevenBag(std::uint32_t seed = std::random_device{}())
        : rng_(seed) {}

    /** Return the next tetromino ID (one of I,O,T,S,Z,J,L). */
    char next()
    {
        if(index_ >= 7) {               // bag empty → refill
            std::shuffle(bag_.begin(), bag_.end(), rng_);
            index_ = 0;
        }
        return bag_[index_++];
    }

    /** Fill *out* with exactly *N* pieces. */
    template<std::size_t N>
    std::array<char,N> nextN()
    {
        std::array<char,N> out{};
        for(std::size_t i=0;i<N;++i) out[i] = next();
        return out;
    }
};

// ------------------------------------------------------
// Convenience: generate *count* pieces into a std::vector
// ------------------------------------------------------
inline std::vector<char> generate_queue(std::size_t count,
                                        std::uint32_t seed = std::random_device{}())
{
    SevenBag bag(seed);
    std::vector<char> q;
    q.reserve(count);
    for(std::size_t i=0;i<count;++i) q.push_back(bag.next());
    return q;
}

} // namespace ai::common
