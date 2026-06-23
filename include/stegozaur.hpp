#pragma once
#include <memory>
#include <string>
#include <vector>
#include "algorithm.hpp"

class Stegozaur {
private:
    std::vector<std::unique_ptr<Algorithm>> algorithms_;

    void showMainMenu();
    int getUserChoice(int min, int max);
    std::string getUserInput(const std::string& prompt);

    void handleHideMode();
    void handleExtractMode();
    void handleBatchHideMode();

    void registerAlgorithms();

public:
    Stegozaur();
    ~Stegozaur() = default;

    void run();
};
