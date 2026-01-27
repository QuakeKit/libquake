add_library(${PROJECT_NAME} SHARED
        common/surface.cpp
        common/vertex.cpp
        common/entity.cpp
        common/entity_parser.cpp

        bsp/qbsp.cpp
        bsp/qbsp_provider.cpp
        bsp/entity_solid.cpp
        bsp/surface.cpp
        bsp/lightmap.cpp

        map/map_file.cpp
        map/map.cpp
        map/brush.cpp
        map/qmath.cpp
        map/face.cpp
        map/entity_solid.cpp
        map/lightmap_generator.cpp
        map/qmap_provider.cpp

        wad/palette.cpp
        wad/wad.cpp
        wad/texture.cpp
        wad/wad_manager.cpp

        utils/xatlas/xatlas.cpp

        wrapper.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC "../include/")
