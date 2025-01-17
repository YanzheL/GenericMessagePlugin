//  Copyright GenericMessagePlugin, Inc. All Rights Reserved.

#pragma once
#include "Runtime/Launch/Resources/Version.h"
#include "UObject/UnrealType.h"
#include "UnrealCompatibility.h"

#include <tuple>
#include <type_traits>

#if UE_5_01_OR_LATER
#define GMP_IF_CONSTEXPR if constexpr
#elif PLATFORM_COMPILER_HAS_IF_CONSTEXPR
#define GMP_IF_CONSTEXPR if constexpr
#else
#define GMP_IF_CONSTEXPR if
#endif
#if (__cplusplus >= 201703L) || (defined(_HAS_CXX17) && _HAS_CXX17)
#define GMP_INLINE inline
#else
#define GMP_INLINE
#endif

GMP_API DECLARE_LOG_CATEGORY_EXTERN(LogGMP, Log, All);

static const FName NAME_GMPSkipValidate{TEXT("SkipValidate")};

#define GMP_WARNING(FMT, ...) UE_LOG(LogGMP, Warning, FMT, ##__VA_ARGS__)
#define GMP_ERROR(FMT, ...) UE_LOG(LogGMP, Error, FMT, ##__VA_ARGS__)
#define GMP_LOG(FMT, ...) UE_LOG(LogGMP, Log, FMT, ##__VA_ARGS__)
#define GMP_TRACE(FMT, ...) UE_LOG(LogGMP, Verbose, FMT, ##__VA_ARGS__)

#ifndef GMP_DEBUGGAME
#if WITH_EDITOR && defined(UE_BUILD_DEBUGGAME) && UE_BUILD_DEBUGGAME
#define GMP_DEBUGGAME 1
#else
#define GMP_DEBUGGAME 0
#endif
#endif

#if GMP_DEBUGGAME
#define GMP_DEBUG_LOG(FMT, ...) GMP_LOG(FMT, ##__VA_ARGS__)
#define GMP_TRACE_FMT(FMT, ...) GMP_TRACE(TEXT("GMP-TRACE:[%s] ") FMT, ITS::TypeWStr<decltype(this)>(), ##__VA_ARGS__);
#define GMP_TRACE_THIS() GMP_TRACE(TEXT("GMP-TRACE:[%s]"), ITS::TypeStr<decltype(this)>());

#else
#define GMP_DEBUG_LOG(FMT, ...) void(0)
#define GMP_TRACE_FMT(FMT, ...) void(0)
#define GMP_TRACE_THIS() void(0)
#endif
#define GMP_TO_STR_(STR) #STR
#define GMP_TO_STR(STR) GMP_TO_STR_(STR)

#define GMP_ENABLE_DEBUGVIEW GMP_DEBUGGAME_EDITOR
#if GMP_ENABLE_DEBUGVIEW
#define GMP_DEBUGVIEW_LOG(FMT, ...) GMP_TRACE(FMT, ##__VA_ARGS__)
#define GMP_DEBUGVIEW_FMT(FMT, ...) GMP_TRACE(TEXT("GMP-DEBUG:[%s] ") FMT, ITS::TypeWStr<decltype(this)>(), ##__VA_ARGS__);
#define GMP_DEBUGVIEW_THIS() GMP_TRACE(TEXT("GMP-DEBUG:[%s]"), ITS::TypeWStr<decltype(this)>());
#else
#define GMP_DEBUGVIEW_LOG(FMT, ...) void(0)
#define GMP_DEBUGVIEW_FMT(FMT, ...) void(0)
#define GMP_DEBUGVIEW_THIS() void(0)
#endif

