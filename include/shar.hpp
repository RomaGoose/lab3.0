#pragma once

#include <concepts>
#include <cstddef>

struct control_block_base {
    std::size_t shared_count = 0;

    virtual void destroy() noexcept = 0;
    virtual ~control_block_base() = default;
};

template<typename U>
struct control_block : control_block_base {
    U* ptr;

    explicit control_block(U* p) : ptr(p) {}

    void destroy() noexcept override {
        delete ptr;
        ptr = nullptr;
    }
};


template<typename T>
class shar_ptr {
    template<typename U>
    friend class shar_ptr;

    T* ptr_ = nullptr;
    control_block_base* ctrl_ = nullptr;

    void release() noexcept {
        if (ctrl_ == nullptr) return;
        if (--ctrl_->shared_count == 0) {
            ctrl_->destroy();
            delete ctrl_;
        }
        ptr_ = nullptr;
        ctrl_ = nullptr;
    }

public:
    shar_ptr() noexcept = default;

    template<typename U>
    requires std::convertible_to<U*, T*>
    explicit shar_ptr(U* p) : ptr_(p) {
        if (p != nullptr) {
            ctrl_ = new control_block<U>(p);
            ctrl_->shared_count = 1;
        }
    }

    shar_ptr(const shar_ptr& other) noexcept
        : ptr_(other.ptr_), ctrl_(other.ctrl_) {
        if (ctrl_ != nullptr) {
            ++ctrl_->shared_count;
        }
    }

    shar_ptr& operator=(const shar_ptr& other) noexcept {
        if (this != &other) {
            release();
            ptr_ = other.ptr_;
            ctrl_ = other.ctrl_;
            if (ctrl_ != nullptr) {
                ++ctrl_->shared_count;
            }
        }
        return *this;
    }

    template<typename U>
    requires std::convertible_to<U*, T*>
    shar_ptr(const shar_ptr<U>& other) noexcept
        : ptr_(other.ptr_), ctrl_(other.ctrl_) {
        if (ctrl_ != nullptr) {
            ++ctrl_->shared_count;
        }
    }

    ~shar_ptr() {
        release();
    }

    T* get() const noexcept { return ptr_; }
    T& operator*() const { return *ptr_; }
    T* operator->() const noexcept { return ptr_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    size_t ref_count() const noexcept {
        return ctrl_ ? ctrl_->shared_count : 0;
    }
};