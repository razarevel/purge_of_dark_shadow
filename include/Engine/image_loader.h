#pragma once
#include "ktx.h"
#include <string>


struct ImageLoader {

	static ktxTexture1* fromfile(std::string filename);
};