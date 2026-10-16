#include <iostream>
#include "Plugin/build/_deps/juce-src/modules/juce_gui_basics/juce_gui_basics.h"
int main() {
    std::cout << juce::KeyPress::rightKey << std::endl;
    return 0;
}
