#pragma once

#include <type_traits>
#include <utility>

namespace nickel {
    namespace detail {
        template <bool B>
        struct conditional
        {
            template <typename A, typename>
            using eval = A;
        };

        template <>
        struct conditional<false>
        {
            template <typename, typename B>
            using eval = B;
        };

        template <bool Cond, typename A, typename B>
        using conditional_t = typename conditional<Cond>::template eval<A, B>;

        template <bool... Bs>
        struct bool_pack
        {};

        template <bool... Bs>
        constexpr bool all = std::is_same< //
            bool_pack<true, Bs...>, //
            bool_pack<Bs..., true> //
            >::value;

        // By De Morgan's Law:
        template <bool... Bs>
        constexpr bool any = !all<!Bs...>;

        template <typename T>
        struct type_identity
        {
            using type = T;
        };

        template <typename... Ts>
        struct inherit : Ts...
        {};

        // Slightly modified version of `mp_map_find` from Boost.MP11, copied
        // with permission from Peter Dimov.
        template <typename K, typename... Ts>
        struct mp_map_find_impl
        {
            using U = inherit<type_identity<Ts>...>;

            // L<K, U...> matches any template something<K, anything...>.
            // This L<K, U...> element would be one of the types in Ts....
            template <template <typename...> class L, typename... U>
            static type_identity<L<K, U...>> f(type_identity<L<K, U...>>*);
            static type_identity<void> f(...);

            // The `U*` can be converted to `type_identity` of any of the Ts...
            using V = decltype(f((U*)nullptr));

            using type = typename V::type;
        };

        // A bound named argument
        template <typename Name, typename T>
        struct named
        {
            using name_type = Name;

            T&& value;
        };

        template <typename Name, typename... Nameds>
        using find_named = typename mp_map_find_impl<Name, Nameds...>::type;

        // The current arguments
        template <typename... Nameds>
        class storage : private Nameds...
        {
        public:
            // Is there already an argument for the given name?
            template <typename Name>
            static constexpr bool is_set
                = any<std::is_same<typename Nameds::name_type, Name>::value...>;

            template <typename... FNameds>
            explicit constexpr storage(FNameds&&... nameds)
                : Nameds{std::forward<FNameds>(nameds)}...
            {}

            template <typename Name, typename T>
            constexpr auto set(T&& value) &&
            {
                return storage<Nameds..., named<Name, T>>{
                    static_cast<Nameds&&>(*this)...,
                    named<Name, T>{std::forward<T>(value)},
                };
            }

#define NICKEL_DETAIL_FWD(...)                                                 \
    ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

            template <typename Name>
            constexpr decltype(auto) get() &&
            {
                using Named = find_named<Name, Nameds...>;
                // Note: Clang rejects this without the macro, GCC is fine.
                // Unsure which compiler is wrong.
                return NICKEL_DETAIL_FWD(static_cast<Named&&>(*this).value);
            }
        };

        // Provide the .name() member iff the parameter hasn't been set before.
        template <typename Storage, typename Name, typename CRTP>
        using name_member_if_unset
            = conditional_t<Storage::template is_set<Name>, type_identity<Name>,
                typename Name::template name_type<CRTP>>;

        // Enables the passing of named arguments
        template <typename Storage, typename Fn, typename... Names>
        class wrapped_fn : public name_member_if_unset<Storage, Names,
                               wrapped_fn<Storage, Fn, Names...>>...,
                           private Names...
        {
        public:
            explicit constexpr wrapped_fn(
                Storage&& storage, Fn&& fn, Names... names)
                : Names{std::move(names)}...
                , storage_{std::move(storage)}
                , fn_{std::forward<Fn>(fn)}
            {}

            // Bind the name to the argument.
            // NOT PUBLIC API
            // Note: not a named member function to avoid conflicts with
            // inherited names.
            template <typename Name, typename T>
            constexpr auto operator()(Name, T&& value) &&
            {
                using NewStorage
                    = decltype(std::move(storage_).template set<Name>(
                        std::forward<T>(value)));

                return wrapped_fn<NewStorage, Fn, Names...>{
                    std::move(storage_).template set<Name>(
                        std::forward<T>(value)),
                    std::forward<Fn>(fn_),
                    static_cast<Names&&>(*this)...,
                };
            }

            // Call the function with bound arguments.
            constexpr decltype(auto) operator()() &&
            {
                return std::forward<Fn>(fn_)(
                    std::move(storage_).template get<Names>()...);
            }

        private:
            Storage storage_;
            Fn&& fn_;
        };

        template <typename... Names>
        class partial_wrap : private Names...
        {
            template <typename T>
            static constexpr bool self
                = std::is_same<std::decay_t<T>, partial_wrap>::value;

        public:
            explicit constexpr partial_wrap(Names... names)
                : Names{std::move(names)}...
            {}

            template <typename Fn>
            constexpr auto operator()(Fn&& fn) &&
            {
                return detail::wrapped_fn<storage<>, Fn, Names...>{
                    storage<>{},
                    std::forward<Fn>(fn),
                    static_cast<Names&&>(*this)...,
                };
            }
        };
    }

    template <typename... Names>
    constexpr auto wrap(Names&&... names)
    {
        return detail::partial_wrap<std::decay_t<Names>...>{
            std::forward<Names>(names)...,
        };
    }

// Generate a new name.
// NICKEL_NAME(variable, name) generates a `variable` which has a `.name()`
// named function
#define NICKEL_NAME(variable, name)                                            \
    struct variable##_type                                                     \
    {                                                                          \
        template <typename Derived>                                            \
        struct name_type                                                       \
        {                                                                      \
            template <typename T>                                              \
            constexpr auto name(T&& value) &&                                  \
            {                                                                  \
                return static_cast<Derived&&>(*this)(                          \
                    variable##_type{}, std::forward<T>(value));                \
            }                                                                  \
        };                                                                     \
    };                                                                         \
                                                                               \
    constexpr auto variable = variable##_type                                  \
    {}

    // built-in names
    namespace names {
        NICKEL_NAME(w, w);
        NICKEL_NAME(x, x);
        NICKEL_NAME(y, y);
        NICKEL_NAME(z, z);
    }
}
