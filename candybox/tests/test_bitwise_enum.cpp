#include "candybox/greatest.h"
#include "candybox/bitwise_enum.hpp"

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

TEST
test()
{
	ASSERT_EQ((e1 | e2), e3);
	ASSERT_EQ((E2::e1 | E2::e2), E2::e3);

	ASSERT_EQ((e1 & e2), none);
	ASSERT_EQ((E2::e1 & E2::e2), E2::none);

	ASSERT_EQ((e3 & ~e1), e2);
	ASSERT_EQ((E2::e3 & ~E2::e1), E2::e2);

	PASS();
}

SUITE(the_suite) { RUN_TEST(test); }

GREATEST_MAIN_DEFS();

int
main(int argc, char **argv)
{
	GREATEST_MAIN_BEGIN();
	RUN_SUITE(the_suite);
	GREATEST_MAIN_END();
}
