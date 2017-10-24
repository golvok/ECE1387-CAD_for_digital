
#ifndef UTIL__GENERATOR_HPP
#define UTIL__GENERATOR_HPP

#include <functional>
#include <util/tuple_utils.hpp>

namespace util {

namespace detail {
	struct identity {
	template<typename U>
		constexpr auto operator()(U&& v) const noexcept -> decltype(std::forward<U>(v)) {
			return std::forward<U>(v);
		}
	};
}

template<typename INDEX_TYPE>
struct generator_base {
	using index_type = INDEX_TYPE;
	auto transform(const index_type& index) { return detail::identity()(index); }
};

template<typename INDEX_TYPE, typename NEXT, typename DONE, typename TRANSFORM>
class generator_iterator : public std::iterator<std::forward_iterator_tag, const INDEX_TYPE, std::ptrdiff_t, const INDEX_TYPE*, const INDEX_TYPE> {
private:
	INDEX_TYPE current;
	NEXT next;
	DONE done;
	TRANSFORM transform;
	bool is_end_iterator;

public:
	generator_iterator(INDEX_TYPE current, NEXT next, DONE done, TRANSFORM transform, bool is_end_iterator)
		: current(current)
		, next(next)
		, done(done)
		, transform(transform)
		, is_end_iterator(is_end_iterator)
	{ }

	generator_iterator& operator++() {
		current = next(std::move(current));
		return *this;
	}

	bool operator==(const generator_iterator& rhs) const {
		if (rhs.is_end_iterator && is_end_iterator) {
			return true;
		} else if (rhs.is_end_iterator) {
			return rhs.done(current);
		} else if (is_end_iterator) {
			return done(rhs.current);
		} else {
			return current == rhs.current;
		}
	}

	bool operator!=(const generator_iterator& rhs) const { return !(*this == rhs); }

	auto operator*() const {
		return transform(current);
	}
};

template<typename INDEX_TYPE, typename NEXT, typename DONE, typename TRANSFORM>
class generator {
public:
	using iterator = generator_iterator<INDEX_TYPE,NEXT,DONE,TRANSFORM>;
	using const_iterator = generator_iterator<INDEX_TYPE,NEXT,DONE,TRANSFORM>;
	using reference = typename std::iterator_traits<iterator>::reference;
private:
	INDEX_TYPE current;
	DONE done;
	NEXT next;
	TRANSFORM transform;

public:
	generator(INDEX_TYPE initial, DONE done, NEXT next, TRANSFORM transform)
		: current(initial)
		, done(done)
		, next(next)
		, transform(transform)
	{ }

	iterator begin() const {
		return iterator(current,next,done,transform,false);
	}

