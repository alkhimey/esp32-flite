file(GLOB_RECURSE sources ./**.cpp ./**.c)
list(FILTER sources EXCLUDE REGEX "lex_data_include*")

idf_component_register(SRCS "${sources}" 
                       INCLUDE_DIRS "include" "lex_data_include"
                       REQUIRES flite)
