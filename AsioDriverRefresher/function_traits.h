#pragma once

template <typename... Args>
struct sumSizeOfArgs
{
	static constexpr size_t totalSize = 0;
};

template <typename T, typename... Args>
struct sumSizeOfArgs<T, Args...>
{
	static constexpr size_t totalSize = sizeof(typename std::decay<T>::type) + sumSizeOfArgs<Args...>::totalSize;
};

template <typename T>
struct sumSizeOfArgs<T>
{
	static constexpr size_t totalSize = sizeof(typename std::decay<T>::type);
};

template <typename T>
struct function_traits_impl;

template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits_impl<ReturnType(ClassType::*)(Args...)>
{
	static constexpr size_t arity = sizeof...(Args);

	using result_type = ReturnType;

	static constexpr size_t totalSize = sumSizeOfArgs<Args...>::totalSize;

	using type_list = std::tuple<Args...>;

	template <size_t i>
	struct arg
	{
		using type = typename std::tuple_element<i, std::tuple<Args...>>::type;
	};
};

template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits_impl<ReturnType(ClassType::*)(Args...) const>
	: function_traits_impl<ReturnType(ClassType::*)(Args...)> {};

template <typename ReturnType, typename... Args>
struct function_traits_impl<ReturnType(Args...)>
{
	static constexpr size_t arity = sizeof...(Args);

	using result_type = ReturnType;

	static constexpr size_t totalSize = sumSizeOfArgs<Args...>::totalSize;

	using type_list = std::tuple<Args...>;

	template <size_t i>
	struct arg
	{
		using type = typename std::tuple_element<i, std::tuple<Args...>>::type;
	};
};

template <typename ReturnType, typename... Args>
struct function_traits_impl<ReturnType(*)(Args...)> : function_traits_impl<ReturnType(Args...)> {};

template <typename T, typename V = void>
struct function_traits : function_traits_impl<T> {};

template <typename T>
struct function_traits<T, decltype((void)&T::operator())> : function_traits_impl<decltype(&T::operator())> {};