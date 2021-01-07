//          Copyright Justin Bassett 2019 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#ifndef NICKEL_H_5C149283
#define NICKEL_H_5C149283

// If you are looking for documentation on how to use Nickel, prefer the out-of-code documentation.
// The documentation in this header file is primarily on the internal details of working _on_ Nickel
// rather than working _with_ nickel. Even so, after the close of the `detail` namespace, you can
// find documentation on using Nickel.

#include <memory> // std::addressof
#include <tuple>
#include <type_traits>

#if defined(__clang__)
#    define NICKEL_DETAIL_IS_CLANG
#elif defined(__GNUC__)
#    define NICKEL_DETAIL_IS_GCC
#elif defined(_MSC_VER)
#    define NICKEL_DETAIL_IS_MSVC
#endif

#define NICKEL_DETAIL_STR2(...) #__VA_ARGS__
#define NICKEL_DETAIL_STR(...) NICKEL_DETAIL_STR2(__VA_ARGS__)

#ifdef NICKEL_DETAIL_IS_CLANG
#    define NICKEL_DETAIL_CLANG_PRAGMA(...) _Pragma(NICKEL_DETAIL_STR(__VA_ARGS__))
#else
#    define NICKEL_DETAIL_CLANG_PRAGMA(...)
#endif
#define NICKEL_DETAIL_CLANG_WIGNORE(warning, ...)                                                  \
    NICKEL_DETAIL_CLANG_PRAGMA(clang diagnostic push)                                              \
    NICKEL_DETAIL_CLANG_PRAGMA(clang diagnostic ignored warning)                                   \
    __VA_ARGS__ NICKEL_DETAIL_CLANG_PRAGMA(clang diagnostic pop)

#ifdef NICKEL_DETAIL_IS_GCC
#    define NICKEL_DETAIL_GCC_PRAGMA(...) _Pragma(NICKEL_DETAIL_STR(__VA_ARGS__))
#else
#    define NICKEL_DETAIL_GCC_PRAGMA(...)
#endif
#define NICKEL_DETAIL_GCC_WIGNORE(warning, ...)                                                    \
    NICKEL_DETAIL_GCC_PRAGMA(GCC diagnostic push)                                                  \
    NICKEL_DETAIL_GCC_PRAGMA(GCC diagnostic ignored warning)                                       \
    __VA_ARGS__ NICKEL_DETAIL_GCC_PRAGMA(GCC diagnostic pop)

#ifdef NICKEL_DETAIL_IS_MSVC
#    define NICKEL_DETAIL_MSVC_PRAGMA(...) _Pragma(NICKEL_DETAIL_STR(__VA_ARGS__))
#else
#    define NICKEL_DETAIL_MSVC_PRAGMA(...)
#endif

#define NICKEL_CLANG_PRAGMA NICKEL_DETAIL_CLANG_PRAGMA
#define NICKEL_CLANG_WIGNORE NICKEL_DETAIL_CLANG_WIGNORE
#define NICKEL_GCC_PRAGMA NICKEL_DETAIL_GCC_PRAGMA
#define NICKEL_GCC_WIGNORE NICKEL_DETAIL_GCC_WIGNORE
#define NICKEL_MSVC_PRAGMA NICKEL_DETAIL_MSVC_PRAGMA

#ifdef __has_cpp_attribute
#    if __has_cpp_attribute(nodiscard) >= 201907
#        define NICKEL_DETAIL_NODISCARD(...)                                                       \
            NICKEL_CLANG_WIGNORE("-Wc++20-extensions", [[nodiscard(__VA_ARGS__)]])
#    elif __has_cpp_attribute(nodiscard)
#        define NICKEL_DETAIL_NODISCARD(...)                                                       \
            NICKEL_CLANG_WIGNORE("-Wc++17-extensions", [[nodiscard]])
#    else
#        define NICKEL_DETAIL_NODISCARD(...)
#    endif
#else
#    define NICKEL_DETAIL_NODISCARD(...)
#endif

#define NICKEL_NODISCARD NICKEL_DETAIL_NODISCARD

// std::forward
#define NICKEL_DETAIL_FWD(...) static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)
// std::move
#define NICKEL_DETAIL_MOVE(...)                                                                    \
    static_cast<::std::remove_reference_t<decltype(__VA_ARGS__)>&&>(__VA_ARGS__)

