add_library(${PROJECT_NAME} SHARED
        common/surface.cpp
        common/vertex.cpp
        common/entity.cpp
        common/entity_parser.cpp

        map/map_file.cpp
        map/map.cpp
        map/brush.cpp
        map/qmath.cpp
        map/face.cpp
        map/entity_solid.cpp
        map/lightmap_generator.cpp
        map/wrapper.cpp

        wad/palette.cpp
        wad/wad.cpp
        wad/texture.cpp
        wad/wad_manager.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC "../include/")
