#pragma once

#include <cstdint>
#include <utility>
#include <stdexcept>

template <typename ValueT, std::size_t N, bool SafeMode = true>
class static_ring_buffer {
    static_assert(N > 0, "effective size of ring buffer must be > 0");

   public:
    static_ring_buffer()
        : wr_p_{reinterpret_cast<ValueT*>(buffer_)}, rd_p_{reinterpret_cast<ValueT*>(buffer_)}, size_{0} {}

    ~static_ring_buffer() {
        while (!empty()) {
            pop_front();
        }
    }

    constexpr bool empty() const { return size_ == 0; }

    constexpr std::size_t size() const { return size_; }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        if /*constexpr*/ (SafeMode) {
            if (size_ == N) {
                throw std::range_error{"adding value would exceed max container capacity"};
            }
        }
        new (wr_p_) ValueT{std::forward<Args>(args)...};
        ++wr_p_;
        ++size_;
        if (wr_p_ == (reinterpret_cast<ValueT*>(buffer_) + N)) {
            wr_p_ = reinterpret_cast<ValueT*>(buffer_);
        }
    }

    constexpr ValueT& front() { return *rd_p_; }

    void pop_front() {
        if /*constexpr*/ (SafeMode) {
            if (size_ == 0) {
                throw std::range_error{"container is already empty"};
            }
        }
        rd_p_->~ValueT();
        ++rd_p_;
        --size_;
        if (rd_p_ == (reinterpret_cast<ValueT*>(buffer_) + N)) {
            rd_p_ = reinterpret_cast<ValueT*>(buffer_);
        }
    }

   private:
    ValueT* wr_p_;
    ValueT* rd_p_;
    alignas(ValueT) std::uint8_t buffer_[N * sizeof(ValueT)];
    std::size_t size_;
};
