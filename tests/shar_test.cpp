#include <catch2/catch_test_macros.hpp>

#include <type_traits>
#include <utility>

#include "shar.hpp"

namespace {

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

struct Base {
    static int base_deleted;
    ~Base() { ++base_deleted; }
};
int Base::base_deleted = 0;

struct Derived : Base {
    static int derived_deleted;
    ~Derived() { ++derived_deleted; }
};
int Derived::derived_deleted = 0;

struct Fixture {
    Fixture() {
        Tracker::alive = 0;
        Tracker::deleted = 0;
        Base::base_deleted = 0;
        Derived::derived_deleted = 0;
    }
};

} 


// =============================================================================
// Базовое владение
// =============================================================================

TEST_CASE_METHOD(Fixture, "shar_ptr: default construction is empty", "[shar_ptr]") {
    shar_ptr<Tracker> p;
    REQUIRE(p.get() == nullptr);
    REQUIRE_FALSE(static_cast<bool>(p));
    REQUIRE(p.ref_count() == 0);
}

TEST_CASE_METHOD(Fixture, "shar_ptr: construction from raw pointer owns it",
                 "[shar_ptr]") {
    shar_ptr<Tracker> p(new Tracker(42));
    REQUIRE(p.get() != nullptr);
    REQUIRE(static_cast<bool>(p));
    REQUIRE(p->value == 42);
    REQUIRE(p.ref_count() == 1);
    REQUIRE(Tracker::alive == 1);
    REQUIRE(Tracker::deleted == 0);
}

TEST_CASE_METHOD(Fixture, "shar_ptr: construction from nullptr", "[shar_ptr]") {
    shar_ptr<Tracker> p(static_cast<Tracker*>(nullptr));
    REQUIRE(p.get() == nullptr);
    REQUIRE_FALSE(static_cast<bool>(p));
    REQUIRE(p.ref_count() == 0);
    REQUIRE(Tracker::alive == 0);
}

TEST_CASE_METHOD(Fixture, "shar_ptr: destructor deletes owned object when last owner dies",
                 "[shar_ptr]") {
    {
        shar_ptr<Tracker> p(new Tracker(1));
        REQUIRE(Tracker::alive == 1);
        REQUIRE(Tracker::deleted == 0);
    }
    REQUIRE(Tracker::alive == 0);
    REQUIRE(Tracker::deleted == 1);
}

TEST_CASE_METHOD(Fixture, "shar_ptr: operator* and operator->", "[shar_ptr]") {
    shar_ptr<Tracker> p(new Tracker(11));
    REQUIRE((*p).value == 11);

    (*p).value = 22;
    REQUIRE(p->value == 22);
    REQUIRE((*p).value == 22);
}

// =============================================================================
// Копирование и подсчёт ссылок
// =============================================================================

TEST_CASE_METHOD(Fixture, "shar_ptr: copy constructor increments ref_count",
                 "[shar_ptr]") {
    shar_ptr<Tracker> a(new Tracker(1));
    Tracker* raw = a.get();

    shar_ptr<Tracker> b(a);

    REQUIRE(a.get() == raw);
    REQUIRE(b.get() == raw);
    REQUIRE(a.ref_count() == 2);
    REQUIRE(b.ref_count() == 2);
    REQUIRE(Tracker::alive == 1);
    REQUIRE(Tracker::deleted == 0);
}

TEST_CASE_METHOD(Fixture, "shar_ptr: object survives until last owner dies",
                 "[shar_ptr]") {
    shar_ptr<Tracker> a(new Tracker(1));
    {
        shar_ptr<Tracker> b(a);
        {
            shar_ptr<Tracker> c(a);
            REQUIRE(a.ref_count() == 3);
        }
        REQUIRE(a.ref_count() == 2);
        REQUIRE(Tracker::alive == 1);
    }
    REQUIRE(a.ref_count() == 1);
    REQUIRE(Tracker::alive == 1);
    REQUIRE(Tracker::deleted == 0);
}

