add_custom_target(format DEPENDS format-cmds)
add_custom_command(
    OUTPUT format-cmds
    COMMENT "Auto formatting of all source and all cmake files"
)

include(clang-format)
include(cmake-format)
