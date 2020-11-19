#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

#define NICKEL_DETAIL_FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

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

        template <typename T>
        using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

        template <typename T, typename...>
        struct tag_t : tag_t<T>
        { };

        template <typename T>
        struct tag_t<T>
        {
            using type = T;
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

        template <typename NameGroup>
        class kwargs_group : private NameGroup
        {
            template <typename T>
            static constexpr bool self = std::is_same<remove_cvref_t<T>, kwargs_group>::value;

        public:
            template <typename FNameGroup, std::enable_if_t<!self<FNameGroup>, int> = 0>
            explicit constexpr kwargs_group(FNameGroup&& group)
                : NameGroup {NICKEL_DETAIL_FWD(group)}
            { }

            template <typename OtherNameGroup>
            constexpr auto combine(kwargs_group<OtherNameGroup>&& group) &&
            {
                auto&& ng = static_cast<NameGroup&&>(*this);
                auto&& other_ng = (OtherNameGroup &&) group;
                return kwargs_group<
                    remove_cvref_t<decltype(std::move(ng).combine(std::move(other_ng)))>> {
                    std::move(ng).combine(std::move(other_ng)),
                };
            }

            // // NOT PUBLIC API
            // template <typename Derived>
            // struct name_type : public Names::name_type<Derived>...
            // { };
        };

        // NOT PUBLIC API
        template <typename... Nameds>
        class kwargs : private Nameds...
        {
        public:
            template <typename... Values>
            explicit constexpr kwargs(Values&&... values)
                : Nameds {std::forward<Values>(values)}...
            { }

            template <typename Named>
            constexpr auto _get(tag_t<Named>) &&
            {
                return NICKEL_DETAIL_FWD(static_cast<Named&&>(*this));
            }
        };

        template <typename Name, typename Value>
        struct defaulted : named<Name, Value>
        {
            using base_name = Name;

            template <typename Derived>
            using name_type = typename Name::template name_type<Derived>;
        };

        template <typename Name, typename... Nameds>
        using find_named = typename mp_map_find_impl<Name, Nameds...>::type;

        template <typename Defaults, typename... Nameds>
        class storage;

        struct nil_storage
        {
            template <typename Defaults, typename... Names>
            constexpr auto combine(
                storage<Defaults, Names...>&& other) && -> storage<Defaults, Names...>&&;

            constexpr auto combine(nil_storage) &&
            {
                return nil_storage {};
            }
        };

        // The current arguments
        template <typename Defaults, typename... Nameds>
        class storage : private Defaults, private Nameds...
        {
            template <typename T>
            static constexpr bool self = std::is_same<remove_cvref_t<T>, storage>::value;

            template <typename Name>
            using lookup_name = find_named<Name, Nameds...>;

        public:
            // Is there already an argument for the given name?
            template <typename Name>
            static constexpr bool is_set = !std::is_void<find_named<Name, Nameds...>>::value;

            template <typename FDefaults, typename... FNameds,
                std::enable_if_t<!self<FDefaults>, int> = 0>
            explicit constexpr storage(FDefaults&& defaults, FNameds&&... nameds)
                : Defaults {NICKEL_DETAIL_FWD(defaults)}
                , Nameds {NICKEL_DETAIL_FWD(nameds)}...
            { }

            template <typename Name, typename T>
            constexpr auto set(T&& value) &&
            {
                return storage<Defaults, Nameds..., named<Name, T&&>> {
                    static_cast<Defaults&&>(*this),
                    static_cast<Nameds&&>(*this)...,
                    named<Name, T&&> {NICKEL_DETAIL_FWD(value)},
                };
            }

            constexpr auto combine(nil_storage) && -> storage<Defaults, Nameds...>&&
            {
                return std::move(*this);
            }

            template <typename OtherDefaults, typename... OtherNameds>
            constexpr auto combine(storage<OtherDefaults, OtherNameds...>&& other) &&
            {
                auto&& my_defaults = static_cast<Defaults&&>(*this);
                auto&& other_defaults = (OtherDefaults &&)(other);

                using combined_defaults = remove_cvref_t<decltype(
                    std::move(my_defaults).combine(std::move(other_defaults)))>;

                return storage<combined_defaults, Nameds..., OtherNameds...> {
                    std::move(my_defaults).combine(std::move(other_defaults)),
                    static_cast<Nameds&&>(*this)...,
                    (OtherNameds &&)(other)...,
                };
            }

            template <typename Name>
            constexpr decltype(auto) _get(tag_t<Name> id) &&
            {
                using Named = lookup_name<Name>;

                if constexpr (std::is_same<Named, void>::value) {
                    return static_cast<Defaults&&>(*this)._get(id);
                } else {
                    return static_cast<Named&&>(*this);
                }
            }

            template <typename Name>
            constexpr decltype(auto) get(tag_t<Name> id) &&
            {
                // Note: Clang rejects this without the macro, GCC is fine.
                // Unsure which compiler is wrong.
                return NICKEL_DETAIL_FWD(std::move(*this)._get(id).value);
            }

            template <typename... Names>
            constexpr decltype(auto) get(tag_t<kwargs_group<Names...>>) &&
            {
                using kwargs_t = kwargs<lookup_name<Names>...>;
                return kwargs_t {
                    std::move(*this)._get(tag_t<Names> {})...,
                };
            }
        };

        template <typename Defaults, typename... Nameds>
        constexpr auto nil_storage::combine(
            storage<Defaults, Nameds...>&& other) && -> storage<Defaults, Nameds...>&&
        {
            return std::move(other);
        }

        // Provide the .name() member iff the parameter hasn't been set before.
        template <typename Storage, typename Name, typename CRTP>
        using name_member_if_unset = conditional_t<Storage::template is_set<Name>, tag_t<Name>,
            typename Name::template name_type<CRTP>>;

        // Enables the passing of named arguments
        template <typename Storage, typename Fn, typename... Names>
        class wrapped_fn
            : public name_member_if_unset<Storage, Names, wrapped_fn<Storage, Fn, Names...>>...,
              private Names...
        {
        public:
            template <typename FFn>
            explicit constexpr wrapped_fn(Storage&& storage, FFn&& fn, Names... names)
                : Names {std::move(names)}...
                , storage_ {std::move(storage)}
                , fn_ {NICKEL_DETAIL_FWD(fn)}
            { }

            // Bind the name to the argument.
            // NOT PUBLIC API
            // Note: not a named member function to avoid conflicts with
            // inherited names.
            template <typename Name, typename T>
            constexpr auto operator()(Name, T&& value) &&
            {
                using NewStorage
                    = decltype(std::move(storage_).template set<Name>(NICKEL_DETAIL_FWD(value)));

                return wrapped_fn<NewStorage, remove_cvref_t<Fn>, Names...> {
                    std::move(storage_).template set<Name>(NICKEL_DETAIL_FWD(value)),
                    std::move(fn_),
                    static_cast<Names&&>(*this)...,
                };
            }

            // Bind the name to the argument.
            // NOT PUBLIC API
            // Note: not a named member function to avoid conflicts with
            // inherited names.
            template <typename Name, typename T>
            constexpr auto operator()(tag_t<Name>, T&& value) &&
            {
                using NewStorage
                    = decltype(std::move(storage_).template set<Name>(NICKEL_DETAIL_FWD(value)));

                return wrapped_fn<NewStorage, remove_cvref_t<Fn>, Names...> {
                    std::move(storage_).template set<Name>(NICKEL_DETAIL_FWD(value)),
                    std::move(fn_),
                    static_cast<Names&&>(*this)...,
                };
            }

            // NOT PUBLIC API
            template <typename Name, typename T>
            constexpr auto operator->*(named<Name, T> value) &&
            {
                return std::move(*this)(tag_t<Name> {}, std::move(value).value);
            }

#ifdef __cpp_fold_expressions
            template <typename... KwargsNameds>
            constexpr auto operator()(kwargs<KwargsNameds...>&& kwargs_) &&
            {
                return (std::move(*this)->*...->*std::move(kwargs_)._get(tag_t<KwargsNameds> {}));
            }
#endif

            // Call the function with bound arguments.
            constexpr decltype(auto) operator()() &&
            {
                return std::move(fn_)(std::move(storage_).get(tag_t<Names> {})...);
            }

        private:
            Storage storage_;
            Fn fn_;
        };

        template <typename Defaults, typename... Names>
        class partial_wrap : private Defaults
        {
            template <typename T>
            static constexpr bool self = std::is_same<remove_cvref_t<T>, partial_wrap>::value;

        public:
            template <typename FDefaults, std::enable_if_t<!self<FDefaults>, int> = 0>
            explicit constexpr partial_wrap(FDefaults&& defaults)
                : Defaults {NICKEL_DETAIL_FWD(defaults)}
            { }

            template <typename Fn>
            constexpr auto operator()(Fn&& fn) &&
            {
                using DFn = remove_cvref_t<Fn>;

                return detail::wrapped_fn<storage<Defaults>, DFn, Names...> {
                    storage<Defaults> {static_cast<Defaults&&>(*this)},
                    NICKEL_DETAIL_FWD(fn),
                    Names {}...,
                };
            }
        };

        struct name_group_expand_into_tag
        { };

        struct name_group_to_partial_fn_tag
        { };

        template <typename Defaults, typename... Names>
        class name_group : private Defaults
        {
            template <typename T>
            static constexpr bool self = std::is_same<remove_cvref_t<T>, name_group>::value;

        public:
            template <typename FDefaults, std::enable_if_t<!self<FDefaults>, int> = 0>
            explicit constexpr name_group(FDefaults&& defaults)
                : Defaults {NICKEL_DETAIL_FWD(defaults)}
            { }

            template <typename OtherDefaults, typename... OtherNames>
            constexpr auto combine(name_group<OtherDefaults, OtherNames...>&& group) &&
            {
                using combined_defaults = remove_cvref_t<decltype(
                    static_cast<Defaults&&>(*this).combine((OtherDefaults &&) group))>;

                return name_group<combined_defaults, Names..., OtherNames...> {
                    static_cast<Defaults&&>(*this).combine((OtherDefaults &&) group),
                };
            }

            template <typename Fn>
            constexpr auto operator()(name_group_expand_into_tag, Fn f) &&
            {
                return f(tag_t<Names> {}...);
            }

            constexpr auto operator()(name_group_to_partial_fn_tag) &&
            {
                return std::move(*this)(name_group_expand_into_tag {}, [&](auto... names) {
                    return detail::partial_wrap<Defaults, typename decltype(names)::type...> {
                        static_cast<Defaults&&>(*this),
                    };
                });
            }
        };

        template <typename Name, typename Value>
        constexpr auto extracted_default(defaulted<Name, Value>) -> named<Name, Value>;

        template <typename Name>
        constexpr auto extracted_default(Name) -> tag_t<void, Name>;

        template <typename Name, typename Value>
        constexpr auto extracted_name(defaulted<Name, Value>) -> Name;

        template <typename Name>
        constexpr auto extracted_name(Name) -> Name;

        template <typename Name, typename Value>
        constexpr auto extract_default(defaulted<Name, Value>&& defaulted) -> named<Name, Value>&&
        {
            return std::move(defaulted);
        }

        template <typename Name, typename Value>
        constexpr auto extract_default(defaulted<Name, Value> const& defaulted)
            -> named<Name, Value> const&
        {
            return defaulted;
        }

        template <typename Name>
        constexpr auto extract_default(Name) -> tag_t<void, Name>
        {
            return {};
        }

        template <typename... Names>
        constexpr auto extract_defaults(Names&&... names)
        {
            return storage<nil_storage, decltype(detail::extracted_default(names))...> {
                nil_storage {},
                detail::extract_default(NICKEL_DETAIL_FWD(names))...,
            };
        }
    }

    template <typename... Names>
    constexpr auto name_group(Names&&... names)
    {
        return detail::name_group<decltype(detail::extract_defaults(NICKEL_DETAIL_FWD(names)...)),
            decltype(detail::extracted_name(names))...> {
            detail::extract_defaults(NICKEL_DETAIL_FWD(names)...),
        };
    }

    template <typename... Names, typename... Rest>
    constexpr auto name_group(detail::name_group<Names...> group, Rest&&... names)
    {
        return std::move(group).combine(nickel::name_group(NICKEL_DETAIL_FWD(names)...));
    }

    template <typename... Names>
    constexpr auto kwargs_group(Names&&... names)
    {
        auto ng = nickel::name_group(NICKEL_DETAIL_FWD(names)...);
        return detail::kwargs_group<decltype(ng)> {std::move(ng)};
    }

    template <typename... Names, typename... Rest>
    constexpr auto kwargs_group(detail::kwargs_group<Names...> kwargs, Rest&&... names)
    {
        return std::move(kwargs).combine(nickel::kwargs_group(NICKEL_DETAIL_FWD(names)...));
    }

    template <typename... Names>
    constexpr auto wrap(Names&&... names)
    {
        return nickel::name_group(NICKEL_DETAIL_FWD(names)...)(
            detail::name_group_to_partial_fn_tag {});
    }

    template <typename... Names1, typename... Names2, typename... Rest>
    constexpr auto wrap(detail::kwargs_group<Names1...> kwargs1,
        detail::kwargs_group<Names1...> kwargs2, Rest&&... names)
    {
        return nickel::wrap(
            std::move(kwargs1).combine(std::move(kwargs2)), NICKEL_DETAIL_FWD(names)...);
    }

