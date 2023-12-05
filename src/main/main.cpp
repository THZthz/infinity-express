#include <algorithm>
#include <cassert>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>
#include <cstdint>

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
#include "Physics/DebugDraw.hpp"
#include "Physics/World.hpp"
#include "Scenes/Main.hpp"
#include "Generator/CellAutomata.hpp"
#include "utils/Array.hpp"
#include "utils/tests/tests_Array.hpp"

int
main()
{
	printf("Infinity Express v%s\n", INFINITY_EXPRESS_VERSION_STR);
	printf("\tworking directory %s\n", INFINITY_EXPRESS_WORKING_DIR);
	printf("\tc++ standard version: %ld\n", __cplusplus);

	//	App app;
	//	app.start();

	CellAutomata gen;
	gen.generate();

	test_Array();


	return EXIT_SUCCESS;
}
