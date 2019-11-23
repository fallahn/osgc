#include "App.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define LIGHTMAPPER_IMPLEMENTATION
//#define LM_DEBUG_INTERPOLATION
#include "lightmapper.h"

int main(int argc, char* argv[])
{
    App app;
    app.run();

	return 0;
}