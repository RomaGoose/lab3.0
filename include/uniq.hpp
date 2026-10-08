
#include <cstddef>
#include <type_traits>
template<typename T>
class uniq_ptr {
    
    using element_type = std::remove_extent_t<T>;
    
    element_type* ptr_;

    void free_mem(){
        if constexpr (std::is_array_v<T>) {
            delete[] ptr_;
        }
        else 
            delete ptr_;
    }
    
public:

     uniq_ptr(element_type* ptr = nullptr) noexcept : ptr_(ptr) {};
    
    uniq_ptr(const uniq_ptr<T>& other) = delete;
    uniq_ptr& operator=(const uniq_ptr<T>& other) = delete;

    uniq_ptr(uniq_ptr<T>&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    };
    uniq_ptr& operator=(uniq_ptr<T>&& other) noexcept {
        if(ptr_ != other.get()){
            free_mem();
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }

        return *this;
    };  

    ~uniq_ptr() {
        free_mem();
    }

    element_type* get() const noexcept {
        return ptr_;
    };
    explicit operator bool() const noexcept {
        return ptr_ != nullptr; 
    };
    element_type& operator*() const noexcept requires (!std::is_array_v<T>) {
        return *ptr_;
    };
    element_type* operator->() const noexcept requires (!std::is_array_v<T>) {
        return ptr_;
    };
    element_type* release() noexcept {
        element_type* tmp = ptr_;
        ptr_ = nullptr;
        return tmp;
    };
    void reset(element_type* new_ptr = nullptr) noexcept {
        if(ptr_ != new_ptr) {
            free_mem();
            ptr_ = new_ptr;
        }
    };
    element_type& operator[](size_t index) requires std::is_array_v<T> {
        return ptr_[index];
    }

    const element_type& operator[](size_t index) const requires std::is_array_v<T> {
        return ptr_[index];
    }
};