// Shortcuts which only exist inside this header; they are #undef'd at the end.
#define NICKEL_FWD NICKEL_DETAIL_FWD
#define NICKEL_MOVE NICKEL_DETAIL_MOVE

// Wraps std::trait_v<...> to work pre-C++17.
#ifdef __cpp_lib_type_trait_variable_templates
#    define NICKEL_IS_VOID(...) std::is_void_v<__VA_ARGS__>
#    define NICKEL_IS_SAME(...) std::is_same_v<__VA_ARGS__>
#    define NICKEL_IS_RVALUE_REFERENCE(...) std::is_rvalue_reference_v<__VA_ARGS__>
#else
#    define NICKEL_IS_VOID(...) std::is_void<__VA_ARGS__>::value
#    define NICKEL_IS_SAME(...) std::is_same<__VA_ARGS__>::value
#    define NICKEL_IS_RVALUE_REFERENCE(...) std::is_rvalue_reference<__VA_ARGS__>::value
#endif

namespace nickel {
    // Internal implementation details of Nickel.
    namespace detail {
        // Implement an equivalent of std::conditional_t which is more efficient,
        // because there's only ever two instantiations of the type as compared to an instantiation
        // for every combination of `bool`, Type1, Type2.
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

        // std::remove_cvref_t polyfill
#ifdef __cpp_lib_remove_cvref
        template <typename T>
        using remove_cvref_t = std::remove_cvref_t<T>;
#else
        template <typename T>
        using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;
#endif

        // Allows tag dispatch on a type `T`
        template <typename T>
        struct tag_t
        {
            using type = T;
        };

        // Allows tag dispatch on an int. std::integral_constant, but nicer.
        template <int N>
        struct int_t
        {
            static constexpr int value = N;
        };

        // Used to implement mp_map_find, simply inherits from all the types.
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

        // A function taking a priv_tag is effectively private.
        // We use this because several Nickel types can have arbitrary member functions (from the
        // user-specified names), so we need an unambiguous way to refer to _our_ functions.
        struct priv_tag
        {
            // Constructor marked explicit to prevent empty brace initialization.
            explicit constexpr priv_tag()
            { }
        };

        // A bound named argument
        template <typename Name, typename T>
        struct named
        {
            // The bound value. May be a reference (in fact, often is a reference).
            T value;
        };

        // A metaprogramming list of names.
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

            // TODO: figure out what this is and where it belongs.
            template <typename Fn, typename Storage, typename Defaults, typename... Extra>
            static constexpr auto map_reduce(Fn&& reduce, Storage&& storage, Defaults&& defaults,
                Extra&&... extra) -> decltype(reduce(NICKEL_FWD(extra)...,
                NICKEL_FWD(storage).get_or_default(tag_t<Names> {}, NICKEL_FWD(defaults))...))
            {
                return reduce(NICKEL_FWD(extra)...,
                    NICKEL_FWD(storage).get_or_default(tag_t<Names> {}, NICKEL_FWD(defaults))...);
            }
        };

        // Finds the named<X, Y> in `Nameds` corresponding to `Name`
        template <typename Name, typename... Nameds>
        using find_named = typename mp_map_find_impl<Name, Nameds...>::type;

        // Marks a constructor.
        // This eliminates the need to use SFINAE to prevent a constructor from subsuming the
        // copy/move constructors.
        struct construct_tag
        { };

        // Marks a "set" action as requested by a name.
        struct set_tag
        { };

        // Marks a "get" action as requested by a name.
        struct get_tag
        { };

        // The "kwargs" variable which gets passed to the user's function definition.
        // Holds arguments which are desired to be passed on to further named-parameter functions.
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
            // TODO: Find a way to _not_ expose this.
            // Create a `storage<...>` with our bound names AND rhs's bound names.
            template <typename OtherStorage>
            constexpr auto combine(OtherStorage&& rhs) &&
            {
                return static_cast<Storage&&>(*this).combine(NICKEL_FWD(rhs));
            }

            // The following are NOT PUBLIC API:
            // TODO: Find a way to _not_ expose this.
            // TODO: figure out why this is desired, and why it isn't public API.
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

