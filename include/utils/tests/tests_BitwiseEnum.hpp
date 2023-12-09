#pragma once
#include "utils/bitwise_enum.hpp"

namespace {

using namespace ie::bitwise_enum;

enum E1
{
	none,
	e1 = 1,
	e2 = 2,
	e3 = 3,
	e4 = 4,
};

enum class E2
{
	none,
	e1 = 1,
	e2 = 2,
	e3 = 3,
	e4 = 4,
};

} // namespace

void
test_BitwiseEnum()
{
	assert((e1 | e2) == e3);
	assert((E2::e1 | E2::e2) == E2::e3);

	assert((e1 & e2) == none);
	assert((E2::e1 & E2::e2) == E2::none);

	assert((e3 & ~e1) == e2);
	assert((E2::e3 & ~E2::e1) == E2::e2);
}
