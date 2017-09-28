#ifndef UTIL__TEMPLATE_UTILS_H
#define UTIL__TEMPLATE_UTILS_H

namespace util {

template<typename... Ts>
struct type_vector { };

template<template <typename> class Subst, typename... Devices>
void forceInstantiation(type_vector<Devices...>) {
	(void)std::make_tuple(Subst<Devices>::func()..., 0);
}

template<typename T>
using add_pointer_to_const_t = std::add_pointer_t<std::add_const_t<T>>;

template <typename T>
using same_type = T;

template< template <typename...> class Container, template <typename> class Adaptor = same_type, typename... Ts>
using substitute_into = Container<Adaptor<Ts>...>;

}

#endif // UTIL__TEMPLATE_UTILS_H
