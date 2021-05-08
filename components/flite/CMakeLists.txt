file(GLOB_RECURSE sources ./**/*.cpp ./**/*.c)

idf_component_register(SRCS "${sources}" INCLUDE_DIRS "include")
