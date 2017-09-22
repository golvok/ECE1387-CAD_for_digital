
#ifndef UTIL__ITERATION_UTILS_H
#define UTIL__ITERATION_UTILS_H

#include <util/utils.hpp>

#include <initializer_list>
#include <iterator>
#include <array>

/*************
 * Begin definition of Iterator Pair Adapter. See end for usage
 *************/

template<typename PAIR_TYPE>
class iterator_pair_adapter {
	PAIR_TYPE t;
public:
	iterator_pair_adapter(const PAIR_TYPE& t) : t(t) { }
	iterator_pair_adapter(const iterator_pair_adapter& src) : t(src.t) { }
	iterator_pair_adapter(PAIR_TYPE&& t) : t(std::move(t)) { }
	iterator_pair_adapter(iterator_pair_adapter&& src) : t(std::move(src.t)) { }

	iterator_pair_adapter& operator=(const iterator_pair_adapter& src) { t = src.t; }
	iterator_pair_adapter& operator=(iterator_pair_adapter&& src) { t = std::move(src.t); }

	auto begin() { return t.first; }
	auto end() { return t.second; }
};

/**
 * takes a pair of iterators, and lets you iterate between them with the
 * for-range syntax
 * Example:
 *
 * std::pair<iterator_type> it_pair = get_begin_and_end()
 * for (auto elem : make_iterable(it_pair)) {
 *     std::cout << elem << '\n';
 * }
 */
template<typename PAIR_TYPE>
auto make_iterable(PAIR_TYPE&& src) {
	return iterator_pair_adapter<PAIR_TYPE>(std::forward<PAIR_TYPE>(src));
}

/*************
 * Begin definition of Index Associative Iterator. See end for usage
 *************/

template<typename IA_ITERATOR>
class index_associative_iteratior_value {
	const IA_ITERATOR* src;
public:

	auto it() const { return src->it(); }
	auto i() const { return src->i(); }
	auto& v() const { return src->v(); }

	index_associative_iteratior_value(const IA_ITERATOR& src) : src(&src) {}
	index_associative_iteratior_value(const index_associative_iteratior_value& src) = default;

	index_associative_iteratior_value& operator=(const index_associative_iteratior_value&) = delete;

	auto& operator*() const { return v(); }
};

template<typename ITERATOR, typename INDEX>
class index_associative_iteratior {
public:
	using iterator_type = ITERATOR;
	using size_type     = INDEX;
	using value_type    = index_associative_iteratior_value<index_associative_iteratior>;
private:
	iterator_type iter;
	size_type index;
public:
	index_associative_iteratior(const iterator_type& it) : iter(it), index(0) {}

	index_associative_iteratior(const index_associative_iteratior&) = default;
	index_associative_iteratior& operator=(const index_associative_iteratior&) = delete;

	index_associative_iteratior& operator++() {
		++iter;
		++index;
		return *this;
	}

	bool operator==(const index_associative_iteratior& rhs) const { return iter == rhs.iter; }
	bool operator!=(const index_associative_iteratior& rhs) const { return iter != rhs.iter; }
	value_type operator*() const { return value_type(*this); }

	iterator_type it() const { return iter; }
	size_type i() const { return index; }
	auto& v() const { return *iter; }
};

template<typename CONTAINER, typename ITERATOR>
class index_associative_iteratior_adapter {
	CONTAINER& c;
public:
	using size_type = typename CONTAINER::size_type;
	using iterator_type = index_associative_iteratior<
		ITERATOR,
		size_type
	>;

	index_associative_iteratior_adapter(CONTAINER& c) : c(c) {}
	index_associative_iteratior_adapter(const index_associative_iteratior_adapter& src) = default;

	index_associative_iteratior_adapter& operator=(const index_associative_iteratior_adapter&) = delete;

	iterator_type begin() { using std::begin; return iterator_type(begin(c)); }
	iterator_type end() { using std::end; return iterator_type(end(c)); }
};

/**
 * Allows for iteration using the for-range syntax, but also gives you an id
 * to use. Useful for using the index as the key for a map, or into another vector.
 *
 * Example:
 * std::vector<int> some_ints { 1, 5, 7, 8, 1, };
 * for (auto iter : index_assoc_iterate(some_ints)) {
 *     std::cout << "There's a " iter.v() << " at " << iter.i() << '\n';
 * }
 *
 * Example's Output:
 * There's a 1 at 0
 * There's a 5 at 1
 * There's a 7 at 2
 * There's a 8 at 3
 * There's a 1 at 4
 *
 * Future Directions:
 * Maybe make it store a reference to the container, and just access it when
 * we need it() ? Won't work well for std::list, though.
 */
