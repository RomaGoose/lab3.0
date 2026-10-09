#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "uniq.hpp"

namespace {
    
template<typename T>
concept has_deref = requires(T& t) {
    *t;
};

template<typename T>
concept has_arrow = requires(T& t) {
    t.operator->();
};

template<typename T>
concept has_subscript = requires(T& t, std::size_t i) {
    t[i];
};

struct Tracker {
    static int alive;
    static int deleted;

    int value;

    explicit Tracker(int v = 0) : value(v) { ++alive; }
    ~Tracker() { --alive; ++deleted; }

    Tracker(const Tracker&) = delete;
    Tracker& operator=(const Tracker&) = delete;
};

int Tracker::alive = 0;
int Tracker::deleted = 0;

struct Fixture {
    Fixture() {
        Tracker::alive = 0;
        Tracker::deleted = 0;
    }
};

}

// =============================================================================
// uniq_ptr<T>
// =============================================================================

TEST_CASE_METHOD(Fixture, "uniq_ptr<T>: default construction is empty", "[uniq_ptr]") {
    uniq_ptr<Tracker> p;
    REQUIRE(p.get() == nullptr);
    REQUIRE_FALSE(static_cast<bool>(p));
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T>: construction from raw pointer owns it",
                 "[uniq_ptr]") {
    uniq_ptr<Tracker> p(new Tracker(42));
    REQUIRE(p.get() != nullptr);
    REQUIRE(static_cast<bool>(p));
    REQUIRE(p->value == 42);
    REQUIRE(Tracker::alive == 1);
    REQUIRE(Tracker::deleted == 0);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T>: construction from nullptr", "[uniq_ptr]") {
    uniq_ptr<Tracker> p(nullptr);
    REQUIRE(p.get() == nullptr);
    REQUIRE_FALSE(static_cast<bool>(p));
    REQUIRE(Tracker::alive == 0);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T>: destructor deletes owned object",
                 "[uniq_ptr]") {
    {
        uniq_ptr<Tracker> p(new Tracker(1));
        REQUIRE(Tracker::alive == 1);
        REQUIRE(Tracker::deleted == 0);
    }
    REQUIRE(Tracker::alive == 0);
    REQUIRE(Tracker::deleted == 1);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T>: move constructor transfers ownership",
                 "[uniq_ptr]") {
    uniq_ptr<Tracker> a(new Tracker(7));
    Tracker* raw = a.get();

    uniq_ptr<Tracker> b(std::move(a));

    REQUIRE(a.get() == nullptr);
    REQUIRE(b.get() == raw);
    REQUIRE(b->value == 7);
    REQUIRE(Tracker::alive == 1);
    REQUIRE(Tracker::deleted == 0);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T>: move constructor from empty is safe",
                 "[uniq_ptr]") {
    uniq_ptr<Tracker> a;
    uniq_ptr<Tracker> b(std::move(a));
    REQUIRE(a.get() == nullptr);
    REQUIRE(b.get() == nullptr);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T>: move assignment deletes current and takes new",
                 "[uniq_ptr]") {
    uniq_ptr<Tracker> a(new Tracker(1));
    uniq_ptr<Tracker> b(new Tracker(2));
    Tracker* fresh = b.get();

    a = std::move(b);

    REQUIRE(a.get() == fresh);
    REQUIRE(b.get() == nullptr);
    REQUIRE(Tracker::deleted == 1);
    REQUIRE(Tracker::alive == 1);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T>: move assignment from empty releases current",
                 "[uniq_ptr]") {
    uniq_ptr<Tracker> a(new Tracker(1));
    uniq_ptr<Tracker> empty;

    a = std::move(empty);

    REQUIRE(a.get() == nullptr);
    REQUIRE(empty.get() == nullptr);
    REQUIRE(Tracker::alive == 0);
    REQUIRE(Tracker::deleted == 1);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T>: release gives up ownership", "[uniq_ptr]") {
    uniq_ptr<Tracker> p(new Tracker(5));
    Tracker* raw = p.get();

    Tracker* released = p.release();

    REQUIRE(released == raw);
    REQUIRE(p.get() == nullptr);
    REQUIRE(Tracker::alive == 1);
    REQUIRE(Tracker::deleted == 0);

    delete released;
    REQUIRE(Tracker::alive == 0);
    REQUIRE(Tracker::deleted == 1);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T>: reset() deletes current and becomes empty",
                 "[uniq_ptr]") {
    uniq_ptr<Tracker> p(new Tracker(3));
    p.reset();
    REQUIRE(p.get() == nullptr);
    REQUIRE(Tracker::alive == 0);
    REQUIRE(Tracker::deleted == 1);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T>: reset() on empty is safe", "[uniq_ptr]") {
    uniq_ptr<Tracker> p;
    p.reset();
    REQUIRE(p.get() == nullptr);
    REQUIRE(Tracker::deleted == 0);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T>: reset(ptr) deletes old and takes new",
                 "[uniq_ptr]") {
    uniq_ptr<Tracker> p(new Tracker(1));
    Tracker* fresh = new Tracker(2);

    p.reset(fresh);

    REQUIRE(p.get() == fresh);
    REQUIRE(Tracker::alive == 1);
    REQUIRE(Tracker::deleted == 1);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T>: reset(nullptr) releases current",
                 "[uniq_ptr]") {
    uniq_ptr<Tracker> p(new Tracker(1));
    p.reset(nullptr);
    REQUIRE(p.get() == nullptr);
    REQUIRE(Tracker::alive == 0);
    REQUIRE(Tracker::deleted == 1);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T>: operator* and operator->", "[uniq_ptr]") {
    uniq_ptr<Tracker> p(new Tracker(11));
    REQUIRE((*p).value == 11);

    (*p).value = 22;
    REQUIRE(p->value == 22);
    REQUIRE((*p).value == 22);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T>: is not copyable", "[uniq_ptr]") {
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<uniq_ptr<Tracker>>);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<uniq_ptr<Tracker>>);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T>: is nothrow movable", "[uniq_ptr]") {
    STATIC_REQUIRE(std::is_move_constructible_v<uniq_ptr<Tracker>>);
    STATIC_REQUIRE(std::is_move_assignable_v<uniq_ptr<Tracker>>);
    STATIC_REQUIRE(std::is_nothrow_move_constructible_v<uniq_ptr<Tracker>>);
    STATIC_REQUIRE(std::is_nothrow_move_assignable_v<uniq_ptr<Tracker>>);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T>: self move assignment keeps object valid",
                 "[uniq_ptr]") {
    uniq_ptr<Tracker> p(new Tracker(1));
    Tracker* raw = p.get();

    p = std::move(p);

    if (p.get() != nullptr) {
        REQUIRE(p.get() == raw);
        REQUIRE(Tracker::deleted == 0);
        REQUIRE(Tracker::alive == 1);
    } else {
        REQUIRE(Tracker::deleted == 1);
        REQUIRE(Tracker::alive == 0);
    }
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T>: releases resource on exception",
                 "[uniq_ptr]") {
    auto fn = [] {
        uniq_ptr<Tracker> p(new Tracker(1));
        throw std::runtime_error("boom");
    };

    REQUIRE_THROWS_AS(fn(), std::runtime_error);
    REQUIRE(Tracker::alive == 0);
    REQUIRE(Tracker::deleted == 1);
}

