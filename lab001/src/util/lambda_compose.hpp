
#ifndef UTIL__LAMBDA_VISITOR_H
#define UTIL__LAMBDA_VISITOR_H

// from a stack overflow answer
// TODO: find the link when I have internet

#include <functional>
#include <type_traits>

namespace util {

namespace detail {

	template<typename BASE, typename... Lambdas>
	struct composed_lambda;

	template<typename BASE, typename Lambda1, typename... Lambdas>
	struct composed_lambda<BASE, Lambda1, Lambdas...>
		: public composed_lambda<BASE, Lambdas...>,
		  public Lambda1
	{
		using Lambda1::operator ();
		using composed_lambda<BASE, Lambdas...>::operator ();

		composed_lambda(Lambda1 l1, Lambdas... lambdas)
			: composed_lambda<BASE, Lambdas...>(lambdas...)
			, Lambda1(l1)
		{ }
	};

	template<typename BASE, typename Lambda1>
	struct composed_lambda<BASE, Lambda1> : public Lambda1, public BASE {
		using Lambda1::operator ();

		composed_lambda(Lambda1 l1)
			: Lambda1(l1)
		{ }
	};
}

template<typename BASE, typename... Fs>
auto compose(Fs&& ...fs) {
	using composed_type = detail::composed_lambda<BASE, std::decay_t<Fs>...>;
	return composed_type(std::forward<Fs>(fs)...);
}

} // end namespace util

#endif // UTIL__LAMBDA_VISITOR_H