        // An unevaluated default argument value
        template <typename Lambda>
        struct deferred : Lambda
        {
            template <typename FLambda>
            constexpr explicit deferred(construct_tag, FLambda&& fn)
                : Lambda(NICKEL_FWD(fn))
            { }

            using Lambda::operator();
        };

        // Extracts the value in a default argument.
        // These functions exist to enable first-class support for the deferred<...> type.
        template <typename T>
        constexpr auto get_default(T&& value) -> T&&
        {
            return NICKEL_FWD(value);
        }

        // Deferred default arguments get called here.
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

        // The currently bound names; that is, the bound arguments.
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

            // Binds `Name` to `value`.
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
            // Like `set()`, but forces a value instead of a reference.
            template <typename Name, typename T>
            constexpr auto _set_value(set_tag, T&& value) &&
            {
                return storage<Nameds..., named<Name, T>> {
                    construct_tag {},
                    static_cast<Nameds&&>(*this)...,
                    named<Name, T> {NICKEL_FWD(value)},
                };
            }

            // Retrieves the requested named<Name, T>
            template <typename Named>
            constexpr auto get_named() && -> Named&&
            {
                return static_cast<Named&&>(*this);
            }

            // Retrieves the named<Name, T> corresponding with Name.
            template <typename Name>
            constexpr decltype(auto) lookup_named(tag_t<Name>) &&
            {
                static_assert(is_set<Name>, "Name is not set");
                using Named = lookup_name<Name>;

                return static_cast<Named&&>(*this);
            }

            // Combines our bound arguments with `other`'s bound arguments, producing a `storage<>`
            // with both.
            template <typename... OtherNameds>
            constexpr auto combine(storage<OtherNameds...>&& other) &&
            {
                return storage<Nameds..., OtherNameds...> {
                    construct_tag {},
                    static_cast<Nameds&&>(*this)...,
                    NICKEL_MOVE(other).template get_named<OtherNameds>()...,
                };
            }

            // Retrieves the bound value associated with the Name.
            template <typename Name>
            constexpr decltype(auto) get(tag_t<Name> id) &&
            {
                // Note: Clang rejects this without the macro, GCC is fine.
                // Unsure which compiler is wrong.
                return NICKEL_FWD(NICKEL_MOVE(*this).lookup_named(id).value);
            }

#ifdef __cpp_if_constexpr
            // Retrieves the bound value associated with the Name, else falls back on retrieving
            // from `defaults`.
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

            // Retrieves the bound value associated with the Name, else falls back on retrieving
            // from `defaults`.
            template <typename Name, typename Defaults>
            constexpr decltype(auto) get_or_default(tag_t<Name> id, Defaults&& defaults) &&
            {
                return NICKEL_MOVE(*this).get_or_default_(
                    std::integral_constant<bool, is_set<Name>> {}, id, NICKEL_FWD(defaults));
            }
#endif

            // NOT PUBLIC API
            // TODO: figure out how to enforce non-public API.
            // TODO: figure out precisely what this does.
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

        // Unwraps the names_t<...> Kwargs and Names parameters of the wrapped_fn.
        template <typename Derived, typename Storage, typename Kwargs, typename Names>
        struct wrapped_fn_base;

        template <typename Derived, typename Storage, typename... Kwargs, typename... Names>
        struct wrapped_fn_base<Derived, Storage, names_t<Kwargs...>, names_t<Names...>>
            : // Provide .<name>() members for kwargs.
              public allow_set_only_if_unset<Storage, Kwargs, Derived>...,
              // Provide .<name>() members for named parameters.
              public allow_set_only_if_unset<Storage, Names, Derived>...
        { };

        // wrapped_fn is the main workhorse of Nickel.

