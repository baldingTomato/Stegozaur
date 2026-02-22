#include <iostream>
#include <string>
#include <limits>

int main() {
    std::cout << "=== Stegozaur - steganografia audio w obrazach ===\n\n";

    int mode = 0;
    while (true) {
        std::cout << "1. Ukryj audio w obrazie\n";
        std::cout << "2. Wydobadz audio z obrazu\n";
        std::cout << "3. Wyjscie\n";
        std::cout << "?: ";

        std::cin >> mode;
        if (std::cin.fail() || mode < 1 || mode > 3) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Sprobuj ponownie.\n\n";
            continue;
        }
        break;
    }

    if (mode == 3) {
        std::cout << "Adieu!\n";
        return 0;
    }

    std::string inputImage, inputAudio, outputImage, outputAudio;

    if (mode == 1) { // hide
        std::cout << "\n--- Ukrywanie ---\n";
        std::cout << "Podaj nazwe obrazu wejsciowego (PNG): ";
        std::cin >> inputImage;
        std::cout << "Podaj nazwe audio wejsciowego (WAV): ";
        std::cin >> inputAudio;
        std::cout << "Podaj nazwe dla obrazu wyjsciowego (PNG): ";
        std::cin >> outputImage;

        std::cout << "\n" << inputImage << " + " << inputAudio << " -> " << outputImage << "\n";

        int algo = 0;
        std::cout << "\nWybierz algorytm:\n";
        std::cout << "1. LSB\n";
        std::cout << "2. (w przygotowaniu)\n";
        std::cout << "3. (w przygotowaniu)\n";
        std::cout << "?: ";
        std::cin >> algo;
        // algorytm()
    } else { // extract
        std::cout << "\n--- Wydobywanie ---\n";
        std::cout << "Podaj nazwe obrazu z ukrytym audio (PNG): ";
        std::cin >> inputImage;
        std::cout << "Podaj nazwe dla pliku audio wyjsciowego (WAV): ";
        std::cin >> outputAudio;

        std::cout << "\n" << inputImage << " -> " << outputAudio << "\n";
        // ekstrakcja()
    }

    std::cout << "\nOperacja zakonczona.\n";
    return 0;
}