	iterator end() const {
		return iterator(current,next,done,transform,true);
	}
};

template<typename INDEX_TYPE, typename NEXT, typename DONE, typename TRANSFORM>
auto begin(const generator<INDEX_TYPE,NEXT,DONE,TRANSFORM>& gen) {
	return gen.begin();
}

template<typename INDEX_TYPE, typename NEXT, typename DONE, typename TRANSFORM>
auto end(const generator<INDEX_TYPE,NEXT,DONE,TRANSFORM>& gen) {
	return gen.end();
}

template<typename INDEX_TYPE, typename PTYPE1, typename NEXT, typename DONE, typename TRANSFORM = detail::identity>
auto make_generator(PTYPE1 initial, DONE done, NEXT next, TRANSFORM transform = TRANSFORM(), decltype(transform(initial),done(initial),next(std::move(initial)))* = nullptr) {
	return generator<INDEX_TYPE,NEXT,DONE,TRANSFORM>(
		initial,
		done,
		next,
		transform
	);
}

template<
	typename INDEX_TYPE, typename PTYPE1, typename PTYPE2, typename NEXT, typename TRANSFORM = detail::identity,
	typename = std::enable_if_t<
		std::is_convertible<PTYPE1,INDEX_TYPE>::value && std::is_convertible<PTYPE2,INDEX_TYPE>::value
	>
>
auto make_generator(PTYPE1 initial, PTYPE2 past_end, NEXT next, TRANSFORM transform = TRANSFORM(), decltype(transform(initial),next(std::move(initial)))* = nullptr) {
	auto done = [=](const INDEX_TYPE& current ) { return current == (INDEX_TYPE)past_end; };
	return generator<INDEX_TYPE, NEXT, decltype(done), TRANSFORM>(
		initial,
		done,
		next,
		transform
	);
}

template<typename GEN>
auto make_generator(GEN&& gen) {
	using index_type = typename GEN::index_type;
	return make_generator<index_type>(
		gen.initial(),
		[&](const index_type& current) { return gen.done(current); },
		[&](const index_type& current) { return gen.next(current); },
		[&](const index_type& current) { return gen.transform(current); }
	);
}

template<
	typename INDEX_TYPE, typename PTYPE1, typename PTYPE2, typename TRANSFORM = detail::identity,
	typename = std::enable_if_t<
		std::is_convertible<PTYPE1,INDEX_TYPE>::value && std::is_convertible<PTYPE2,INDEX_TYPE>::value
	>
>
auto xrange_forward_pe(const PTYPE1& start, const PTYPE2& end, TRANSFORM transform = TRANSFORM()) {
	auto next = [](INDEX_TYPE i) { return ++i; };
	auto done = [=](INDEX_TYPE i) { return i == end; };
	return generator<INDEX_TYPE, decltype(next), decltype(done), TRANSFORM>(
		start,
		done,
		next,
		transform
	);
}

template<
	typename INDEX_TYPE, typename PTYPE1, typename PTYPE2, typename TRANSFORM = detail::identity,
	typename = std::enable_if_t<
		std::is_convertible<PTYPE1,INDEX_TYPE>::value && std::is_convertible<PTYPE2,INDEX_TYPE>::value
	>
>
auto xrange_forward(const PTYPE1& start, const PTYPE2& end, TRANSFORM transform = TRANSFORM()) {
	auto next = [](INDEX_TYPE i) { return ++i; };
	auto done = [=](INDEX_TYPE i) { return i == end + 1; };
	return generator<INDEX_TYPE, decltype(next), decltype(done), TRANSFORM>(
		start,
		done,
		next,
		transform
	);
}

template<
	typename INDEX_TYPE, typename PTYPE1, typename PTYPE2, typename TRANSFORM = detail::identity,
	typename = std::enable_if_t<
		std::is_convertible<PTYPE1,INDEX_TYPE>::value && std::is_convertible<PTYPE2,INDEX_TYPE>::value
	>
>
auto xrange_backward(const PTYPE1& start, const PTYPE2& end, TRANSFORM transform = TRANSFORM()) {
	auto next = [](INDEX_TYPE i) { return --i; };
	auto done = [=](INDEX_TYPE i) { return i == end - 1; };
	return generator<INDEX_TYPE, decltype(next), decltype(done), TRANSFORM>(
		start,
		done,
		next,
		transform
	);
}

template<
	typename INDEX_TYPE, typename PTYPE1, typename PTYPE2, typename TRANSFORM = detail::identity,
	typename = std::enable_if_t<
		std::is_convertible<PTYPE1,INDEX_TYPE>::value && std::is_convertible<PTYPE2,INDEX_TYPE>::value
	>
>
auto xrange(const PTYPE1& start, const PTYPE2& end, TRANSFORM transform = TRANSFORM()) {
	const bool forwards = static_cast<INDEX_TYPE>(end) >= static_cast<INDEX_TYPE>(start);
	auto next = [=](INDEX_TYPE i) { return forwards ? ++i : --i; };
	auto done = [=](INDEX_TYPE i) { return i == end + (forwards ? 1 : -1); };
	return generator<INDEX_TYPE, decltype(next), decltype(done), TRANSFORM>(
		start,
		done,
		next,
		transform
	);
}

template<typename INDEX_TYPE, typename PTYPE1, typename TRANSFORM = detail::identity>
auto xrange(const PTYPE1& end, TRANSFORM transform = TRANSFORM(), decltype(transform(end),-1)* = nullptr) {
	return xrange<INDEX_TYPE>(0,end,transform);
}

struct dereferencer {
	template<typename T, typename RETTYPE = typename std::iterator_traits<T>::reference >
	RETTYPE operator()(T& elem) {
		return *elem;
	}
};

template<typename... COLLECTION_TYPES>
auto zip(COLLECTION_TYPES&... containers) {
	using iterator_tuple = std::tuple<typename COLLECTION_TYPES::iterator...>;
	using reference_tuple = std::tuple<typename COLLECTION_TYPES::reference...>;

	iterator_tuple end_iter_tuple(end(containers)...);

	auto next = [=](const iterator_tuple& i) -> iterator_tuple {
		return ::util::map_tuple(i, [](auto& elem) {
			return std::next(elem);
		});
	};

	auto done = [=](const iterator_tuple& i) -> bool {
		return combine_tuples(
			true,
			[](auto& eleml, auto& elemr) -> bool {
				return eleml == elemr;
			},
			[](bool lhs, bool rhs) -> bool {
				return lhs || rhs;
			},
			i,
			end_iter_tuple
		);
	};

	auto transform = [=](const iterator_tuple& i) -> reference_tuple {
		return ::util::map_tuple<reference_tuple>(i, dereferencer());
	};

	return generator<iterator_tuple, decltype(next), decltype(done), decltype(transform)>(
		iterator_tuple(begin(containers)...),
		done,
		next,
		transform
	);
}

template<typename CONTAINER>
auto iterate_with_iterators(CONTAINER& c) {
	return xrange_forward_pe<decltype(begin(c))>(
		begin(c),
		end(c)
	);
}

} // end namespace util

#endif /* UTIL__GENERATOR_HPP */
