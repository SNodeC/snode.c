#include "KeyboardReader.h"
#include "TowerCalculator.h"
#include "core/SNodeC.h"

#include <functional>

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    TowerCalculator towerCalculator;

    towerCalculator.calculate(25);
    towerCalculator.calculate(100);

    KeyboardReader keyboardReader([&towerCalculator](long startValue) -> void {
        towerCalculator.calculate(startValue);
    });

    return core::SNodeC::start();
}
