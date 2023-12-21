#pragma once

#include "candybox/value_ptr.hpp"

// Thanks to value_ptr we get value semantics for free:

struct Widget
{
	explicit Widget(int x);

	int next() const;

	struct Pimpl;
	ie::value_ptr<Pimpl> ptr;
};

// in source file:

struct Widget::Pimpl
{
	int x;

	explicit Pimpl(int v) : x(v) { }

	int next() { return ++x; }
};


Widget::Widget(int x) : ptr(Widget::Pimpl(x)) { } // or: ptr( x )

int
Widget::next() const
{
	return ptr->next();
}

void
test_value_ptr()
{
	Widget w1(42);
	Widget w2(w1);

	assert(w1.next() == 43);
	assert(w2.next() == 43);
}
