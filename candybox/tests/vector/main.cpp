//
// Created by Amias on 2023/12/31.
//
#include <catch2/catch_session.hpp>


int
main(int argc, char *argv[])
{
	int result = Catch::Session().run(argc, argv);

	return result;
}