template<typename CONTAINER>
auto index_assoc_iterate(CONTAINER& c) {
	return index_associative_iteratior_adapter<CONTAINER,typename CONTAINER::iterator>(c);
}
template<typename CONTAINER>
auto index_assoc_iterate(const CONTAINER& c) {
	return index_associative_iteratior_adapter<const CONTAINER,typename CONTAINER::const_iterator>(c);
}

/*************
 * Begin definition of Container Concatenation Adapter. See end for usage
 *************/

template<typename ADAPTER>
class container_concat_adapter_iterator {
public:
	ADAPTER& adapter;

	using container_iter_type = typename std::remove_reference<decltype(adapter.get_containers().begin())>::type;
	container_iter_type container_iter;

	using iterator_type = typename std::remove_reference<decltype((*adapter.get_containers().begin())->begin())>::type;
	iterator_type iter;

	container_concat_adapter_iterator(ADAPTER& a, container_iter_type ci, iterator_type i)
		: adapter(a)
		, container_iter(ci)
		, iter(i)
	{
		skip_empties();
	}

	container_concat_adapter_iterator(const container_concat_adapter_iterator&) = default;
	container_concat_adapter_iterator& operator=(const container_concat_adapter_iterator&) = default;

	auto operator*() { return *iter; }

	void skip_empties() {
		while ((container_iter+1) != adapter.get_containers().end() && iter == (*container_iter)->end()) {
			++container_iter;
			iter = (*container_iter)->begin();
		}
	}

	container_concat_adapter_iterator& operator++() {
		++iter;
		skip_empties();
		return *this;
	}

	bool operator==(const container_concat_adapter_iterator& rhs) const {
		return container_iter == rhs.container_iter && iter == rhs.iter;
	}
	bool operator!=(const container_concat_adapter_iterator& rhs) const {
		return container_iter != rhs.container_iter || iter != rhs.iter;
	}
};

template<typename CONTAINER, size_t SIZE_MINUS_ONE>
class container_concat_adapter {
public:
	std::array<CONTAINER*,SIZE_MINUS_ONE + 1> containers;
	using container_type = CONTAINER;
	using size_type      = typename CONTAINER::size_type;
	using iterator_type  = container_concat_adapter_iterator<container_concat_adapter>;

	container_concat_adapter(const decltype(containers)&& cs)
		: containers(cs)
	{ }

	container_concat_adapter(const container_concat_adapter& src) = default;

	container_concat_adapter& operator=(const container_concat_adapter&) = delete;

	iterator_type begin() {
		return iterator_type(*this, containers.begin(), (*containers.begin())->begin());
	}

	iterator_type end() {
		return iterator_type(*this, (containers.end()-1), (*(containers.end()-1))->end());
	}

	auto& get_containers() { return containers; }
};

/**
 * Allows runtime "concatination" of containers, from the perspective of iteration.
 * Ie. iterate over a set of containers as if the were one, but without actually
 * combining or copying them. All containers must be convertable to the type of the first.
 * Any number of containers may be empty.
 * Example:
 *
 * vector<int> v1 { 1, 2, 3};
 * vector<int> v2 { };
 * vector<int> v3 { 4, 5, 6};
 *
 * // this will print 1 2 3 4 5 6
 * for (auto node : iterate_all(v1,v2,v3)) {
 *     std::cout << node << ' ';
 * }
 *
 */
template<typename CONTAINER, typename... REST>
auto iterate_all(CONTAINER& c_first, REST&... c_rest) {
	return container_concat_adapter<CONTAINER, sizeof...(REST)>({&c_first, (&c_rest)...});
}

/*******************
 * Begin definition of iterator_iterator_adapter
 *******************/

template<typename ITER>
class iterator_iterator_adapter {
public:

	class iterator_iterator_adapter_iterator { // : public ITER {
	public:
		ITER wrapped_iter;
		ITER& operator*() { return wrapped_iter; }
		const ITER& operator*() const { return wrapped_iter; }
		bool operator!=(const iterator_iterator_adapter_iterator& rhs) const { return this->wrapped_iter != rhs.wrapped_iter; }
		bool operator==(const iterator_iterator_adapter_iterator& rhs) const { return !(*this != rhs); }
		iterator_iterator_adapter_iterator& operator++() { ++wrapped_iter; return *this; }
		iterator_iterator_adapter_iterator operator++(int) { using std::next; return next(*this); }
	};

	iterator_iterator_adapter_iterator begin_iter;
	iterator_iterator_adapter_iterator end_iter;

	iterator_iterator_adapter(ITER b, ITER e)
		: begin_iter{b}, end_iter{e}
	{ }

	iterator_iterator_adapter_iterator begin() { return begin_iter; }
	iterator_iterator_adapter_iterator end() { return end_iter; }

};

template<typename CONTAINER>
auto iterate_as_iterators(CONTAINER& c) {
	return iterator_iterator_adapter<decltype(c.begin())>(c.begin(), c.end());
}

#endif /* UTIL__ITERATION_UTILS_H */
