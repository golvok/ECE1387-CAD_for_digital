
#ifndef UTIL__HANDLER_HPP
#define UTIL__HANDLER_HPP

#include <memory>

namespace util {

template<typename HANDLED_TYPE>
class unique_handle {
public:
	using handled_type = HANDLED_TYPE;
	using handled_type_ptr = HANDLED_TYPE*;
	using handled_type_ref = HANDLED_TYPE&;
	using ptr_type = std::unique_ptr<handled_type>;

	unique_handle() : ptr() { }
	unique_handle(unique_handle&&) = default;
	unique_handle& operator=(unique_handle&&) = default;

	unique_handle(ptr_type&& ptr) : ptr(std::move(ptr)) { }
	unique_handle& operator=(ptr_type&& ptr) { this->ptr = std::move(ptr); return *this; }

	explicit operator bool() const { return static_cast<bool>(ptr); }

	handled_type_ref operator*() const { return *ptr; }
	handled_type_ptr operator->() const { return get(); }

	handled_type_ptr get() const { return ptr.get(); }
private:
	ptr_type ptr;
};

template<typename HANDLED_TYPE>
class shared_handle {
public:
	using handled_type = HANDLED_TYPE;
	using handled_type_ptr = HANDLED_TYPE*;
	using handled_type_ref = HANDLED_TYPE&;
	using ptr_type = std::shared_ptr<handled_type>;

	shared_handle() : ptr() { }
	shared_handle(const shared_handle&) = default;
	shared_handle(shared_handle&&) = default;
	shared_handle& operator=(const shared_handle&) = default;
	shared_handle& operator=(shared_handle&&) = default;

	shared_handle(const ptr_type& ptr) : ptr(ptr) { }
	shared_handle& operator=(const ptr_type& ptr) { this->ptr = ptr; return *this; }

	explicit operator bool() const { return static_cast<bool>(ptr); }

	handled_type_ref operator*() const { return *ptr; }
	handled_type_ptr operator->() const { return get(); }

	handled_type_ptr get() const { return ptr.get(); }
	void reset() { ptr.reset(); }
private:
	ptr_type ptr;
};

} // end namespace util

#endif /* UTIL__HANDLER_HPP */
