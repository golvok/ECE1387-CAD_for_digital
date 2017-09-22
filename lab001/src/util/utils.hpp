#ifndef UTIL_H
#define UTIL_H

#include <algorithm>
#include <cstdint>
#include <climits>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>

#include <iostream>

template <typename...> struct all_convertible;

template <> struct all_convertible<> : std::true_type { };

template <typename T> struct all_convertible<T> : std::true_type { };

template <typename T, typename V, typename... Rest> struct all_convertible<T, V, Rest...>
: std::integral_constant<
	bool,
	std::is_convertible<V, T>::value && all_convertible<T,Rest...>::value
> { };

namespace util {
	template<typename T>
	void reverse(T& t) {
		reverse(std::begin(t),std::end(t));
	}

	template<typename FUNC>
	void repeat(uint count, const FUNC& f) {
		for (uint i = 1; i <= count; ++i) {
			f();
		}
	}

	template<typename FUNC, typename VALUE>
	auto repeat_extra_times(size_t num_extra_times, VALUE&& v, const FUNC& f) {
		auto result = f(v);
		for (size_t i = 0; i < num_extra_times; ++i) {
			result = f(result);
		}
		return result;
	}

	template<typename COLLECTION, typename VALUE>
	auto skip_find(const COLLECTION& c, size_t num_to_skip, const VALUE& v) {
		using std::begin; using std::end; using std::next;
		auto curr = std::find(begin(c), end(c), v);
		for (size_t match_num = 2; (match_num-1) <= num_to_skip; ++match_num) {
			curr = std::find(next(curr), end(c), v);
			if (curr == end(c)) {
				break;
			}
		}
		return curr;
	}

	template<typename T>
	auto make_shared(T&& t) {
		return std::make_shared<
			typename std::remove_cv<
				typename std::remove_reference<T>::type
			>::type
		>(
			std::forward<T>(t)
		);
	}

	template<typename T>
	auto make_copy(const T& t) {
		return T(t);
	}

	template<typename CONTAINER, typename PRED>
	void remove_if_assoc(CONTAINER& c, PRED&& p) {
		for(auto it = begin(c); it != end(c); ) {
			if (p(*it)) {
				it = c.erase(it);
			} else {
				++it;
			}
		}
	}
}

/*******
 * Begin definition of ComparesWithTag
 *******/

template<typename T, typename U>
class ComparesWithTag {
	const T& thing;
	U identifier;

public:
	ComparesWithTag(const T& thing, U identifier)
		: thing(thing)
		, identifier(identifier)
	{ }
	ComparesWithTag(const ComparesWithTag&) = default;
	ComparesWithTag& operator=(const ComparesWithTag&) = delete;
	ComparesWithTag& operator=(ComparesWithTag&&) = default;

	operator std::tuple<T,U>() const {
		return {thing,identifier};
	}

	template<typename THEIR_T, typename THEIR_U>
	bool operator<(const ComparesWithTag<THEIR_T,THEIR_U> rhs) const {
		return thing < rhs.thing;
	}

	const T& value() const { return thing; }
	U id() const { return identifier; }
};


/**
 * Allows you to tag some data along when you call std::max or something.
 * Defines operator< using the first argumen's operator<
 *
 * Example:
 *    enum class MyEnum {
 *        A,B,
 *    };
 *
 *    auto result = std::max(
 *        compare_with_tag(getA(),MyEnum::A),
 *        compare_with_tag(getB(),MyEnum::B)
 *    );
 *
 *    if (result.id() == MyEnum::A) {
 *        std::cout << "A was smaller, A = " << result << '\n';
 *    } else if (result.id() == MyEnum::B) {
 *        std::cout << "B was smaller, B = " << result << '\n';
 *    }
 */
template<typename T, typename U>
ComparesWithTag<T,U> compare_with_tag(const T& thing, U id) {
	return ComparesWithTag<T,U>(thing,id);
}

template<typename T, typename V = void>
using EnableIfEnum = typename std::enable_if<std::is_enum<T>::value,V>;

template<typename T, typename V = void>
using EnableIfIntegral = typename std::enable_if<std::is_integral<T>::value,V>;

template<typename CAST_TO>
struct StaticCaster {
	template<typename SRC>
	auto operator()(SRC src) const {
		return static_cast<CAST_TO>(src);
	}
};

namespace util {

class IDBase {
public:
	int getValue() const;
	void print(std::ostream& os);
};

template<typename id_type, typename TAG>
class ID : public IDBase {
	id_type value;
protected:
	explicit ID(const id_type& value) : value(value) { }

	template<typename ID_TYPE, typename... ARGS>
	friend auto make_id(ARGS&&... args) -> std::enable_if_t<
		std::is_base_of<typename ID_TYPE::ThisIDType,ID_TYPE>::value,
		ID_TYPE
	>;
public:
	using IDType = id_type;
	using ThisIDType = ID<id_type,TAG>;
	const static id_type DEFAULT_VALUE = TAG::DEFAULT_VALUE;
	const static size_t bit_size = sizeof(IDType)*CHAR_BIT;

	ID() : value(TAG::DEFAULT_VALUE) { }
	ID(const ID&) = default;
	ID(ID&&) = default;

