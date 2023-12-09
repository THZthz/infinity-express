#include <algorithm>
#include <cassert>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>
#include <cstdint>
#include <cstdlib>

// implements nanovg
#define FONS_SDF
#include "nanovg.h"
#define NANOVG_GL3 1
#define NANOVG_GL_IMPLEMENTATION
#include "nanovg_gl.h"
#define NANOVG_GLU_IMPLEMENTATION
#include "nanovg_gl_utils.h"
#undef NANOVG_GL_IMPLEMENTATION
#undef NANOVG_GLU_IMPLEMENTATION

#include "stb_image.h"
#include "utils/Scene.hpp"
#include "utils/Color.hpp"
#include "utils/TaskScheduler.hpp"
#include "utils/Linear.hpp"
#include "utils/vector.hpp"
#include "Main.hpp"
#include "CellAutomata.hpp"

#include "utils/tests/tests_Array.hpp"
#include "utils/tests/tests_Memory.hpp"
#include "utils/tests/tests_BitwiseEnum.hpp"
#include "utils/tests/tests_Optional.hpp"


int
main()
{
	test_Array();
	test_Memory();
	test_BitwiseEnum();
	test_optional();

	//	App app;
	//	app.start();

	//	CellAutomata gen;
	//	gen.generate();


	return EXIT_SUCCESS;
}
