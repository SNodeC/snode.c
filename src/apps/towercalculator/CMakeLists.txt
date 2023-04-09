cmake_minimum_required(VERSION 3.0)

set(TOWERCALCULATION_CPP towerCalculation.cpp KeyboardReader.cpp
                         TowerCalculator.cpp
)

set(TOWERCALCULATION_H KeyboardReader.h TowerCalculator.h)

add_executable(towerCalculation ${TOWERCALCULATION_CPP} ${TOWERCALCULATION_H})

target_link_libraries(towerCalculation PRIVATE snodec::core)

install(TARGETS towerCalculation RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                         COMPONENT apps
)