        // The in-progress function call sequence.
        template <typename Defaults, // The default arguments
            typename Storage, // Any currently bound arguments
            typename Fn, // The actual function we are wrapping (which we will call)
            typename Kwargs, // Any Kwargs
            typename Names, // The explicit named parameters
            typename CallEvalPolicy> // How to implement calling `Fn`.
        class NICKEL_NODISCARD("Should be invoked with a bare () at the end") wrapped_fn
            : public wrapped_fn_base<
                  wrapped_fn<Defaults, Storage, Fn, Kwargs, Names, CallEvalPolicy>, Storage, Kwargs,
                  Names>
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
            // TODO: figure out how to enforce the NOT PUBLIC API
            template <typename Name, int N, typename... Ts>
            constexpr auto operator()(set_tag, Name, int_t<N>, Ts&&... values) &&
            {
                using NewStorage = decltype(NICKEL_MOVE(storage_).template _set_value<Name>(
                    set_tag {}, std::tuple<Ts&&...>(NICKEL_FWD(values)...)));
                return wrapped_fn<Defaults, NewStorage, remove_cvref_t<Fn>, Kwargs, Names,
                    CallEvalPolicy> {
                    NICKEL_MOVE(defaults_),
                    NICKEL_MOVE(storage_).template _set_value<Name>(
                        set_tag {}, std::tuple<Ts&&...>(NICKEL_FWD(values)...)),
                    NICKEL_MOVE(fn_),
                };
            }

            // Bind the name to the single argument.
            // NOT PUBLIC API
            // TODO: figure out how to enforce the NOT PUBLIC API
            template <typename Name, typename T>
            constexpr auto operator()(set_tag, Name, int_t<-1>, T&& value) &&
            {
                using NewStorage
                    = decltype(NICKEL_MOVE(storage_).template set<Name>(NICKEL_FWD(value)));
                return wrapped_fn<Defaults, NewStorage, remove_cvref_t<Fn>, Kwargs, Names,
                    CallEvalPolicy> {
                    NICKEL_MOVE(defaults_),
                    NICKEL_MOVE(storage_).template set<Name>(NICKEL_FWD(value)),
                    NICKEL_MOVE(fn_),
                };
            }

            // Bind the kwargs
            // NOT PUBLIC API
            // TODO: figure out how to enforce the NOT PUBLIC API
            template <typename OtherStorage>
            constexpr auto operator()(kwargs<OtherStorage>&& kwargs) &&
            {
                using NewStorage = decltype(NICKEL_MOVE(kwargs).combine(NICKEL_MOVE(storage_)));
                return wrapped_fn<Defaults, NewStorage, remove_cvref_t<Fn>, Kwargs, Names,
                    CallEvalPolicy> {
                    NICKEL_MOVE(defaults_),
                    NICKEL_MOVE(kwargs).combine(NICKEL_MOVE(storage_)),
                    NICKEL_MOVE(fn_),
                };
            }

