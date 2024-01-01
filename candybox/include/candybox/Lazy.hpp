#ifndef CANDYBOX_LAZY_HPP__
#define CANDYBOX_LAZY_HPP__

#include <algorithm>

namespace candybox {

template <typename T>
class Lazy
{
public:
	Lazy() = default;
	explicit Lazy(const T *src) : ptr_(src ? new(&storage) T(*src) : nullptr) { }
	Lazy(const Lazy &that) : ptr_(that.ptr_ ? new(&storage) T(*that.ptr_) : nullptr) { }
	Lazy(Lazy &&that) noexcept
	    : ptr_(that.ptr_ ? new(&storage) T(std::move(*that.ptr_)) : nullptr)
	{
	}
	~Lazy() { this->reset(); }

	Lazy &operator=(const Lazy &rhs)
	{
		if (&rhs == this) return *this; // self assignment
		else if (rhs.isValid()) this->set(*rhs);
		else this->reset();
		return *this;
	}

	Lazy &operator=(Lazy &&rhs) noexcept
	{
		if (rhs.isValid()) set(std::move(*rhs));
		else reset();
		return *this;
	}

	void reset()
	{
		if (!isValid()) return;
		ptr_->~T();
		ptr_ = nullptr;
	}

	// Return true if pointer is null.
	bool isValid() const { return ptr_ != nullptr; }

	T *set(const T &src)
	{
		if (isValid()) *ptr_ = src;
		else ptr_ = new (&storage) T(src);
		return ptr_;
	}

	template <typename... Args>
	T *init(Args &&...args)
	{
		reset();
		ptr_ = new (&storage) T(std::forward<Args>(args)...);
		return ptr_;
	}

	T *set(T &&src) // rvalue reference.
	{
		if (isValid()) *ptr_ = std::move(src);
		else ptr_ = new (&storage) T(std::move(src));
		return ptr_;
	}

	T *get() const { return ptr_; }

private:
	alignas(T) char storage[sizeof(T)] = {};
	T *ptr_ = nullptr;
};

} // namespace candybox
#endif // CANDYBOX_LAZY_HPP__
