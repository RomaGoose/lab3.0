#include <chrono>
#include <iostream>
#include <memory>
#include <vector>

#include "uniq.hpp"
#include "shar.hpp"

template<typename T>
inline void do_not_optimize(T const& v) {
    asm volatile("" : : "r,m"(v) : "memory");
}

struct Payload {
    int a = 0, b = 0, c = 0;
    Payload(int v) : a(v), b(v + 42), c(v*6767) {}
};

template<typename F>
double bench(F fn, int n) {
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < n; ++i) fn(i);
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

int main() {
    const int N = 100000;

    // --- create ---
    auto std_u_create = [](int i) { auto p = std::make_unique<Payload>(1); do_not_optimize(p); };
    auto my_u_create  = [](int i) { uniq_ptr<Payload> p(new Payload(1)); do_not_optimize(p); };
    auto std_s_create = [](int i) { auto p = std::make_shared<Payload>(1); do_not_optimize(p); };
    auto my_s_create  = [](int i) { shar_ptr<Payload> p(new Payload(1)); do_not_optimize(p); };

    std::cout << "create unique: std " << bench(std_u_create, N) << " ms\n";
    std::cout << "create unique: my  " << bench(my_u_create,  N) << " ms\n";
    std::cout << "create shared: std " << bench(std_s_create, N) << " ms\n";
    std::cout << "create shared: my  " << bench(my_s_create,  N) << " ms\n";

    // --- copy (shared) ---
    std::vector<std::shared_ptr<Payload>> std_src;
    std::vector<shar_ptr<Payload>>        my_src;
    std_src.reserve(N);
    my_src.reserve(N);
    for (int i = 0; i < N; ++i) {
        std_src.push_back(std::make_shared<Payload>(i));
        my_src.push_back(shar_ptr<Payload>(new Payload(i)));
    }

    auto std_s_copy = [&](int i) { auto p = std_src[i]; (void)p; };
    auto my_s_copy  = [&](int i) { auto p = my_src[i];  (void)p; };

    std::cout << "copy shared:   std " << bench(std_s_copy, N) << " ms\n";
    std::cout << "copy shared:   my  " << bench(my_s_copy,  N) << " ms\n";

    // --- move (unique) ---
    std::vector<std::unique_ptr<Payload>> std_u_src;
    std::vector<uniq_ptr<Payload>>        my_u_src;
    std_u_src.reserve(N);
    my_u_src.reserve(N);
    for (int i = 0; i < N; ++i) {
        std_u_src.push_back(std::make_unique<Payload>(i));
        my_u_src.push_back(uniq_ptr<Payload>(new Payload(i)));
    }

    auto std_u_move = [&](int i) { auto p = std::move(std_u_src[i]); (void)p; };
    auto my_u_move  = [&](int i) { auto p = std::move(my_u_src[i]);  (void)p; };

    std::cout << "move unique:   std " << bench(std_u_move, N) << " ms\n";
    std::cout << "move unique:   my  " << bench(my_u_move,  N) << " ms\n";

    // --- destroy ---
    std::vector<std::unique_ptr<Payload>> std_u_kill;
    std::vector<uniq_ptr<Payload>>        my_u_kill;
    std::vector<std::shared_ptr<Payload>> std_s_kill;
    std::vector<shar_ptr<Payload>>        my_s_kill;
    std_u_kill.reserve(N);
    my_u_kill.reserve(N);
    std_s_kill.reserve(N);
    my_s_kill.reserve(N);

    for (int i = 0; i < N; ++i) {
        std_u_kill.push_back(std::make_unique<Payload>(i));
        my_u_kill.push_back(uniq_ptr<Payload>(new Payload(i)));
        std_s_kill.push_back(std::make_shared<Payload>(i));
        my_s_kill.push_back(shar_ptr<Payload>(new Payload(i)));
    }

    auto t0 = std::chrono::steady_clock::now();
    std_u_kill.clear();
    auto t1 = std::chrono::steady_clock::now();
    std::cout << "destroy unique: std "
              << std::chrono::duration<double, std::milli>(t1 - t0).count() << " ms\n";

    t0 = std::chrono::steady_clock::now();
    my_u_kill.clear();
    t1 = std::chrono::steady_clock::now();
    std::cout << "destroy unique: my  "
              << std::chrono::duration<double, std::milli>(t1 - t0).count() << " ms\n";

    t0 = std::chrono::steady_clock::now();
    std_s_kill.clear();
    t1 = std::chrono::steady_clock::now();
    std::cout << "destroy shared: std "
              << std::chrono::duration<double, std::milli>(t1 - t0).count() << " ms\n";

    t0 = std::chrono::steady_clock::now();
    my_s_kill.clear();
    t1 = std::chrono::steady_clock::now();
    std::cout << "destroy shared: my  "
              << std::chrono::duration<double, std::milli>(t1 - t0).count() << " ms\n";
}