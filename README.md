# glRoom

A small application implementing modern OpenGL 4.6 rendering techniques alongwith ECS and physics functionality.

**Controls**: Left Click to pick and move objects, Right Click and Wheel to move the camera.\
Press 2 or 3 to enable / Press 1 to disable : Debug mode regarding bullet physics.

https://github.com/chirag9510/glRoom/assets/78268919/8ebb3d7d-bc32-4986-92d0-fbbf1b3d5ec5

**Please note about Bindless Textures**: If you don't have a dedicated NVidia or AMD graphics card with support for bindless textures, the application will appear as blank because of all the textures will be missing. Very few intel integrated graphics support bindless textures.

# Functionality
## Engine
Physics functionality with [Bullet physics](https://github.com/bulletphysics/bullet3)\
Instanced Indirect Draw (GL_DRAW_INDIRECT_BUFFER) rendering with Direct State Access (DSA)\
Post Processing\
Bindless textures\
Stencil culling\
ECS with [entt](https://github.com/skypjack/entt)\
Observer pattern among systems\
\
Only 3 Draw calls are made with glMultiDrawElementsIndirect() regarding rendering of meshes since each mesh and its submesh is assigned a Vertex Array Object based on its material. Only a single indirect draw buffer is used where VAOs access their respective rendering data from the DrawElementsIndirectCommand data structure via an offset in memory. 

## Post processing  
Extended Reinhard tonemapping\
Gamma correction\
Bloom with multiple framebuffers and subroutines

## Also includes
[nuklear gui](https://github.com/Immediate-Mode-UI/Nuklear)\
[tinyobjloader](https://github.com/tinyobjloader/tinyobjloader)\
[spdlog](https://github.com/gabime/spdlog)\
Blender for level design and scripting

After compiling the application under the **x64 Release**, copy the contents of the **bin_cpy** folder into the **assets** folder after creating it inside the **executable folder (./assets/)**. The assets folder path can also be changed in the **set.ini** file which resides with the executable. \
Or download the released **glRoom.zip** and run the application right away.

# Blender scripting
Open the **level.blend file in bin_cpy folder** and run the python script through the Blender application. This exports the level.txt file with all the entities and assets adjusted for bullet physics parsed automatically by the application when placed in **assets** folder.

![blender](https://github.com/chirag9510/glRoom/assets/78268919/948a0575-aecb-443a-845c-828cee67ba58)

