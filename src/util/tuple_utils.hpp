
#ifndef UTILS__TUPLE_UTILS_HPP
#define UTILS__TUPLE_UTILS_HPP

#include <tuple>

namespace util {

namespace detail {

	template<std::size_t... Is>
	struct seq { };

	template<std::size_t N, std::size_t... Is>
	struct gen_seq : gen_seq<N - 1, N - 1, Is...> { };

	template<std::size_t... Is>
	struct gen_seq<0, Is...> : seq<Is...> { };

	struct recursive_combiner {
		template<typename FIRST_TYPE_IN, typename COMBINER>
		auto operator()(COMBINER, FIRST_TYPE_IN&& first) {
			return first;
		}

		template<typename FIRST_TYPE_IN, typename SECOND_TYPE_IN, typename COMBINER>
		auto operator()(COMBINER comb, FIRST_TYPE_IN&& first, SECOND_TYPE_IN&& second) {
			return this->operator()(comb, comb(first, second));
		}

		template<typename FIRST_TYPE_IN, typename SECOND_TYPE_IN, typename... REST_TYPES_IN, typename COMBINER>
		auto operator()(COMBINER comb, FIRST_TYPE_IN&& first, SECOND_TYPE_IN&& second, REST_TYPES_IN&&... rest_args) {
			return this->operator()(comb, comb(first, second), rest_args...);
		}
	};

}

template<std::size_t INDEX>
struct getter {
	template<typename TUPLE>
	auto& operator()(TUPLE& t) {
		return std::get<INDEX>(t);
	}
};

namespace detail {

	template<typename TUPLE_IN, typename F, std::size_t... Is>
	auto map_tuple_impl(TUPLE_IN&& t, F f, seq<Is...>) {
		return std::make_tuple(
			(f(
				std::get<Is>(t)
			))...
		);
	}

	template<typename TUPLE_IN, typename... OTHER_ARGS, std::size_t... Is, typename F>
	auto unpack_into_impl(F f, seq<Is...>, TUPLE_IN&& t, OTHER_ARGS&&... other_args) {
		return f(
			other_args..., std::get<Is>(t)...
		);
	}
}

template<typename TUPLE_IN, typename... OTHER_ARGS, typename F>
auto unpack_into(F f, TUPLE_IN&& t, OTHER_ARGS&&... other_args) {
	using PURE_TUPLE_IN = typename std::remove_reference<TUPLE_IN>::type;
	return detail::unpack_into_impl(f, detail::gen_seq<std::tuple_size<PURE_TUPLE_IN>::value>(), t, other_args...);
}

namespace detail {

	template<typename TUPLE_OUT, typename TUPLE_IN, typename F, std::size_t... Is>
	TUPLE_OUT map_tuple_impl(TUPLE_IN&& t, F f, seq<Is...>) {
		return TUPLE_OUT( (f(std::get<Is>(t)))... );
	}

}

template<typename TUPLE_OUT, typename TUPLE_IN, typename F>
auto map_tuple(const TUPLE_IN& t, F f) {
	return detail::map_tuple_impl<TUPLE_OUT>(t, f, detail::gen_seq<std::tuple_size<TUPLE_IN>::value>());
}

template<typename TUPLE_IN, typename F>
auto map_tuple(const TUPLE_IN& t, F f) {
	return detail::map_tuple_impl(t, f, detail::gen_seq<std::tuple_size<TUPLE_IN>::value>());
}

namespace detail {

	template<std::size_t INDEX, typename... TUPLES_IN>
	auto one_from_each(std::tuple<TUPLES_IN&...>&& tuples) {
		return map_tuple(tuples, getter<INDEX>());
	}

}

namespace detail {

	template<typename F, std::size_t... Is, typename... TUPLES_IN>
	auto map_multi_tuple_impl(F f, seq<Is...>, TUPLES_IN&&... tuples) {
		return std::make_tuple(
			(unpack_into(f, one_from_each<Is>(std::tie(tuples...))))...
		);
	}

}

template<typename... TUPLES_IN, typename F>
auto map_multi_tuple(F f, const TUPLES_IN&... tuples) {
	return detail::map_multi_tuple_impl(
		f,
		detail::gen_seq<std::max( // max?, min?, any? it doesn't really work if the sizes are different...
			(std::tuple_size<TUPLES_IN>::value) ...
		)>(),
		tuples...
	);
}

template<typename RESULT, typename TUPLE_IN, typename COMBINER>
RESULT combine_tuple(
	RESULT initial,
	COMBINER comb,
	TUPLE_IN&& tuple
) {
	return unpack_into(detail::recursive_combiner(), tuple, comb, initial);
}

template<typename RESULT, typename CONVERTER, typename COMBINER, typename... TUPLES_IN>
RESULT combine_tuples(
	RESULT initial,
	CONVERTER conv,
	COMBINER comb,
	TUPLES_IN&... tuples
) {
	return combine_tuple(initial, comb, map_multi_tuple(conv, tuples...));
}

} // end namespace util

#endif /* UTILS__TUPLE_UTILS_HPP */