            // Call the function with bound arguments
            constexpr decltype(auto) operator()() &&
            {
                return CallEvalPolicy::eval(NICKEL_MOVE(defaults_), NICKEL_MOVE(storage_),
                    Kwargs {}, Names {}, NICKEL_MOVE(fn_));
            }
        };

        // The default policy: calls the function with the named arguments
        struct named_eval_policy
        {
            // Calls the function, including kwargs parameters.
            template <typename Defaults, typename Storage, typename Kwargs, typename Names,
                typename Fn>
            static constexpr decltype(auto) eval_impl(
                std::true_type, Defaults&& defaults, Storage&& storage, Kwargs, Names, Fn&& fn)
            {
                return Names::map_reduce(NICKEL_FWD(fn), NICKEL_FWD(storage), NICKEL_FWD(defaults),
                    NICKEL_FWD(storage).get(detail::tag_t<Kwargs> {}, NICKEL_FWD(defaults)));
            }

            // Calls the function, in the absence of kwargs.
            template <typename Defaults, typename Storage, typename Kwargs, typename Names,
                typename Fn>
            static constexpr decltype(auto) eval_impl(
                std::false_type, Defaults&& defaults, Storage&& storage, Kwargs, Names, Fn&& fn)
            {
                return Names::map_reduce(
                    NICKEL_MOVE(fn), NICKEL_MOVE(storage), NICKEL_MOVE(defaults));
            }

            // Evaluate the function call
            template <typename Defaults, typename Storage, typename Kwargs, typename Names,
                typename Fn>
            static constexpr decltype(auto) eval(
                Defaults&& defaults, Storage&& storage, Kwargs, Names, Fn&& fn)
            {
                return eval_impl(std::integral_constant<bool, Kwargs::count != 0> {},
                    NICKEL_FWD(defaults), NICKEL_FWD(storage), Kwargs {}, Names {}, NICKEL_FWD(fn));
            }
        };

        // Re-uses the wrapped_fn workhorse to implement member stealing.
        struct steal_eval_policy
        {
            // Defaults -> the members of the class we are stealing from
            // Storage -> 0-arg values, storing the members requested and their order
            // Fn -> The object which we are stealing from

            // Normally: Defaults, Storage, Kwargs, Names, Fn.
            // We're re-using those for something else
            template <typename Members, typename Names_, typename Class, typename... Names>
            static constexpr auto eval(Members&& members,
                storage<named<Names, std::tuple<>>...> /* storage */, names_t<> /* kwargs */,
                Names_ /* names */, Class* object)
            {
                return std::make_tuple(
                    NICKEL_MOVE(object->*(NICKEL_MOVE(members).get(tag_t<Names> {})))...);
            }

            // Special case the single-name version to remove the tuple
            template <typename Members, typename Names_, typename Class, typename Name>
            static constexpr decltype(auto) eval(Members&& members,
                storage<named<Name, std::tuple<>>> /* storage */, names_t<> /* kwargs */,
                Names_ /* names */, Class* object)
            {
                return NICKEL_MOVE(object->*(NICKEL_MOVE(members).get(tag_t<Name> {})));
            }
        };

        // A partial function definition where the names have been specified, but not the function.
        // i.e. nickel::wrap(name1, name2), but without the second parentheses.
        template <typename Defaults, typename Kwargs, typename Names>
        class partial_wrap : private Defaults
        {
        public:
            template <typename FDefaults>
            explicit constexpr partial_wrap(construct_tag, FDefaults&& defaults)
                : Defaults {NICKEL_FWD(defaults)}
            { }

            // Add the actual function that we will call
            template <typename Fn>
            constexpr auto operator()(Fn&& fn) &&
            {
                using DFn = remove_cvref_t<Fn>;

                return wrapped_fn<Defaults, storage<>, DFn, Kwargs, Names, named_eval_policy> {
                    Defaults {static_cast<Defaults&&>(*this)},
                    storage<> {construct_tag {}},
                    NICKEL_FWD(fn),
                };
            }
        };

        // A defaulted argument
        template <typename Name, typename Value>
        struct defaulted
        {
            using name_type = Name;

            Value value;
        };

        // TODO: convert this to some more uniform way of having private methods.
        struct name_group_to_partial_fn_tag
        { };

        // TODO: convert this to some more uniform way of having private methods
        struct mark_kwargs_tag
        { };

        // Groups several names into one piece of functionality that can be passed like one name.
        // Every path through `nickel::wrap(...)` or similar gets wrapped in a name_group to enable
        // uniformity in name_group's abilities.
        template <typename Defaults, typename Kwargs, typename Names>
        class name_group : private Defaults
        {
        public:
            template <typename FDefaults>
            explicit constexpr name_group(construct_tag, FDefaults&& defaults)
                : Defaults {NICKEL_FWD(defaults)}
            { }

            // Produces a single name_group which has all of _our_ names and all of _group_'s names.
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

            // Initiates the partial_wrap sequence.
            constexpr auto operator()(name_group_to_partial_fn_tag) &&
            {
                return detail::partial_wrap<Defaults, Kwargs, Names> {
                    construct_tag {},
                    static_cast<Defaults&&>(*this),
                };
            }

            // Initiates the member stealing sequence.
            template <typename Class>
            constexpr auto _make_steal_wrapped_fn(priv_tag, Class* obj) &&
            {
                return detail::wrapped_fn<Defaults, storage<>, Class*, names_t<>, Names,
                    steal_eval_policy> {
                    static_cast<Defaults&&>(*this),
                    storage<> {construct_tag {}},
                    obj,
                };
            }

            // Marks all of the names inside this name_group as kwargs instead of regular names.
            constexpr auto _mark_all_kwargs(mark_kwargs_tag) &&
            {
                return name_group<Defaults, typename Kwargs::template append_names<Names>,
                    names_t<>> {
                    construct_tag {},
                    static_cast<Defaults&&>(*this),
                };
            }

            // Detects if this name_group has any kwargs.
            static constexpr bool _has_kwargs(priv_tag)
            {
                return !NICKEL_IS_SAME(Kwargs, names_t<>);
            }
        };

        // Wraps a single argument into a name_group.
        template <typename Name>
        constexpr auto name_group_single(Name&&)
        {
            return detail::name_group<detail::storage<>, detail::names_t<>,
                detail::names_t<typename detail::remove_cvref_t<Name>::name_type>> {
                detail::construct_tag {},
                detail::storage<> {detail::construct_tag {}},
            };
        }

        // Wraps a single defaulted argument into a name_group.
        template <typename Name, typename Value>
        constexpr auto name_group_single(defaulted<Name, Value> defaulted_arg)
        {
            return name_group<storage<named<Name, Value>>, names_t<>,
                names_t<typename remove_cvref_t<Name>::name_type>> {
                construct_tag {},
                storage<named<Name, Value>> {
                    construct_tag {},
                    named<Name, Value> {NICKEL_MOVE(defaulted_arg).value},
                },
            };
        }

        // "Wraps" a single name_group argument into a name_group
        template <typename Defaults, typename Kwargs, typename Names>
        constexpr auto name_group_single(name_group<Defaults, Kwargs, Names> group)
        {
            return NICKEL_MOVE(group);
        }

        // Combines a sequence of name_groups into one.
        constexpr auto name_group_impl()
        {
            return name_group<storage<>, names_t<>, names_t<>> {
                construct_tag {},
                storage<> {construct_tag {}},
            };
        }

        template <typename NameGroup>
        constexpr auto name_group_impl(NameGroup&& group)
        {
            return NICKEL_FWD(group);
        }

        template <typename First, typename Second, typename... Rest>
        constexpr auto name_group_impl(First&& first, Second&& second, Rest&&... rest)
        {
            return detail::name_group_impl(
                NICKEL_FWD(first).combine(NICKEL_FWD(second)), NICKEL_FWD(rest)...);
        }
    }

    // Groups several names, allowing them in most places where a single name can be passed.
    // In such a case, a name_group acts as if the names were passed inline.
    template <typename... Names>
    constexpr auto name_group(Names&&... names)
    {
        return detail::name_group_impl(detail::name_group_single(NICKEL_FWD(names))...);
    }

    // Marks names as kwargs.
    // Kwargs can be set using the function call syntax, but will be grouped up and passed as the
    // first argument to the lambda. This first argument can then be passed on to other named
    // parameter functions.
    template <typename... Names>
    constexpr auto kwargs_group(Names&&... names)
    {
        auto ng = nickel::name_group(NICKEL_FWD(names)...);
        return NICKEL_MOVE(ng)._mark_all_kwargs(detail::mark_kwargs_tag {});
    }

    // Creates a wrapped function definition with the specified named parameters.
    // nickel::wrap(clustered=false, count)([](bool clustered, int num_stars) { /* spawn stars */ })
    template <typename... Names>
    constexpr auto wrap(Names&&... names)
    {
        return nickel::name_group(NICKEL_FWD(names)...)(detail::name_group_to_partial_fn_tag {});
    }

    // Marks a default argument value as unevaluated unless needed.
    template <typename Lambda>
    constexpr auto deferred(Lambda&& fn)
    {
        return detail::deferred<detail::remove_cvref_t<Lambda>>(
            detail::construct_tag {}, NICKEL_FWD(fn));
    }

    // EXPERIMENTAL
    // Enables stealing members from `object` in the order specified by the caller.
    template <typename Class, typename... Names, typename... PtrToMemData>
    constexpr auto steal(Class&& object, detail::defaulted<Names, PtrToMemData>... names)
    {
        static_assert(NICKEL_IS_RVALUE_REFERENCE(Class &&),
            "Object must be an rvalue. Pass std::move(*this) to nickel::steal(...)");

        using check_group_t = decltype(nickel::name_group(NICKEL_MOVE(names)...));

        static_assert(!check_group_t::_has_kwargs(detail::priv_tag {}),
            "nickel::steal(...) cannot accept kwargs");

        return nickel::name_group((Names {}.template multivalued<0>() = names.value)...)
            ._make_steal_wrapped_fn(detail::priv_tag {}, std::addressof(object));
    }

