#include <iostream>
#include "../include/stegozaur.hpp"

int main() {
    try {
        Stegozaur app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Krytyczny blad: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