// =============================================================================
// uniq_ptr<T[]>
// =============================================================================

TEST_CASE_METHOD(Fixture, "uniq_ptr<T[]>: default construction is empty",
                 "[uniq_ptr][array]") {
    uniq_ptr<Tracker[]> p;
    REQUIRE(p.get() == nullptr);
    REQUIRE_FALSE(static_cast<bool>(p));
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T[]>: destructor deletes array with delete[]",
                 "[uniq_ptr][array]") {
    {
        uniq_ptr<Tracker[]> p(new Tracker[3]);
        REQUIRE(p.get() != nullptr);
        REQUIRE(static_cast<bool>(p));
        REQUIRE(Tracker::alive == 3);
        REQUIRE(Tracker::deleted == 0);
    }
    REQUIRE(Tracker::alive == 0);
    REQUIRE(Tracker::deleted == 3);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T[]>: construction from nullptr",
                 "[uniq_ptr][array]") {
    uniq_ptr<Tracker[]> p(nullptr);
    REQUIRE(p.get() == nullptr);
    REQUIRE(Tracker::alive == 0);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T[]>: operator[] gives mutable access",
                 "[uniq_ptr][array]") {
    uniq_ptr<Tracker[]> p(new Tracker[4]);
    for (std::size_t i = 0; i < 4; ++i) {
        p[i].value = static_cast<int>(i * 10);
    }
    for (std::size_t i = 0; i < 4; ++i) {
        REQUIRE(p[i].value == static_cast<int>(i * 10));
    }
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T[]>: operator[] on const object",
                 "[uniq_ptr][array]") {
    uniq_ptr<Tracker[]> p(new Tracker[2]);
    p[0].value = 1;
    p[1].value = 2;

    const uniq_ptr<Tracker[]>& cp = p;
    REQUIRE(cp[0].value == 1);
    REQUIRE(cp[1].value == 2);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T[]>: move constructor transfers ownership",
                 "[uniq_ptr][array]") {
    uniq_ptr<Tracker[]> a(new Tracker[3]);
    Tracker* raw = a.get();

    uniq_ptr<Tracker[]> b(std::move(a));

    REQUIRE(a.get() == nullptr);
    REQUIRE(b.get() == raw);
    REQUIRE(Tracker::alive == 3);
    REQUIRE(Tracker::deleted == 0);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T[]>: move constructor from empty is safe",
                 "[uniq_ptr][array]") {
    uniq_ptr<Tracker[]> a;
    uniq_ptr<Tracker[]> b(std::move(a));
    REQUIRE(a.get() == nullptr);
    REQUIRE(b.get() == nullptr);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T[]>: move assignment deletes current array",
                 "[uniq_ptr][array]") {
    uniq_ptr<Tracker[]> a(new Tracker[2]);
    uniq_ptr<Tracker[]> b(new Tracker[5]);
    Tracker* fresh = b.get();

    a = std::move(b);

    REQUIRE(a.get() == fresh);
    REQUIRE(b.get() == nullptr);
    REQUIRE(Tracker::alive == 5);
    REQUIRE(Tracker::deleted == 2);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T[]>: move assignment from empty releases current",
                 "[uniq_ptr][array]") {
    uniq_ptr<Tracker[]> a(new Tracker[3]);
    uniq_ptr<Tracker[]> empty;

    a = std::move(empty);

    REQUIRE(a.get() == nullptr);
    REQUIRE(empty.get() == nullptr);
    REQUIRE(Tracker::alive == 0);
    REQUIRE(Tracker::deleted == 3);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T[]>: release gives up ownership",
                 "[uniq_ptr][array]") {
    uniq_ptr<Tracker[]> p(new Tracker[3]);
    Tracker* raw = p.get();

    Tracker* released = p.release();

    REQUIRE(released == raw);
    REQUIRE(p.get() == nullptr);
    REQUIRE(Tracker::alive == 3);
    REQUIRE(Tracker::deleted == 0);

    delete[] released;
    REQUIRE(Tracker::alive == 0);
    REQUIRE(Tracker::deleted == 3);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T[]>: reset() deletes array and becomes empty",
                 "[uniq_ptr][array]") {
    uniq_ptr<Tracker[]> p(new Tracker[4]);
    p.reset();
    REQUIRE(p.get() == nullptr);
    REQUIRE(Tracker::alive == 0);
    REQUIRE(Tracker::deleted == 4);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T[]>: reset(ptr) replaces array",
                 "[uniq_ptr][array]") {
    uniq_ptr<Tracker[]> p(new Tracker[2]);
    Tracker* fresh = new Tracker[3];

    p.reset(fresh);

    REQUIRE(p.get() == fresh);
    REQUIRE(Tracker::alive == 3);
    REQUIRE(Tracker::deleted == 2);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T[]>: reset(nullptr) releases array",
                 "[uniq_ptr][array]") {
    uniq_ptr<Tracker[]> p(new Tracker[2]);
    p.reset(nullptr);
    REQUIRE(p.get() == nullptr);
    REQUIRE(Tracker::alive == 0);
    REQUIRE(Tracker::deleted == 2);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T[]>: is not copyable, is nothrow movable",
                 "[uniq_ptr][array]") {
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<uniq_ptr<Tracker[]>>);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<uniq_ptr<Tracker[]>>);
    STATIC_REQUIRE(std::is_move_constructible_v<uniq_ptr<Tracker[]>>);
    STATIC_REQUIRE(std::is_move_assignable_v<uniq_ptr<Tracker[]>>);
    STATIC_REQUIRE(std::is_nothrow_move_constructible_v<uniq_ptr<Tracker[]>>);
    STATIC_REQUIRE(std::is_nothrow_move_assignable_v<uniq_ptr<Tracker[]>>);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T[]>: has no operator* or operator->",
                 "[uniq_ptr][array]") {
    STATIC_REQUIRE_FALSE(has_deref<uniq_ptr<Tracker[]>>);
    STATIC_REQUIRE_FALSE(has_arrow<uniq_ptr<Tracker[]>>);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T>: has operator* and operator->",
                 "[uniq_ptr]") {
    STATIC_REQUIRE(has_deref<uniq_ptr<Tracker>>);
    STATIC_REQUIRE(has_arrow<uniq_ptr<Tracker>>);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T>: has no operator[]", "[uniq_ptr]") {
    STATIC_REQUIRE_FALSE(has_subscript<uniq_ptr<Tracker>>);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T[]>: works with trivial types",
                 "[uniq_ptr][array]") {
    uniq_ptr<int[]> p(new int[5]{1, 2, 3, 4, 5});
    REQUIRE(p[0] == 1);
    REQUIRE(p[1] == 2);
    REQUIRE(p[2] == 3);
    REQUIRE(p[3] == 4);
    REQUIRE(p[4] == 5);

    p[2] = 30;
    REQUIRE(p[2] == 30);
}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T[]>: self move assignment keeps object valid",
                 "[uniq_ptr][array]") {
    uniq_ptr<Tracker[]> p(new Tracker[2]);
    Tracker* raw = p.get();

    p = std::move(p);

    REQUIRE(p.get() == raw);
    REQUIRE(Tracker::deleted == 0);
    REQUIRE(Tracker::alive == 2);

}

TEST_CASE_METHOD(Fixture, "uniq_ptr<T[]>: releases resource on exception",
                 "[uniq_ptr][array]") {
    auto fn = [] {
        uniq_ptr<Tracker[]> p(new Tracker[3]);
        throw std::runtime_error("boom");
    };

    REQUIRE_THROWS_AS(fn(), std::runtime_error);
    REQUIRE(Tracker::alive == 0);
    REQUIRE(Tracker::deleted == 3);
}