TEST_CASE_METHOD(Fixture, "shar_ptr: object destroyed when last owner goes out of scope",
                 "[shar_ptr]") {
    {
        shar_ptr<Tracker> a(new Tracker(1));
        shar_ptr<Tracker> b(a);
        REQUIRE(Tracker::alive == 1);
    }
    REQUIRE(Tracker::alive == 0);
    REQUIRE(Tracker::deleted == 1);
}

TEST_CASE_METHOD(Fixture, "shar_ptr: copy assignment shares ownership",
                 "[shar_ptr]") {
    shar_ptr<Tracker> a(new Tracker(1));
    shar_ptr<Tracker> b(new Tracker(2));
    Tracker* raw_a = a.get();

    b = a;

    REQUIRE(b.get() == raw_a);
    REQUIRE(a.ref_count() == 2);
    REQUIRE(b.ref_count() == 2);
    REQUIRE(Tracker::alive == 1);   
    REQUIRE(Tracker::deleted == 1);
}

TEST_CASE_METHOD(Fixture, "shar_ptr: copy assignment releases previous ownership",
                 "[shar_ptr]") {
    shar_ptr<Tracker> a(new Tracker(1));
    shar_ptr<Tracker> b(a);
    shar_ptr<Tracker> c(new Tracker(2));

    c = a;

    REQUIRE(a.ref_count() == 3);
    REQUIRE(c.ref_count() == 3);
    REQUIRE(Tracker::alive == 1);
    REQUIRE(Tracker::deleted == 1);
}

TEST_CASE_METHOD(Fixture, "shar_ptr: self copy assignment is no-op", "[shar_ptr]") {
    shar_ptr<Tracker> p(new Tracker(1));
    Tracker* raw = p.get();

    p = p;

    REQUIRE(p.get() == raw);
    REQUIRE(p.ref_count() == 1);
    REQUIRE(Tracker::alive == 1);
    REQUIRE(Tracker::deleted == 0);
}

TEST_CASE_METHOD(Fixture, "shar_ptr: self move assignment falls back to copy and is no-op",
                 "[shar_ptr]") {
    shar_ptr<Tracker> p(new Tracker(1));
    Tracker* raw = p.get();

    p = std::move(p);

    REQUIRE(p.get() == raw);
    REQUIRE(p.ref_count() == 1);
    REQUIRE(Tracker::alive == 1);
    REQUIRE(Tracker::deleted == 0);
}

TEST_CASE_METHOD(Fixture, "shar_ptr: swap via copy of temporaries", "[shar_ptr]") {
    shar_ptr<Tracker> a(new Tracker(1));
    shar_ptr<Tracker> b(new Tracker(2));
    Tracker* ra = a.get();
    Tracker* rb = b.get();

    shar_ptr<Tracker> tmp(a);
    a = b;
    b = tmp;

    REQUIRE(a.get() == rb);
    REQUIRE(b.get() == ra);
    REQUIRE(a.ref_count() == 1);
    REQUIRE(b.ref_count() == 2);
    REQUIRE(Tracker::alive == 2);
    REQUIRE(Tracker::deleted == 0);
}

// =============================================================================
// Стирание типа в control block
// =============================================================================

TEST_CASE_METHOD(Fixture, "shar_ptr: converting copy Derived -> Base shares control block",
                 "[shar_ptr]") {
    shar_ptr<Derived> d(new Derived());
    Derived* raw = d.get();

    shar_ptr<Base> b(d);

    REQUIRE(d.ref_count() == 2);
    REQUIRE(b.ref_count() == 2);
    REQUIRE(b.get() == raw);   
    REQUIRE(Derived::derived_deleted == 0);
}

TEST_CASE_METHOD(Fixture, "shar_ptr: delete through Base calls the real Derived destructor",
                 "[shar_ptr]") {
    {
        shar_ptr<Base> p(new Derived());
        REQUIRE(Derived::derived_deleted == 0);
        REQUIRE(Base::base_deleted == 0);
    }
    REQUIRE(Derived::derived_deleted == 1);
    REQUIRE(Base::base_deleted == 1);
}