	ID& operator=(const ID&) = default;
	ID& operator=(ID&&) = default;

	explicit operator id_type() const { return value; }
	id_type getValue() const { return value; }
	void print(std::ostream& os) { os << value; }
};

template<typename ID_TYPE, typename... ARGS>
auto make_id(ARGS&&... args) -> std::enable_if_t<
	std::is_base_of<typename ID_TYPE::ThisIDType,ID_TYPE>::value,
	ID_TYPE
> {
	return ID_TYPE(std::forward<ARGS>(args)...);
}

template<typename ID>
class IDGenerator {
	using IDType = typename ID::IDType;

	IDType next_id_value;
public:
	IDGenerator(IDType first_value)
		: next_id_value(first_value)
	{ }

	ID gen_id() {
		ID id = make_id<ID>(next_id_value);
		++next_id_value;
		return id;
	}
};

template<typename... ARGS>
auto make_id_generator(ARGS&&... args) {
	return IDGenerator<ARGS...>(std::forward<ARGS>(args)...);
}

template<typename id_type, typename TAG>
bool operator==(const ID<id_type,TAG>& lhs, const ID<id_type,TAG>& rhs) {
	return (id_type)lhs == (id_type)rhs;
}

template<typename id_type, typename TAG>
bool operator!=(const ID<id_type,TAG>& lhs, const ID<id_type,TAG>& rhs) {
	return !(lhs == rhs);
}

template<typename T>
std::string stringify_through_stream(const T& t) {
	std::ostringstream stream;
	stream << t;
	return stream.str();
}

template<typename RETVAL_TYPE, typename ITER_TYPE>
class DerefAndIncrementer {
	ITER_TYPE iter;
	size_t i;
public:
	DerefAndIncrementer(ITER_TYPE beg) : iter(beg), i(0) { }
	template<typename ARG>
	RETVAL_TYPE operator()(const ARG&) {
		RETVAL_TYPE result = *iter;
		std::cout << result << " - " << i << '\n';
		++iter;
		++i;
		return result;
	}
};

/**
 * Retuns a lambda stlye object that will return the value of
 * *iter the first time it is called, then *std::next(iter), etc.
 */
template<typename RETVAL_TYPE, typename ITER_TYPE>
auto make_deref_and_incrementer(const ITER_TYPE& iter) {
	return DerefAndIncrementer<RETVAL_TYPE,ITER_TYPE>(iter);
}

template<class InputIt, class UnaryPredicate>
std::pair<InputIt,size_t> find_by_index(InputIt first, InputIt last, UnaryPredicate p) {
	size_t index = 0;
	for (; first != last; ++first, ++index) {
		if (p(index)) {
			return { first, index };
		}
	}
	return { last, index };
}

template<class InputIt, class BinaryPredicate>
std::pair<InputIt,size_t> find_with_index(InputIt first, InputIt last, BinaryPredicate p) {
	size_t index = 0;
	for (; first != last; ++first, ++index) {
		if (p(*first,index)) {
			return { first, index };
		}
	}
	return { last, index };
}

template<class ForwardIt, class UnaryPredicate>
ForwardIt remove_by_index(ForwardIt first, ForwardIt last, UnaryPredicate p) {
	size_t index = 0;
	std::tie(first, index) = find_by_index(first, last, p);
	if (first != last) {
		for(ForwardIt i = first; i != last; ++i, ++index) {
			if (!p(index)) {
				*first = std::move(*i);
				++first;
			}
		}
	}
	return first;
}

namespace detail {
	struct printer {
	template<typename STREAM, typename T>
		void operator()(STREAM& os, const T& t) const {
			os << t;
		}
	};
}

template<typename CONTAINER, typename OSTREAM, typename FUNC = ::util::detail::printer>
void print_container(
	const CONTAINER& c,
	OSTREAM&& os,
	const std::string& sep = ", ",
	const std::string& prefix_str = "{ ",
	const std::string& suffix_str = " }",
	FUNC func = FUNC{}
) {
	auto beg = begin(c);
	auto en = end(c);

	os << prefix_str;
	if (beg != en) {
		func(os,*beg);
		std::for_each(std::next(beg), en, [&](const auto& v){
			os << sep;
			func(os,v);
		});
	}
	os << suffix_str;
}

// template<class T, class E>
// using first = T;

template<class T, typename = void>
struct IDHasher;

template<class T>
struct IDHasher<T, std::enable_if_t<std::is_base_of<IDBase, T>::value>> {
	size_t operator()(const T& id) const {
		return std::hash<decltype(id.getValue())>()(id.getValue());
	}
};

template<class T, typename = void>
struct MyHash {
	using type = std::hash<T>;
};

template<class T>
struct MyHash<T, std::enable_if_t<std::is_base_of<IDBase, T>::value>> {
	using type = IDHasher<T>;
};

template<class T>
using MyHash_t = typename MyHash<T>::type;

template<template <typename... ARGS> class CONTAINER, typename KEY, typename... REST>
using with_my_hash_t = CONTAINER<KEY, REST..., MyHash_t<KEY>>;

} // end namespace util

#endif /* UTIL_H */
