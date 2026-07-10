cmake_minimum_required(VERSION 3.18)

# snodec-control-core: the target-process-agnostic parsing/model/rendering/action logic, kept as a
# separate static library so the unit tests (and the UI logic library below) can link against it without
# needing curses. ProcessRunner lives here too since ConfigActions (discovery/save) needs it, and nothing
# about spawning a subprocess is curses- or UI-specific.
add_library(
    snodec-control-core STATIC
    CommandBuilder.cpp
    CommandBuilder.h
    ConfigActions.cpp
    ConfigActions.h
    ConfigEditor.cpp
    ConfigEditor.h
    ConfigModel.cpp
    ConfigModel.h
    ConfigParser.cpp
    ConfigParser.h
    Json.cpp
    Json.h
    JsonWriter.cpp
    JsonWriter.h
    Materializer.cpp
    Materializer.h
    Metadata.cpp
    Metadata.h
    ProcessRunner.cpp
    ProcessRunner.h
)
target_include_directories(
    snodec-control-core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
)

# snodec-control-ui-logic: the hierarchical tree derivation and interactive session state (UiTree/
# UiState) behind the --ui feature. Deliberately curses-free so it is unit-testable without a real
# terminal and always builds, even when SNODEC_CONTROL_BUILD_TUI is off or Curses is missing.
add_library(
    snodec-control-ui-logic STATIC
    ui/UiTree.cpp
    ui/UiTree.h
    ui/UiState.cpp
    ui/UiState.h
    ui/LineEditor.cpp
    ui/LineEditor.h
    ui/RenderUtil.cpp
    ui/RenderUtil.h
)
target_include_directories(
    snodec-control-ui-logic PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(snodec-control-ui-logic PUBLIC snodec-control-core)

# snodec-control-ui: the public Ui.h entry point (isUiAvailable()/runInteractiveUi()), backed by exactly
# one of two mutually exclusive implementations depending on Curses availability, so the rest of the
# program never needs to know which one it got.
if(SNODEC_CONTROL_BUILD_TUI)
    find_package(Curses)
endif()

if(SNODEC_CONTROL_BUILD_TUI AND CURSES_FOUND)
    message(STATUS "snodec-control: building with interactive UI support (--ui), Curses found")
    set(SNODEC_CONTROL_UI_SOURCES ui/CursesUi.cpp)
else()
    if(SNODEC_CONTROL_BUILD_TUI)
        message(STATUS "snodec-control: Curses not found; --ui will fail clearly at runtime, everything else still works")
    else()
        message(STATUS "snodec-control: SNODEC_CONTROL_BUILD_TUI is off; --ui will fail clearly at runtime")
    endif()
    set(SNODEC_CONTROL_UI_SOURCES ui/UiUnavailable.cpp)
endif()

add_library(snodec-control-ui STATIC ${SNODEC_CONTROL_UI_SOURCES} ui/Ui.h)
target_include_directories(
    snodec-control-ui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(snodec-control-ui PUBLIC snodec-control-ui-logic)

if(SNODEC_CONTROL_BUILD_TUI AND CURSES_FOUND)
    target_include_directories(snodec-control-ui PRIVATE ${CURSES_INCLUDE_DIRS})
    target_link_libraries(snodec-control-ui PRIVATE ${CURSES_LIBRARIES})
endif()

add_executable(
    snodec-control
    main.cpp
    Cli.cpp
    Cli.h
    ArgTokenizer.cpp
    ArgTokenizer.h
)
target_link_libraries(snodec-control PRIVATE snodec-control-core snodec-control-ui)

install(TARGETS snodec-control RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
