//          Copyright Justin Bassett 2019 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <tuple>
#include <type_traits>

#define NICKEL_DETAIL_FWD(...) static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)
#define NICKEL_DETAIL_MOVE(...)                                                                    \
    static_cast<::std::remove_reference_t<decltype(__VA_ARGS__)>&&>(__VA_ARGS__)

#define NICKEL_FWD NICKEL_DETAIL_FWD
#define NICKEL_MOVE NICKEL_DETAIL_MOVE

#ifdef __cpp_lib_type_trait_variable_templates
#define NICKEL_IS_VOID(...) std::is_void_v<__VA_ARGS__>
#else
#define NICKEL_IS_VOID(...) std::is_void<__VA_ARGS__>::value
#endif

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

#ifdef __cpp_lib_remove_cvref
        template <typename T>
        using remove_cvref_t = std::remove_cvref_t<T>;
#else
        template <typename T>
        using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;
#endif

        template <typename T, typename...>
        struct tag_t : tag_t<T>
        { };

        template <typename T>
        struct tag_t<T>
        {
            using type = T;
        };

        template <int N>
        struct int_t
        {
            static constexpr int value = N;
        };

        template <typename... Ts>
        struct inherit : Ts...
        { };

        // Slightly modified version of `mp_map_find` from Boost.MP11, copied
        // with permission from Peter Dimov.
        template <typename K, typename... Ts>
        struct mp_map_find_impl
        {
            using U = inherit<tag_t<Ts>...>;

            // L<K, U...> matches any template something<K, anything...>.
            // This L<K, U...> element would be one of the types in Ts....
            template <template <typename...> class L, typename... U>
            static tag_t<L<K, U...>> f(tag_t<L<K, U...>>*);
            static tag_t<void> f(...);

            // The `U*` can be converted to `tag_t` of any of the Ts...
            using V = decltype(f((U*)nullptr));

            using type = typename V::type;
        };

        // A bound named argument
        template <typename Name, typename T>
        struct named
        {
            T value;
        };

        template <typename... Names>
        struct names_t
        {
            static constexpr std::size_t count = sizeof...(Names);

            template <template <typename...> class MFn>
            using apply = MFn<Names...>;

            template <typename... Rhs>
            using append = names_t<Names..., Rhs...>;

            template <typename Rhs>
            using append_names = typename Rhs::template apply<append>;

            template <typename Fn, typename Storage, typename Defaults, typename... Extra>
            static constexpr auto map_reduce(Fn&& reduce, Storage&& storage, Defaults&& defaults,
                Extra&&... extra) -> decltype(reduce(NICKEL_FWD(extra)...,
                NICKEL_FWD(storage).get_or_default(tag_t<Names> {}, NICKEL_FWD(defaults))...))
            {
                return reduce(NICKEL_FWD(extra)...,
                    NICKEL_FWD(storage).get_or_default(tag_t<Names> {}, NICKEL_FWD(defaults))...);
            }
        };

        template <typename Name, typename... Nameds>
        using find_named = typename mp_map_find_impl<Name, Nameds...>::type;

        struct construct_tag
        { };

        struct set_tag
        { };

        struct get_tag
        { };

        template <typename Storage>
        class kwargs : private Storage
        {
        public:
            template <typename FStorage>
            explicit constexpr kwargs(construct_tag, FStorage&& storage)
                : Storage {NICKEL_FWD(storage)}
            { }

            template <typename Name>
            constexpr decltype(auto) get(Name) &
            {
                return static_cast<Storage&>(*this).get(tag_t<Name> {});
            }

            template <typename Name>
            constexpr decltype(auto) get(Name) const&
            {
                return static_cast<Storage const&>(*this).get(tag_t<Name> {});
            }

            template <typename Name>
            constexpr decltype(auto) get(Name) &&
            {
                return static_cast<Storage&&>(*this).get(tag_t<Name> {});
            }

            template <typename Name>
            constexpr decltype(auto) get(Name) const&&
            {
                return static_cast<Storage const&&>(*this).get(tag_t<Name> {});
            }

            // NOT PUBLIC API
            template <typename OtherStorage>
            constexpr auto combine(OtherStorage&& rhs) &&
            {
                return static_cast<Storage&&>(*this).combine(NICKEL_FWD(rhs));
            }

            // The following are NOT PUBLIC API:
            template <typename Name>
            constexpr decltype(auto) operator()(get_tag, Name) &
            {
                return static_cast<Storage&>(*this).get(tag_t<Name> {});
            }

            template <typename Name>
            constexpr decltype(auto) operator()(get_tag, Name) const&
            {
                return static_cast<Storage const&>(*this).get(tag_t<Name> {});
            }

            template <typename Name>
            constexpr decltype(auto) operator()(get_tag, Name) &&
            {
                return static_cast<Storage&&>(*this).get(tag_t<Name> {});
            }

            template <typename Name>
            constexpr decltype(auto) operator()(get_tag, Name) const&&
            {
                return static_cast<Storage const&&>(*this).get(tag_t<Name> {});
            }
        };

        template <typename Lambda>
        struct deferred : Lambda
        {
            template <typename FLambda>
            constexpr explicit deferred(construct_tag, FLambda&& fn)
                : Lambda(NICKEL_FWD(fn))
            { }

            using Lambda::operator();
        };

        template <typename T>
        constexpr auto get_default(T&& value) -> T&&
        {
            return NICKEL_FWD(value);
        }

        template <typename Lambda>
        constexpr auto get_default(deferred<Lambda>&& fn) -> decltype(auto)
        {
            return NICKEL_MOVE(fn)();
        }

        template <typename Lambda>
        constexpr auto get_default(deferred<Lambda> const& fn) -> decltype(auto)
        {
            return fn();
        }

        // The current arguments
        template <typename... Nameds>
        class storage : private Nameds...
        {
            template <typename Name>
            using lookup_name = find_named<Name, Nameds...>;

        public:
            // Is there already an argument for the given name?
            template <typename Name>
            static constexpr bool is_set = !NICKEL_IS_VOID(find_named<Name, Nameds...>);

            template <typename... FNameds>
            explicit constexpr storage(construct_tag, FNameds&&... nameds)
                : Nameds {NICKEL_FWD(nameds)}...
            { }

            template <typename Name, typename T>
            constexpr auto set(T&& value) &&
            {
                return storage<Nameds..., named<Name, T&&>> {
                    construct_tag {},
                    static_cast<Nameds&&>(*this)...,
                    named<Name, T&&> {NICKEL_FWD(value)},
                };
            }

            // NOT PUBLIC API
            // Sets by value, not by reference
            template <typename Name, typename T>
            constexpr auto _set_value(set_tag, T&& value) &&
            {
                return storage<Nameds..., named<Name, T>> {
                    construct_tag {},
                    static_cast<Nameds&&>(*this)...,
                    named<Name, T> {NICKEL_FWD(value)},
                };
            }

            template <typename Named>
            constexpr auto get_named() && -> Named&&
            {
                return static_cast<Named&&>(*this);
            }

            template <typename Name>
            constexpr decltype(auto) lookup_named(tag_t<Name>) &&
            {
                static_assert(is_set<Name>, "Name is not set");
                using Named = lookup_name<Name>;

                return static_cast<Named&&>(*this);
            }

            template <typename... OtherNameds>
            constexpr auto combine(storage<OtherNameds...>&& other) &&
            {
                return storage<Nameds..., OtherNameds...> {
                    construct_tag {},
                    static_cast<Nameds&&>(*this)...,
                    NICKEL_MOVE(other).template get_named<OtherNameds>()...,
                };
            }

            template <typename Name>
            constexpr decltype(auto) get(tag_t<Name> id) &&
            {
                // Note: Clang rejects this without the macro, GCC is fine.
                // Unsure which compiler is wrong.
                return NICKEL_FWD(NICKEL_MOVE(*this).lookup_named(id).value);
            }

#ifdef __cpp_if_constexpr
            template <typename Name, typename Defaults>
            constexpr decltype(auto) get_or_default(tag_t<Name> id, Defaults&& defaults) &&
            {
                if constexpr (is_set<Name>) {
                    return NICKEL_MOVE(*this).get(id);
                } else {
                    return detail::get_default(NICKEL_FWD(defaults).get(id));
                }
            }
#else
            template <typename Name, typename Defaults>
            constexpr decltype(auto) get_or_default_(
                std::false_type, tag_t<Name> id, Defaults&& defaults) &&
            {
                return detail::get_default(NICKEL_FWD(defaults).get(id));
            }

            template <typename Name, typename Defaults>
            constexpr decltype(auto) get_or_default_(std::true_type, tag_t<Name> id, Defaults&&) &&
            {
                return NICKEL_MOVE(*this).get(id);
            }

            template <typename Name, typename Defaults>
            constexpr decltype(auto) get_or_default(tag_t<Name> id, Defaults&& defaults) &&
            {
                return NICKEL_MOVE(*this).get_or_default_(
                    std::integral_constant<bool, is_set<Name>> {}, id, NICKEL_FWD(defaults));
            }
#endif

            // NOT PUBLIC API
            template <typename... Names, typename Defaults>
            constexpr decltype(auto) get(tag_t<names_t<Names...>>, Defaults&& defaults) &&
            {
                using kwargs_storage_t = storage<lookup_name<Names>...>;
                return kwargs<kwargs_storage_t> {
                    construct_tag {},
                    kwargs_storage_t {
                        construct_tag {},
                        NICKEL_MOVE(*this).get_or_default(tag_t<Names> {}, NICKEL_FWD(defaults))...,
                    },
                };
            }
        };

        // Provide the .<name>() member iff the parameter hasn't been set before.
        template <typename Storage, typename Name, typename CRTP>
        using allow_set_only_if_unset = conditional_t<Storage::template is_set<Name>, tag_t<Name>,
            typename Name::template set_type<CRTP>>;

        template <typename Derived, typename Storage, typename Kwargs, typename Names>
        struct wrapped_fn_base;

        template <typename Derived, typename Storage, typename... Kwargs, typename... Names>
        struct wrapped_fn_base<Derived, Storage, names_t<Kwargs...>, names_t<Names...>>
            : public allow_set_only_if_unset<Storage, Kwargs, Derived>...,
              public allow_set_only_if_unset<Storage, Names, Derived>...
        { };

        template <typename Defaults, typename Storage, typename Fn, typename Kwargs, typename Names>
        class wrapped_fn : public wrapped_fn_base<wrapped_fn<Defaults, Storage, Fn, Kwargs, Names>,
                               Storage, Kwargs, Names>
        {
        private:
            Defaults defaults_;
            Storage storage_;
            Fn fn_;

        public:
            template <typename FDefaults, typename FFn>
            explicit constexpr wrapped_fn(FDefaults&& defaults, Storage&& storage, FFn&& fn)
                : defaults_ {NICKEL_FWD(defaults)}
                , storage_ {NICKEL_MOVE(storage)}
                , fn_ {NICKEL_FWD(fn)}
            { }

            // Bind the name to the multi-valued argument.
            // NOT PUBLIC API
            template <typename Name, int N, typename... Ts>
            constexpr auto operator()(set_tag, Name, int_t<N>, Ts&&... values) &&
            {
                using NewStorage = decltype(NICKEL_MOVE(storage_).template _set_value<Name>(
                    set_tag {}, std::tuple<Ts&&...>(NICKEL_FWD(values)...)));
                return wrapped_fn<Defaults, NewStorage, remove_cvref_t<Fn>, Kwargs, Names> {
                    NICKEL_MOVE(defaults_),
                    NICKEL_MOVE(storage_).template _set_value<Name>(
                        set_tag {}, std::tuple<Ts&&...>(NICKEL_FWD(values)...)),
                    NICKEL_MOVE(fn_),
                };
            }

            // Bind the name to the single argument.
            // NOT PUBLIC API
            template <typename Name, typename T>
            constexpr auto operator()(set_tag, Name, int_t<-1>, T&& value) &&
            {
                using NewStorage
                    = decltype(NICKEL_MOVE(storage_).template set<Name>(NICKEL_FWD(value)));
                return wrapped_fn<Defaults, NewStorage, remove_cvref_t<Fn>, Kwargs, Names> {
                    NICKEL_MOVE(defaults_),
                    NICKEL_MOVE(storage_).template set<Name>(NICKEL_FWD(value)),
                    NICKEL_MOVE(fn_),
                };
            }

            // Bind the kwargs
            // NOT PUBLIC API
            template <typename OtherStorage>
            constexpr auto operator()(kwargs<OtherStorage>&& kwargs) &&
            {
                using NewStorage = decltype(NICKEL_MOVE(kwargs).combine(NICKEL_MOVE(storage_)));
                return wrapped_fn<Defaults, NewStorage, remove_cvref_t<Fn>, Kwargs, Names> {
                    NICKEL_MOVE(defaults_),
                    NICKEL_MOVE(kwargs).combine(NICKEL_MOVE(storage_)),
                    NICKEL_MOVE(fn_),
                };
            }

#define NICKEL_CALL_WITH_KWARGS()                                                                  \
    Names::map_reduce(NICKEL_MOVE(fn_), NICKEL_MOVE(storage_), NICKEL_MOVE(defaults_),             \
        NICKEL_MOVE(storage_).get(detail::tag_t<Kwargs> {}, NICKEL_MOVE(defaults_)))
#define NICKEL_CALL_NO_KWARGS()                                                                    \
    Names::map_reduce(NICKEL_MOVE(fn_), NICKEL_MOVE(storage_), NICKEL_MOVE(defaults_))

#ifdef __cpp_if_constexpr
            // Call the function with bound arguments.
            constexpr decltype(auto) operator()() &&
            {
                if constexpr (Kwargs::count != 0) {
                    return NICKEL_CALL_WITH_KWARGS();
                } else {
                    return NICKEL_CALL_NO_KWARGS();
                }
            }
#else
        private:
            constexpr decltype(auto) do_call(std::true_type) &&
            {
                return NICKEL_CALL_WITH_KWARGS();
            }

            constexpr decltype(auto) do_call(std::false_type) &&
            {
                return NICKEL_CALL_NO_KWARGS();
            }

        public:
            // Call the function with bound arguments.
            constexpr decltype(auto) operator()() &&
            {
                return NICKEL_MOVE(*this).do_call(
                    std::integral_constant<bool, Kwargs::count != 0> {});
            }
#endif
#undef NICKEL_CALL_WITH_KWARGS
#undef NICKEL_CALL_NO_KWARGS
        };

        template <typename Defaults, typename Kwargs, typename Names>
        class partial_wrap : private Defaults
        {
        public:
            template <typename FDefaults>
            explicit constexpr partial_wrap(construct_tag, FDefaults&& defaults)
                : Defaults {NICKEL_FWD(defaults)}
            { }

            template <typename Fn>
            constexpr auto operator()(Fn&& fn) &&
            {
                using DFn = remove_cvref_t<Fn>;

                return wrapped_fn<Defaults, storage<>, DFn, Kwargs, Names> {
                    Defaults {static_cast<Defaults&&>(*this)},
                    storage<> {construct_tag {}},
                    NICKEL_FWD(fn),
                };
            }
        };

        template <typename Name, typename Value>
        struct defaulted
        {
            using name_type = Name;

            Value value;
        };

        struct name_group_to_partial_fn_tag
        { };

        struct mark_kwargs_tag
        { };

        template <typename Defaults, typename Kwargs, typename Names>
        class name_group : private Defaults
        {
        public:
            template <typename FDefaults>
            explicit constexpr name_group(construct_tag, FDefaults&& defaults)
                : Defaults {NICKEL_FWD(defaults)}
            { }

            template <typename OtherDefaults, typename OtherKwargs, typename OtherNames>
            constexpr auto combine(name_group<OtherDefaults, OtherKwargs, OtherNames>&& group) &&
            {
                using combined_defaults = remove_cvref_t<decltype(
                    static_cast<Defaults&&>(*this).combine((OtherDefaults &&) group))>;

                return name_group<combined_defaults,
                    typename Kwargs::template append_names<OtherKwargs>,
                    typename Names::template append_names<OtherNames>> {
                    construct_tag {},
                    static_cast<Defaults&&>(*this).combine((OtherDefaults &&) group),
                };
            }

            constexpr auto operator()(name_group_to_partial_fn_tag) &&
            {
                return detail::partial_wrap<Defaults, Kwargs, Names> {
                    construct_tag {},
                    static_cast<Defaults&&>(*this),
                };
            }

            constexpr auto _mark_all_kwargs(mark_kwargs_tag) &&
            {
                return name_group<Defaults, typename Kwargs::template append_names<Names>,
                    names_t<>> {
                    construct_tag {},
                    static_cast<Defaults&&>(*this),
                };
            }
        };

        template <typename Storage>
        constexpr auto make_default_storage(Storage&& storage)
        {
            return NICKEL_FWD(storage);
        }

        template <typename Storage, typename Name, typename Value, typename... DefaultedNames>
        constexpr auto make_default_storage(
            Storage&& storage, defaulted<Name, Value> defaulted_arg, DefaultedNames&&... names)
        {
            return make_default_storage(NICKEL_FWD(storage).template _set_value<Name>(
                                            set_tag {}, NICKEL_MOVE(defaulted_arg).value),
                NICKEL_FWD(names)...);
        }

        template <typename Storage, typename DefaultedName, typename... DefaultedNames>
        constexpr auto make_default_storage(
            Storage&& storage, DefaultedName&&, DefaultedNames&&... names)
        {
            return make_default_storage(NICKEL_FWD(storage), NICKEL_FWD(names)...);
        }
    }

    template <typename... DefaultedNames>
    constexpr auto name_group(DefaultedNames&&... names)
    {
        auto defaults = detail::make_default_storage(
            detail::storage<> {detail::construct_tag {}}, NICKEL_FWD(names)...);
        return detail::name_group<decltype(defaults), detail::names_t<>,
            detail::names_t<typename detail::remove_cvref_t<DefaultedNames>::name_type...>> {
            detail::construct_tag {},
            NICKEL_MOVE(defaults),
        };
    }

    template <typename Defaults, typename Kwargs, typename Names, typename... Rest>
    constexpr auto name_group(detail::name_group<Defaults, Kwargs, Names> group, Rest&&... names)
    {
        return NICKEL_MOVE(group).combine(nickel::name_group(NICKEL_FWD(names)...));
    }

    template <typename... Names>
    constexpr auto kwargs_group(Names&&... names)
    {
        auto ng = nickel::name_group(NICKEL_FWD(names)...);
        return NICKEL_MOVE(ng)._mark_all_kwargs(detail::mark_kwargs_tag {});
    }

    template <typename... Names>
    constexpr auto wrap(Names&&... names)
    {
        return nickel::name_group(NICKEL_FWD(names)...)(detail::name_group_to_partial_fn_tag {});
    }

    template <typename Lambda>
    constexpr auto deferred(Lambda&& fn)
    {
        return detail::deferred<detail::remove_cvref_t<Lambda>>(
            detail::construct_tag {}, NICKEL_FWD(fn));
    }