#if UE_BUILD_SHIPPING
#define GMP_NOTE(T, Fmt, ...) ensureAlwaysMsgf(false, Fmt, ##__VA_ARGS__)
#define GMP_CNOTE(C, T, Fmt, ...) ensureAlwaysMsgf(C, Fmt, ##__VA_ARGS__)
#define GMP_CNOTE_ONCE(C, Fmt, ...) ensureMsgf(C, Fmt, ##__VA_ARGS__)
#else
#define GMP_NOTE(T, Fmt, ...)            \
	[&] {                                \
		GMP_WARNING(Fmt, ##__VA_ARGS__); \
		return ensureAlways(!(T));       \
	}()

#define GMP_CNOTE(C, T, Fmt, ...)        \
	(!!(C) || [&] {                      \
		GMP_WARNING(Fmt, ##__VA_ARGS__); \
		return ensureAlways(!(T));       \
	}())
#define GMP_CNOTE_ONCE(C, Fmt, ...)      \
	(!!(C) || [&] {                      \
		GMP_WARNING(Fmt, ##__VA_ARGS__); \
		return ensure(false);            \
	}())
#endif

namespace GMP
{
namespace TypeTraits
{
#ifdef __clang__
	template<class _Ty1, class _Ty2>
	GMP_INLINE constexpr bool IsSameV = __is_same(_Ty1, _Ty2);
#else
	template<class, class>
	GMP_INLINE constexpr bool IsSameV = false;
	template<class _Ty>
	GMP_INLINE constexpr bool IsSameV<_Ty, _Ty> = true;
#endif

	template<class T, std::size_t = sizeof(T)>
	std::true_type IsCompleteImpl(T*);
	std::false_type IsCompleteImpl(...);

	template<class T>
	using IsComplete = decltype(IsCompleteImpl(std::declval<T*>()));

	// is_detected
	template<typename, template<typename...> class Op, typename... T>
	struct IsDetectedImpl : std::false_type
	{
	};
	template<template<typename...> class Op, typename... T>
	struct IsDetectedImpl<VoidType<Op<T...>>, Op, T...> : std::true_type
	{
	};
	template<template<typename...> class Op, typename... T>
	using IsDetected = IsDetectedImpl<void, Op, T...>;

	template<typename... Ts>
	struct TDisjunction;
	template<typename V, typename... Is>
	struct TDisjunction<V, Is...>
	{
		enum
		{
			value = V::value ? V::value : TDisjunction<Is...>::value
		};
	};
	template<>
	struct TDisjunction<>
	{
		enum
		{
			value = 0
		};
	};

	struct UnUseType
	{
	};

	template<template<typename...> class Base, typename Derived>
	struct IsDerivedFromTemplate
	{
		using U = typename std::remove_cv_t<typename std::remove_reference_t<Derived>>;

		template<typename... TArgs>
		static auto test(Base<TArgs...>*) -> typename std::integral_constant<bool, !TypeTraits::IsSameV<U, Base<TArgs...>>>;

		static std::false_type test(void*);

		using type = decltype(test(std::declval<U*>()));
		enum
		{
			value = type::value,
		};

		template<typename... TArgs>
		static auto Types(Base<TArgs...>*) -> Base<TArgs...>;
	};

	template<typename T>
	struct TIsCallable
	{
	private:
		template<typename V>
		using IsCallableType = decltype(&V::operator());
		template<typename V>
		using IsCallable = IsDetected<IsCallableType, V>;

	public:
		static const bool value = IsCallable<std::decay_t<T>>::value;
	};

	template<typename T>
	struct TIsBaseDelegate
	{
	private:
		template<typename V>
		using IsBaseDelegateType = decltype(&V::ExecuteIfBound);
		template<typename V>
		using IsBaseDelegate = IsDetected<IsBaseDelegateType, V>;

	public:
		static const bool value = IsBaseDelegate<std::decay_t<T>>::value;
	};

	template<typename... TArgs>
	struct TGetFirst;
	template<>
	struct TGetFirst<>
	{
		using type = UnUseType;
	};
	template<typename T, typename... TArgs>
	struct TGetFirst<T, TArgs...>
	{
		using type = T;
	};

	template<typename... TArgs>
	using TGetFirstType = typename TGetFirst<TArgs...>::type;

	template<typename T, typename... TArgs>
	struct TIsFirstSame
	{
		using FirstType = TGetFirstType<TArgs...>;
		enum
		{
			value = TIsSame<std::decay_t<FirstType>, T>::Value
		};
	};
	template<typename T>
	struct TIsFirstSame<T>
	{
		enum
		{
			value = 0
		};
	};

	template<typename... TArgs>
	struct TIsFirstTypeCallable;
	template<>
	struct TIsFirstTypeCallable<>
	{
		using type = std::false_type;
		enum
		{
			value = type::value
		};
	};
	template<typename T, typename... TArgs>
	struct TIsFirstTypeCallable<T, TArgs...>
	{
		using type = std::conditional_t<TIsCallable<T>::value, std::true_type, std::false_type>;
		enum
		{
			value = type::value
		};
	};

	template<typename T>
	struct TIsEnumByte
	{
		enum
		{
			value = std::is_enum<T>::value
		};
		using type = T;
	};

	template<typename T>
	struct TIsEnumByte<TEnumAsByte<T>>
	{
		enum
		{
			value = true
		};
		using type = T;
	};

	template<class Tuple, uint32 N>
	struct TRemoveTupleLast;

	template<typename... TArgs, uint32 N>
	struct TRemoveTupleLast<std::tuple<TArgs...>, N>
	{
	private:
		using Tuple = std::tuple<TArgs...>;
		template<std::size_t... n>
		static std::tuple<std::tuple_element_t<n, Tuple>...> extract(std::index_sequence<n...>);
		static_assert(sizeof...(TArgs) >= N, "err");

	public:
		using type = decltype(extract(std::make_index_sequence<sizeof...(TArgs) - N>()));
	};

	template<class Tuple, uint32 N>
	using TRemoveTupleLastType = typename TRemoveTupleLast<Tuple, N>::type;

	template<typename R, typename Tup>
	struct TSigTraitsImpl;
	template<typename R, typename... TArgs>
	struct TSigTraitsImpl<R, std::tuple<TArgs...>>
	{
		using TFuncType = R(TArgs...);
		using ReturnType = R;
		using Tuple = std::tuple<TArgs...>;
		using DecayTuple = std::tuple<std::decay_t<TArgs>...>;

		template<typename T, typename... Args>
		static auto GetLastType(std::tuple<T, Args...>) -> std::tuple_element_t<sizeof...(Args), std::tuple<T, Args...>>;
		static auto GetLastType(std::tuple<>) -> void;
		using LastType = decltype(GetLastType(std::declval<Tuple>()));

		enum
		{
			TupleSize = std::tuple_size<Tuple>::value
		};
	};

	template<typename TFunc, uint32 N = 0>
	struct TFunctionTraitsImpl
	{
		template<typename R, typename... TArgs>
		static auto GetSigType(R (*)(TArgs...)) -> TSigTraitsImpl<R, std::tuple<TArgs...>>;
		template<typename R, typename... TArgs>
		static auto GetSigType(R (&)(TArgs...)) -> TSigTraitsImpl<R, std::tuple<TArgs...>>;
		template<typename R, typename FF, typename... TArgs>
		static auto GetSigType(R (FF::*)(TArgs...)) -> TSigTraitsImpl<R, std::tuple<TArgs...>>;
		template<typename R, typename FF, typename... TArgs>
		static auto GetSigType(R (FF::*)(TArgs...) const) -> TSigTraitsImpl<R, std::tuple<TArgs...>>;
		using TSig = decltype(GetSigType(std::declval<TFunc>()));
		using TFuncType = typename TSig::TFuncType;
	};

	template<typename T, typename = void>
	struct TFunctionTraits;
	template<typename T>
	struct TFunctionTraits<T, VoidType<decltype(&std::decay_t<T>::operator())>> : public TFunctionTraitsImpl<decltype(&std::decay_t<T>::operator())>
	{
		using typename TFunctionTraitsImpl<decltype(&std::decay_t<T>::operator())>::TSig;
	};
	template<typename R, typename... TArgs>
	struct TFunctionTraits<R(TArgs...), void> : public TFunctionTraitsImpl<R(TArgs...)>
	{
		using typename TFunctionTraitsImpl<R(TArgs...)>::TSig;
	};
	template<typename T>
	struct TFunctionTraits<T, std::enable_if_t<std::is_member_function_pointer<T>::value>> : public TFunctionTraitsImpl<T>
	{
		using typename TFunctionTraitsImpl<T>::TSig;
	};
	template<typename T>
	struct TFunctionTraits<T, std::enable_if_t<std::is_pointer<T>::value && std::is_function<std::remove_pointer_t<T>>::value>> : public TFunctionTraitsImpl<T>
	{
		using typename TFunctionTraitsImpl<T>::TSig;
	};

	template<typename T>
	using TSigTraits = typename TFunctionTraits<std::decay_t<T>>::TSig;

	template<typename T>
	using TSigFuncType = typename TSigTraits<T>::TFuncType;

	template<typename... TArgs>
	struct TGetLast;
	template<>
	struct TGetLast<>
	{
		using type = UnUseType;
	};
	template<typename T, typename... TArgs>
	struct TGetLast<T, TArgs...>
	{
		using type = std::tuple_element_t<sizeof...(TArgs), std::tuple<T, TArgs...>>;
	};

	template<typename... TArgs>
	using TGetLastType = typename TGetLast<TArgs...>::type;

	template<typename T, typename... TArgs>
	struct TIsLastSame
	{
		using LastType = TGetLastType<TArgs...>;
		enum
		{
			value = TIsSame<typename TRemoveCV<LastType>::Type, T>::Value
		};
	};
	template<typename T>
	struct TIsLastSame<T>
	{
		enum
		{
			value = 0
		};
	};

	template<typename... TArgs>
	struct TIsLastTypeCallable;
	template<>
	struct TIsLastTypeCallable<>
	{
		enum
		{
			callable_type = 0,
			delegate_type = 0,
			value = 0
		};
		using type = std::false_type;
	};
	template<typename C, typename... TArgs>
	struct TIsLastTypeCallable<C, TArgs...>
	{
		using LastType = std::tuple_element_t<sizeof...(TArgs), std::tuple<C, TArgs...>>;
		enum
		{
			callable_type = TIsCallable<LastType>::value,
			delegate_type = TIsBaseDelegate<LastType>::value,
			value = callable_type || delegate_type
		};
		using type = std::conditional_t<value, std::true_type, std::false_type>;
	};

	template<typename Tuple>
	struct TTupleRemoveLast;

	template<>
	struct TTupleRemoveLast<std::tuple<>>
	{
		using type = std::tuple<>;
	};

	template<typename T, typename... TArgs>
	struct TTupleRemoveLast<std::tuple<T, TArgs...>>
	{
		using Tuple = std::tuple<T, TArgs...>;
		template<std::size_t... Is>
		static auto extract(std::index_sequence<Is...>) -> std::tuple<std::tuple_element_t<Is, Tuple>...>;
		using type = decltype(extract(std::make_index_sequence<std::tuple_size<Tuple>::value - 1>()));
	};
	template<typename Tuple>
	using TTupleRemoveLastType = typename TTupleRemoveLast<std::remove_cv_t<std::remove_reference_t<Tuple>>>::type;

	template<typename... TArgs>
	struct TupleTypeRemoveLast;

	template<>
	struct TupleTypeRemoveLast<>
	{
		using Tuple = std::tuple<>;
		using type = void;
	};

	template<typename T, typename... TArgs>
	struct TupleTypeRemoveLast<T, TArgs...>
	{
		using Tuple = std::tuple<T, TArgs...>;
		using type = TTupleRemoveLastType<Tuple>;
	};

	template<typename... TArgs>
	using TupleTypeRemoveLastType = typename TupleTypeRemoveLast<TArgs...>::type;

	template<typename E>
	constexpr std::underlying_type_t<E> ToUnderlying(E e)
	{
		return static_cast<std::underlying_type_t<E>>(e);
	}

	template<typename TValue, typename TAddr>
	union HorribleUnion
	{
		TValue Value;
		TAddr Addr;
	};
	template<typename TValue, typename TAddr>
	FORCEINLINE TValue HorribleFromAddr(TAddr Addr)
	{
		HorribleUnion<TValue, TAddr> u;
		static_assert(sizeof(TValue) >= sizeof(TAddr), "Cannot use horrible_cast<>");
		u.Value = 0;
		u.Addr = Addr;
		return u.Value;
	}
	template<typename TAddr, typename TValue>
	FORCEINLINE TAddr HorribleToAddr(TValue Value)
	{
		HorribleUnion<TValue, TAddr> u;
		static_assert(sizeof(TValue) >= sizeof(TAddr), "Cannot use horrible_cast<>");
		u.Value = Value;
		return u.Addr;
	}
	template<uint32_t N>
	struct const32
	{
		enum : uint32_t
		{
			Value = N
		};
	};

	template<uint64_t UUID>
	struct const64
	{
		enum : uint64_t
		{
			Value = UUID
		};

		template<std::size_t... First, std::size_t... Second>
		static auto Combine(const char* str, std::index_sequence<First...>, std::index_sequence<Second...>)
		{
			// init only once by UUID
			static auto Result = [](const char* p) {
				constexpr auto FirstSize = (sizeof...(First)) + 1;
				const char s[(sizeof...(Second)) + FirstSize] = {p[1 + First]..., '.', p[FirstSize + 2 + Second]...};
				return FName(s);
			}(str);
			return Result;
		}
		template<std::size_t... First, std::size_t... Second>
		static auto CombineRPC(const char* str, std::index_sequence<First...>, std::index_sequence<Second...>)
		{
			// init only once by UUID
			static auto Result = [](const char* p) {
				constexpr auto FirstSize = (sizeof...(First)) + 1;
				const char s[4 + (sizeof...(Second)) + FirstSize] = {'R', 'P', 'C', '.', p[1 + First]..., '.', p[FirstSize + 2 + Second]...};
				return FName(s);
			}(str);
			return Result;
		}
	};

	constexpr std::size_t GetSplitIndex(const char* str, const std::size_t value = 0) { return (str[0] == ':') ? value : GetSplitIndex(&str[1], value + 1); }

	template<uint64_t UUID, std::size_t N, std::size_t Index>
	FORCEINLINE constexpr auto ToMessageRPCId(const char* str)
	{
		return const64<UUID>::CombineRPC(str, std::make_index_sequence<Index - 1>{}, std::make_index_sequence<N - Index - 2>{});
	}
}  // namespace TypeTraits
}  // namespace GMP

#define Z_GMP_OBJECT_NAME TObjectPtr
#define NAME_GMP_TObjectPtr TEXT(GMP_TO_STR(Z_GMP_OBJECT_NAME))
#if !UE_5_00_OR_LATER
struct FObjectPtr
{
	UObject* Ptr;
};
template<typename T>
struct Z_GMP_OBJECT_NAME : private FObjectPtr
{
	T* Get() const { return CastChecked<T>(Ptr); }
};
static_assert(std::is_base_of<FObjectPtr, Z_GMP_OBJECT_NAME<UObject>>::value, "err");
#else
static_assert(sizeof(FObjectPtr) == sizeof(Z_GMP_OBJECT_NAME<UObject>), "err");
#endif

#if 1
#define GMP_RPC_FUNC_NAME(t) GMP::TypeTraits::ToMessageRPCId<ITS::hash_64_fnv1a_const(t), UE_ARRAY_COUNT(t), GMP::TypeTraits::GetSplitIndex(t)>(t)
#else
#define GMP_RPC_FUNC_NAME(t) GMP::TypeTraits::const32<ITS::hash_32_fnv1a_const(t)>::Value
#endif
