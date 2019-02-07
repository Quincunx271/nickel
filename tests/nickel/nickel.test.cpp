#include <nickel/nickel.hpp>

#include <ostream>
#include <utility>

#include <catch2/catch.hpp>

namespace {
    NICKEL_NAME(foo_name, foo);
    NICKEL_NAME(bar_name, bar);

    auto some_function()
    {
        return nickel::wrap(foo_name, bar_name)([](auto&& foo, auto&& bar) {
            return std::forward<decltype(foo)>(foo)(
                std::forward<decltype(bar)>(bar));
        });
    }
}

TEST_CASE("preserves value category")
{
    int mut_lvalue = 42;

    auto result = some_function() //
                      .foo([](int& i) { return i; })
                      .bar(mut_lvalue)();

    CHECK(result == mut_lvalue);
}

namespace {
    struct state
    {
        int default_ctor = 0;
        int copy_ctor = 0;
        int copy_assign = 0;
        int move_ctor = 0;
        int move_assign = 0;
        int dtor = 0;

        constexpr decltype(auto) as_tuple() const
        {
            return std::tie(default_ctor, copy_ctor, copy_assign, move_ctor,
                move_assign, dtor);
        }

        friend constexpr bool operator==(state const& lhs, state const& rhs)
        {
            return lhs.as_tuple() == rhs.as_tuple();
        }

        friend constexpr bool operator!=(state const& lhs, state const& rhs)
        {
            return lhs.as_tuple() != rhs.as_tuple();
        }

        friend std::ostream& operator<<(std::ostream& out, state const& it)
        {
            return out << "default: " << it.default_ctor << "\n"
                       << "copy_ctor: " << it.copy_ctor
                       << ", copy_assign: " << it.copy_assign
                       << ", total: " << it.copy_ctor + it.copy_assign << "\n"
                       << "move_ctor: " << it.move_ctor
                       << ", move_assign: " << it.move_assign
                       << ", total: " << it.move_ctor + it.move_assign << "\n"
                       << "dtor: " << it.dtor;
        }
    };

    struct count_special
    {
        static state state_;

        count_special()
        {
            ++state_.default_ctor;
        }

        count_special(count_special const&)
        {
            ++state_.copy_ctor;
        }

        count_special(count_special&&)
        {
            ++state_.move_ctor;
        }

        ~count_special()
        {
            ++state_.dtor;
        }

        count_special& operator=(count_special const&)
        {
            ++state_.copy_assign;
            return *this;
        }

        count_special& operator=(count_special&&)
        {
            ++state_.move_assign;
            return *this;
        }

        static auto default_ctor()
        {
            return state_.default_ctor;
        }

        static auto copy_ctor()
        {
            return state_.copy_ctor;
        }

        static auto copy_assign()
        {
            return state_.copy_assign;
        }

        static auto move_ctor()
        {
            return state_.move_ctor;
        }

        static auto move_assign()
        {
            return state_.move_assign;
        }

        static auto dtor()
        {
            return state_.dtor;
        }

        static auto copy()
        {
            return copy_ctor() + copy_assign();
        }

        static auto move()
        {
            return move_ctor() + move_assign();
        }

        static void reset()
        {
            state_ = state{};
        }

        static auto get_state()
        {
            return state_;
        }
    };

    template <typename T>
    auto lifetime(T const& t)
    {
        return nickel::wrap(foo_name, bar_name)([&t](auto&& foo, auto&& bar) {
            (void)bar;
            return foo(t);
        });
    }

    state count_special::state_ = {};
}

TEST_CASE("proper lifetime management")
{
    count_special::reset();

    SECTION("unnamed args")
    {
        SECTION("clvalue")
        {
            count_special const clvalue{};

            auto const state = count_special::get_state();
            lifetime(clvalue)
                .foo([](auto&& t) {
                    (void)t; //
                })
                .bar(42)();

            CHECK(state == count_special::get_state());
        }
        SECTION("mlvalue")
        {
            count_special clvalue{};

            auto const state = count_special::get_state();
            lifetime(clvalue)
                .foo([](auto&& t) {
                    (void)t; //
                })
                .bar(42)();

            CHECK(state == count_special::get_state());
        }
        SECTION("rvalue")
        {
            auto const state_before = [] {
                auto result = count_special::get_state();
                // We construct it in creating the rvalue
                ++result.default_ctor;

                return result;
            }();

            try {
                lifetime(count_special{})
                    .foo([&state_before](auto&& t) {
                        if (t.get_state() != state_before) throw t.get_state();
                    })
                    .bar(42)();
            } catch (state const& st) {
                CHECK(st == state_before);
            }

            auto const state_after = [&] {
                state result = state_before;
                ++result.dtor;
                return result;
            }();
            CHECK(state_after == count_special::get_state());
        }
    }
}
