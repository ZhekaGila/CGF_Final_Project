#pragma once

#include <vector>
#include "geometry/MeshData.h"

namespace cgf {

// Creates all 3D objects that belong to the concert stage.
std::vector<ScenePart> createStageGeometry();

} // namespace cgf