find_package(Boost ${BOOST_MIN_VERSION} REQUIRED COMPONENTS program_options)
find_package(PostgreSQL REQUIRED)

include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.13.7)
FetchContent_MakeAvailable(Catch2)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.19.0") 
    # for cmake 3.19+ https://cmake.org/cmake/help/git-stage/policy/CMP0110.html
    # might not be needed if catch updates its cmakes
    cmake_policy(SET CMP0110 OLD)
endif()

add_executable(mapnik-test-unit 
    unit/run.cpp
    unit/color/css_color.cpp
    unit/core/box2d_test.cpp
    unit/core/comparison_test.cpp
    unit/core/conversions_test.cpp
    unit/core/copy_move_test.cpp
    unit/core/exceptions_test.cpp
    unit/core/expressions_test.cpp
    unit/core/params_test.cpp
    unit/core/transform_expressions_test.cpp
    unit/core/value_test.cpp
    unit/datasource/csv.cpp
    unit/datasource/gdal.cpp
    unit/datasource/geobuf.cpp
    unit/datasource/geojson.cpp
    unit/datasource/memory.cpp
    unit/datasource/ogr.cpp
    unit/datasource/postgis.cpp
    unit/datasource/shapeindex.cpp
    unit/datasource/spatial_index.cpp
    unit/datasource/topojson.cpp
    unit/font/fontset_runtime_test.cpp
    unit/geometry/centroid.cpp
    unit/geometry/closest_point.cpp
    unit/geometry/geometry.cpp
    unit/geometry/geometry_envelope_test.cpp
    unit/geometry/geometry_hit_test.cpp
    unit/geometry/geometry_is_simple.cpp
    unit/geometry/geometry_is_valid.cpp
    unit/geometry/geometry_reprojection.cpp
    unit/geometry/geometry_strategy_test.cpp
    unit/geometry/geometry_test_helper.cpp
    unit/geometry/grid_vertex_converter.cpp
    unit/geometry/interior.cpp
    unit/geometry/is_clockwise.cpp
    unit/geometry/is_empty.cpp
    unit/geometry/polygon_vertex_processor.cpp
    unit/geometry/polylabel.cpp
    unit/geometry/remove_empty.cpp
    unit/imaging/image.cpp
    unit/imaging/image_apply_opacity.cpp
    unit/imaging/image_filter.cpp
    unit/imaging/image_io_test.cpp
    unit/imaging/image_is_solid.cpp
    unit/imaging/image_painted_test.cpp
    unit/imaging/image_premultiply.cpp
    unit/imaging/image_set_pixel.cpp
    unit/imaging/image_view.cpp
    unit/imaging/tiff_io.cpp
    unit/imaging/webp_io.cpp
    unit/map/background.cpp
    unit/numerics/enumeration.cpp
    unit/numerics/safe_cast.cpp
    unit/pixel/agg_blend_src_over_test.cpp
    unit/pixel/palette.cpp
    unit/projection/proj_transform.cpp
    unit/renderer/buffer_size_scale_factor.cpp
    unit/renderer/cairo_io.cpp
    unit/renderer/feature_style_processor.cpp
    unit/serialization/wkb_formats_test.cpp
    unit/serialization/wkb_test.cpp
    unit/serialization/xml_parser_trim.cpp
    unit/sql/sql_parse.cpp
    unit/svg/svg_parser_test.cpp
    unit/svg/svg_path_parser_test.cpp
    unit/svg/svg_renderer_test.cpp
    unit/symbolizer/marker_placement_vertex_last.cpp
    unit/symbolizer/markers_point_placement.cpp
    unit/symbolizer/symbolizer_test.cpp
    unit/text/script_runs.cpp
    unit/text/shaping.cpp
    unit/text/text_placements_list.cpp
    unit/text/text_placements_simple.cpp
    unit/util/char_array_buffer.cpp
    unit/vertex_adapter/clipping_test.cpp
    unit/vertex_adapter/extend_converter.cpp
    unit/vertex_adapter/line_offset_test.cpp
    unit/vertex_adapter/offset_converter.cpp
    unit/vertex_adapter/simplify_converters_test.cpp
    unit/vertex_adapter/transform_path_adapter.cpp
    unit/vertex_adapter/vertex_adapter.cpp
)
set_target_properties(mapnik-test-unit PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY "${MAPNIK_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${MAPNIK_OUTPUT_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${MAPNIK_OUTPUT_DIR}/lib"
)
target_link_libraries(mapnik-test-unit PUBLIC
    Catch2::Catch2 
    mapnik::agg
    mapnik::mapnik
    mapnik::json
    mapnik::wkt
    PostgreSQL::PostgreSQL
)
# workaround since the "offical" include dir would be <catch2/catch.hpp>
file(COPY catch_ext.hpp DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(COPY cleanup.hpp DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(mapnik-test-unit PRIVATE "${catch2_SOURCE_DIR}/single_include/catch2" ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(agg_rasterizer_integer_overflow_test standalone/agg_rasterizer_integer_overflow_test.cpp)
set_target_properties(agg_rasterizer_integer_overflow_test PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY "${MAPNIK_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${MAPNIK_OUTPUT_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${MAPNIK_OUTPUT_DIR}/lib"
)
target_link_libraries(agg_rasterizer_integer_overflow_test PUBLIC
    Catch2::Catch2 
    mapnik::mapnik 
    mapnik::agg
    mapnik::json
)
target_include_directories(agg_rasterizer_integer_overflow_test PRIVATE "${catch2_SOURCE_DIR}/single_include/catch2" ${CMAKE_CURRENT_BINARY_DIR})

add_executable(datasource_registration_test standalone/datasource_registration_test.cpp)
set_target_properties(datasource_registration_test PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY "${MAPNIK_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${MAPNIK_OUTPUT_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${MAPNIK_OUTPUT_DIR}/lib"
)
target_link_libraries(datasource_registration_test PUBLIC
    Catch2::Catch2
    mapnik::mapnik 
    mapnik::agg
)
target_include_directories(datasource_registration_test PRIVATE "${catch2_SOURCE_DIR}/single_include/catch2" ${CMAKE_CURRENT_BINARY_DIR})

add_executable(font_registration_test standalone/font_registration_test.cpp)
set_target_properties(font_registration_test PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY "${MAPNIK_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${MAPNIK_OUTPUT_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${MAPNIK_OUTPUT_DIR}/lib"
)
target_link_libraries(font_registration_test PUBLIC
    Catch2::Catch2 
    mapnik::mapnik 
    mapnik::agg
    mapnik::json
)
target_include_directories(font_registration_test PRIVATE "${catch2_SOURCE_DIR}/single_include/catch2" ${CMAKE_CURRENT_BINARY_DIR})

#not workable since boost::filesystem native returns a wstring and the function taskes a std::string
add_executable(map_xml_test standalone/map_xml_test.cpp)
set_target_properties(map_xml_test PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY "${MAPNIK_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${MAPNIK_OUTPUT_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${MAPNIK_OUTPUT_DIR}/lib"
)
target_link_libraries(map_xml_test PUBLIC
    Catch2::Catch2 
    mapnik::mapnik 
    mapnik::agg
    mapnik::json
)
target_include_directories(map_xml_test PRIVATE "${catch2_SOURCE_DIR}/single_include/catch2" ${CMAKE_CURRENT_BINARY_DIR})

add_executable(mapnik-test-visual 
    visual/parse_map_sizes.cpp
    visual/report.cpp
    visual/runner.cpp
    visual/run.cpp
)
set_target_properties(mapnik-test-visual PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY "${MAPNIK_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${MAPNIK_OUTPUT_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${MAPNIK_OUTPUT_DIR}/lib"
)
target_link_libraries(mapnik-test-visual PRIVATE
    Catch2::Catch2
    Boost::program_options
    Boost::filesystem
    mapnik::mapnik
    mapnik::agg
)
target_include_directories(mapnik-test-visual PRIVATE "${catch2_SOURCE_DIR}/single_include/catch2" ${CMAKE_CURRENT_BINARY_DIR})

include("${catch2_SOURCE_DIR}/contrib/Catch.cmake")
include("${catch2_SOURCE_DIR}/contrib/ParseAndAddCatchTests.cmake")

file(COPY data DESTINATION "${MAPNIK_OUTPUT_DIR}/test")
file(COPY data-visual DESTINATION "${MAPNIK_OUTPUT_DIR}/test")
file(COPY unit/data DESTINATION "${MAPNIK_OUTPUT_DIR}/test/unit")
file(COPY "${mapnik_SOURCE_DIR}/demo/data" DESTINATION "${MAPNIK_OUTPUT_DIR}/demo")
add_dependencies(mapnik-test-unit mapnik-index shapeindex)

if(WIN32)
    set(m_test_path "\$<JOIN:\$<SHELL_PATH:$<TARGET_FILE_DIR:mapnik-index>;$<TARGET_FILE_DIR:shapeindex>;$ENV{PATH}>,\\\\\\\\\;>")
else()
    set(m_test_path "$<SHELL_PATH:$<TARGET_FILE_DIR:shapeindex>;$<TARGET_FILE_DIR:mapnik-index>;$ENV{PATH}>")
endif()

catch_discover_tests(mapnik-test-unit
    WORKING_DIRECTORY "${MAPNIK_OUTPUT_DIR}"
    PROPERTIES 
        ENVIRONMENT 
            "PATH=${m_test_path}"
)
catch_discover_tests(agg_rasterizer_integer_overflow_test WORKING_DIRECTORY "${MAPNIK_OUTPUT_DIR}")
catch_discover_tests(datasource_registration_test WORKING_DIRECTORY "${MAPNIK_OUTPUT_DIR}")
catch_discover_tests(font_registration_test WORKING_DIRECTORY "${MAPNIK_OUTPUT_DIR}")
catch_discover_tests(map_xml_test WORKING_DIRECTORY "${MAPNIK_OUTPUT_DIR}")
