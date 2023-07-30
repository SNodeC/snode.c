# Custom target to generate a library dependency graph

add_custom_target(
    graphviz
    COMMAND ${CMAKE_COMMAND} "--graphviz=foo.dot" .
    COMMAND dot -Tpdf foo.dot -o ${CMAKE_SOURCE_DIR}/SNodeC-Lib-Dependencies.pdf
    COMMAND rm foo.dot.*
    COMMAND rm foo.dot
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)