// Generate a new name.
// NICKEL_NAME(variable, name) generates a `variable` which has a `.name()`
// named function
#define NICKEL_NAME(variable, name)                                                                \
    struct variable##_type                                                                         \
    {                                                                                              \
        template <typename Derived>                                                                \
        struct name_type                                                                           \
        {                                                                                          \
            template <typename T>                                                                  \
            constexpr auto name(T&& value) &&                                                      \
            {                                                                                      \
                return static_cast<Derived&&>(*this)(                                              \
                    variable##_type {}, NICKEL_DETAIL_FWD(value));                                 \
            }                                                                                      \
        };                                                                                         \
                                                                                                   \
        template <typename T>                                                                      \
        constexpr auto operator=(T&& value) const                                                  \
        {                                                                                          \
            return ::nickel::detail::defaulted<variable##_type,                                    \
                ::nickel::detail::remove_cvref_t<T>> {NICKEL_DETAIL_FWD(value)};                   \
        }                                                                                          \
    };                                                                                             \
                                                                                                   \
    constexpr auto variable = variable##_type                                                      \
    { }

    // built-in names
    namespace names {
        NICKEL_NAME(w, w);
        NICKEL_NAME(x, x);
        NICKEL_NAME(y, y);
        NICKEL_NAME(z, z);
    }
}