// Generate a new name.
// NICKEL_NAME(variable, name) generates a `variable` which has a `.name()`
// named function
#define NICKEL_NAME(variable, name)                                                                \
    template <int N = -1>                                                                          \
    struct variable##_type                                                                         \
    {                                                                                              \
        using name_type = variable##_type;                                                         \
        template <typename Derived>                                                                \
        struct set_type                                                                            \
        {                                                                                          \
            template <typename... Ts>                                                              \
            constexpr auto name(Ts&&... values) &&                                                 \
            {                                                                                      \
                static_assert(sizeof...(Ts) == N || sizeof...(Ts) == 1 && N == -1,                 \
                    "Must call the function with the specified arguments: " #name);                \
                return static_cast<Derived&&>(*this)(::nickel::detail::set_tag {},                 \
                    variable##_type {}, ::nickel::detail::int_t<N> {},                             \
                    NICKEL_DETAIL_FWD(values)...);                                                 \
            }                                                                                      \
        };                                                                                         \
                                                                                                   \
        template <typename Derived>                                                                \
        struct get_type                                                                            \
        {                                                                                          \
            constexpr decltype(auto) name() &                                                      \
            {                                                                                      \
                return static_cast<Derived&>(*this)(                                               \
                    ::nickel::detail::get_tag {}, variable##_type {});                             \
            }                                                                                      \
            constexpr decltype(auto) name() const&                                                 \
            {                                                                                      \
                return static_cast<Derived const&>(*this)(                                         \
                    ::nickel::detail::get_tag {}, variable##_type {});                             \
            }                                                                                      \
            constexpr decltype(auto) name() &&                                                     \
            {                                                                                      \
                return static_cast<Derived&&>(*this)(                                              \
                    ::nickel::detail::get_tag {}, variable##_type {});                             \
            }                                                                                      \
            constexpr decltype(auto) name() const&&                                                \
            {                                                                                      \
                return static_cast<Derived const&&>(*this)(                                        \
                    ::nickel::detail::get_tag {}, variable##_type {});                             \
            }                                                                                      \
        };                                                                                         \
                                                                                                   \
        template <typename T>                                                                      \
        constexpr auto operator=(T&& value) const                                                  \
        {                                                                                          \
            return ::nickel::detail::defaulted<variable##_type,                                    \
                ::nickel::detail::remove_cvref_t<T>> {                                             \
                NICKEL_DETAIL_FWD(value),                                                          \
            };                                                                                     \
        }                                                                                          \
                                                                                                   \
        template <int NArgs>                                                                       \
        constexpr auto multivalued() const -> variable##_type<NArgs>                               \
        {                                                                                          \
            static_assert(NArgs >= 0, "Cannot ask for a negative number of args");                 \
            return {};                                                                             \
        }                                                                                          \
    };                                                                                             \
                                                                                                   \
    constexpr auto variable = variable##_type<>                                                    \
    { }
}

#undef NICKEL_FWD
#undef NICKEL_MOVE
#undef NICKEL_IS_VOID
