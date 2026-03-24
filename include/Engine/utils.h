#pragma once

#include <string>
#include <ktx.h>
#include "Engine/stbi_image.h"
#include <cstdint>

std::string getFilename(std::string& filename);

ktxTexture1* loadKtxFromFile(std::string& filename);