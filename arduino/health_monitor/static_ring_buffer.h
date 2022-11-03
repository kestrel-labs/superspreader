#pragma once

#include <cstdint>
#include <utility>

template <typename ValueT, std::size_t N>
class static_ring_buffer {
    static_assert(N > 0, "effective size of ring buffer must be > 0");

   public:
    static_ring_buffer()
        : wr_p_{reinterpret_cast<ValueT*>(buffer_)}, rd_p_{reinterpret_cast<ValueT*>(buffer_)} {}

    ~static_ring_buffer() {
        while (!empty()) {
            pop_front();
        }
    }

    constexpr bool empty() const { wr_p_ == rd_p_; }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        new (wr_p_) ValueT{std::forward<Args>(args)...};
        ++wr_p_;
        if (wr_p_ == (reinterpret_cast<ValueT*>(buffer_) + N)) {
            wr_p_ = reinterpret_cast<ValueT*>(buffer_);
        }
    }

    constexpr ValueT& front() { return *rd_p_; }

    constexpr void pop_front() {
        rd_p_->~ValueT();
        ++rd_p_;
        if (rd_p_ == (reinterpret_cast<ValueT*>(buffer_) + N)) {
            rd_p_ = reinterpret_cast<ValueT*>(buffer_);
        }
    }

   private:
    ValueT* wr_p_;
    ValueT* rd_p_;
    alignas(ValueT) std::uint8_t buffer_[N * sizeof(ValueT)];
};