// Creates a name. This is the 2-arg overload.
#define NICKEL_DETAIL_NAME2(variable, name)                                                        \
    template <int N = -1>                                                                          \
    struct variable##_nickel_name_type                                                             \
    {                                                                                              \
        using name_type = variable##_nickel_name_type;                                             \
        template <typename Derived>                                                                \
        struct set_type                                                                            \
        {                                                                                          \
            template <typename... Ts>                                                              \
            constexpr auto name(Ts&&... values) &&                                                 \
            {                                                                                      \
                static_assert(sizeof...(Ts) == N || (sizeof...(Ts) == 1 && N == -1),               \
                    "Must call the function with the specified arguments: " #name);                \
                return static_cast<Derived&&>(*this)(::nickel::detail::set_tag {},                 \
                    variable##_nickel_name_type {}, ::nickel::detail::int_t<N> {},                 \
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
                    ::nickel::detail::get_tag {}, variable##_nickel_name_type {});                 \
            }                                                                                      \
            constexpr decltype(auto) name() const&                                                 \
            {                                                                                      \
                return static_cast<Derived const&>(*this)(                                         \
                    ::nickel::detail::get_tag {}, variable##_nickel_name_type {});                 \
            }                                                                                      \
            constexpr decltype(auto) name() &&                                                     \
            {                                                                                      \
                return static_cast<Derived&&>(*this)(                                              \
                    ::nickel::detail::get_tag {}, variable##_nickel_name_type {});                 \
            }                                                                                      \
            constexpr decltype(auto) name() const&&                                                \
            {                                                                                      \
                return static_cast<Derived const&&>(*this)(                                        \
                    ::nickel::detail::get_tag {}, variable##_nickel_name_type {});                 \
            }                                                                                      \
        };                                                                                         \
                                                                                                   \
        template <typename T>                                                                      \
        constexpr auto operator=(T&& value) const                                                  \
        {                                                                                          \
            return ::nickel::detail::defaulted<variable##_nickel_name_type,                        \
                ::nickel::detail::remove_cvref_t<T>> {                                             \
                NICKEL_DETAIL_FWD(value),                                                          \
            };                                                                                     \
        }                                                                                          \
                                                                                                   \
        template <int NArgs>                                                                       \
        constexpr auto multivalued() const -> variable##_nickel_name_type<NArgs>                   \
        {                                                                                          \
            static_assert(NArgs >= 0, "Cannot ask for a negative number of args");                 \
            return {};                                                                             \
        }                                                                                          \
    };                                                                                             \
                                                                                                   \
    constexpr auto variable = variable##_nickel_name_type<>                                        \
    { }

// Creates a name. This is the 1-arg overload.
#define NICKEL_DETAIL_NAME1(name) NICKEL_DETAIL_NAME2(name, name)

// Force evaluation of the preprocessor, even previous tokens of `MACRO ( macro args )`
#define NICKEL_DETAIL_EXPAND(...) __VA_ARGS__

#define NICKEL_DETAIL_NAME_OVERLOAD(_1, _2, Overload, ...) Overload

// Creates a name.
// There are two overloads, one for 2 arguments, and one for 1 argument.
// 2 args: NICKEL_NAME(var, name): creates a variable `var` with name `.<name>()`.
// 1 arg: NICKEL_NAME(name): Equivalent to NICKEL_NAME(name, name).
#define NICKEL_NAME(...)                                                                           \
    NICKEL_DETAIL_EXPAND(NICKEL_DETAIL_NAME_OVERLOAD(                                              \
        __VA_ARGS__, NICKEL_DETAIL_NAME2, NICKEL_DETAIL_NAME1, )(__VA_ARGS__))
}

#undef NICKEL_FWD
#undef NICKEL_MOVE
#undef NICKEL_IS_VOID
#undef NICKEL_IS_SAME
#undef NICKEL_IS_RVALUE_REFERENCE
#undef NICKEL_NODISCARD
#undef NICKEL_MSVC_PRAGMA
#undef NICKEL_GCC_WIGNORE
#undef NICKEL_GCC_PRAGMA
#undef NICKEL_CLANG_WIGNORE
#undef NICKEL_CLANG_PRAGMA

#endif
