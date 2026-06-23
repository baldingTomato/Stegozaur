#include "../include/stegozaur.hpp"
#include <iostream>
#include <limits>
#include <algorithm>
#include <cstring>
#include <windows.h>
#include "../include/audio.hpp"
#include "../include/image.hpp"
#include "../include/lsb_algorithm.hpp"
#include "../include/lsb_matching_algorithm.hpp"
#include "../include/pvd_algorithm.hpp"

static bool fileExists(const std::string& path) {
    DWORD attrib = GetFileAttributesA(path.c_str());
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

static bool createDirectory(const std::string& path) {
    if (CreateDirectoryA(path.c_str(), nullptr)) return true;
    return (GetLastError() == ERROR_ALREADY_EXISTS);
}

// Zbiera wszystkie pliki .png z podanego katalogu
static std::vector<std::string> getPngFiles(const std::string& dir) {
    std::vector<std::string> result;
    std::string searchPattern = dir;
    if (!searchPattern.empty() && searchPattern.back() != '/' && searchPattern.back() != '\\')
        searchPattern += "\\";
    searchPattern += "*.png";

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPattern.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return result;

    do {
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::string fullPath = dir;
            if (!fullPath.empty() && fullPath.back() != '/' && fullPath.back() != '\\')
                fullPath += "\\";
            fullPath += findData.cFileName;
            result.push_back(fullPath);
        }
    } while (FindNextFileA(hFind, &findData) != 0);
    FindClose(hFind);
    return result;
}

// Wyciąga nazwę pliku bez ścieżki i rozszerzenia
static std::string getBaseName(const std::string& path) {
    size_t lastSlash = path.find_last_of("/\\");
    std::string filename = (lastSlash == std::string::npos) ? path : path.substr(lastSlash + 1);
    size_t lastDot = filename.find_last_of('.');
    if (lastDot != std::string::npos) filename = filename.substr(0, lastDot);
    return filename;
}

Stegozaur::Stegozaur() {
    registerAlgorithms();
}

void Stegozaur::registerAlgorithms() {
    algorithms_.push_back(std::make_unique<LSBAlgorithm>(1));
    algorithms_.push_back(std::make_unique<LSBMatchingAlgorithm>(1));
    algorithms_.push_back(std::make_unique<PVDAlgorithm>());
    // algorithms_.push_back(std::make_unique<DCTAlgorithm>());
}

void Stegozaur::run() {
    std::cout << "=== Stegozaur - steganografia audio w obrazach ===\n\n";

    while (true) {
        showMainMenu();

        int choice = getUserChoice(1, 5);

        if (choice == 1) {
            handleHideMode();
        } else if (choice == 2) {
            handleBatchHideMode();
        } else if (choice == 3) {
            handleExtractMode();
        } else if (choice == 4) {
            std::cout << "Adieu!\n";
            break;
        }
    }
}

void Stegozaur::showMainMenu() {
    std::cout << "  1. Ukryj audio w obrazie\n"
              << "  2. Ukryj audio we wszystkich obrazach z katalogu\n"
              << "  3. Wydobadz audio z obrazu\n"
              << "  4. Wyjscie\n"
              << "?: ";
}

int Stegozaur::getUserChoice(int min, int max) {
    int choice;
    while (true) {
        std::cin >> choice;

        if (std::cin.fail() || choice < min || choice > max) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Nieprawidlowy wybor. Podaj liczbe od "
                      << min << " do " << max << ": ";
        } else {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return choice;
        }
    }
}

std::string Stegozaur::getUserInput(const std::string& prompt) {
    std::cout << prompt;
    std::string input;
    std::getline(std::cin, input);
    return input;
}

void Stegozaur::handleHideMode() {
    std::cout << "\n--- TRYB UKRYWANIA ---\n";

    try {
        auto imageName = getUserInput("Nazwa do obrazu PNG: ");
        auto audioName = getUserInput("Nazwa do pliku WAV: ");
        auto outputName = getUserInput("Nazwa obrazu wynikowego: ");

        std::string imagePath = "../pics/" + imageName;
        std::string audioPath = "../sounds/" + audioName;
        std::string outputPath = "../stegopics/" + outputName;

        std::cout << "Wczytywanie obrazu...\n";
        Image coverImage(imagePath);

        std::cout << "Wczytywanie audio...\n";
        Audio secretAudio(audioPath);

        std::cout << "\n--- INFORMACJE O PLIKACH ---\n";
        std::cout << "Obraz: " << coverImage.getWidth() << "x" << coverImage.getHeight()
                  << ", " << coverImage.getChannels() << " kanaly\n";

        auto& audioInfo = secretAudio.getInfo();
        std::cout << "Audio: " << audioInfo.totalFrames << " ramek, "
                  << audioInfo.channels << " kanalow, "
                  << audioInfo.sampleRate << " Hz\n";

        std::cout << "\n--- DOSTEPNE ALGORYTMY ---\n";
        for (size_t i = 0; i < algorithms_.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << algorithms_[i]->getName() << "\n";
        }

        std::cout << "?: ";
        int algoChoice = getUserChoice(1, static_cast<int>(algorithms_.size()));

        Image stegoImage(coverImage.getWidth(),
                         coverImage.getHeight(),
                         coverImage.getChannels());

        std::cout << "Ukrywanie danych...\n";
        algorithms_[static_cast<size_t>(algoChoice - 1)]->hide(coverImage, secretAudio, stegoImage);

        std::cout << "Zapisywanie obrazu...\n";
        stegoImage.save(outputPath);

        std::cout << "SUKCES! Audio ukryte w: " << outputPath << "\n\n";

    } catch (const std::exception& e) {
        std::cerr << "BLAD: " << e.what() << "\n\n";
    }
}

void Stegozaur::handleBatchHideMode() {
    std::cout << "\n--- TRYB UKRYWANIA (wsadowy) ---\n";

    try {
        auto filename = getUserInput("Nazwa pliku WAV: ");
        std::string audioPath = "../payload/08/" + filename;

        if (!fileExists(audioPath)) {
            throw std::runtime_error("Plik audio nie istnieje: " + audioPath);
        }

        // Przygotuj nazwę bazową dla plików wynikowych
        std::string audioBase = [&]() {
            std::string base;
            // Wyciągnij nazwę pliku bez rozszerzenia
            size_t dotPos = filename.find_last_of('.');
            if (dotPos != std::string::npos) filename = filename.substr(0, dotPos);
            base = filename;

            // Dodaj przedrostek z katalogu (np. payload_2)
            base =  "08_" + base;
            return base;
        }();

        std::cout << "Wczytywanie audio z: " << audioPath << "\n";
        Audio secretAudio(audioPath);
        auto& audioInfo = secretAudio.getInfo();
        std::cout << "Audio: " << audioInfo.totalFrames << " ramek, "
                  << audioInfo.channels << " kanalow, "
                  << audioInfo.sampleRate << " Hz\n";

        std::cout << "\n--- DOSTEPNE ALGORYTMY ---\n";
        for (size_t i = 0; i < algorithms_.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << algorithms_[i]->getName() << "\n";
        }
        std::cout << "?: ";
        int algoChoice = getUserChoice(1, static_cast<int>(algorithms_.size()));
        auto& algorithm = algorithms_[static_cast<size_t>(algoChoice - 1)];

        std::string algoShort;
        std::string algoFull = algorithm->getName();
        if (algoFull.find("LSB") != std::string::npos) {
            if (algoFull.find("m") != std::string::npos)
                algoShort = "LSBM";
            else
                algoShort = "LSB";
        } else if (algoFull.find("PVD") != std::string::npos) {
            algoShort = "PVD";
        } else {
            algoShort = "UNKNOWN";
        }

        // Katalog źródłowy obrazów
        const std::string picsDir = "../cover";
        std::vector<std::string> imageFiles = getPngFiles(picsDir);
        if (imageFiles.empty()) {
            throw std::runtime_error("Brak plików PNG w katalogu ../cover/");
        }

        // Katalog docelowy
        const std::string stegoDir = "../stegofiles";
        createDirectory(stegoDir);

        int successCount = 0;
        int totalCount = static_cast<int>(imageFiles.size());

        for (const auto& imageFullPath : imageFiles) {
            std::string imageName = getBaseName(imageFullPath);
            std::string outputName = imageName + "_" + audioBase + "_" + algoShort + ".png";
            std::string outputPath = stegoDir + "/" + outputName;

            std::cout << "\nPrzetwarzanie: " << imageFullPath << " -> " << outputName << std::endl;

            try {
                Image coverImage(imageFullPath);
                Image stegoImage(coverImage.getWidth(), coverImage.getHeight(), coverImage.getChannels());
                algorithm->hide(coverImage, secretAudio, stegoImage);
                stegoImage.save(outputPath);
                std::cout << "  OK" << std::endl;
                successCount++;
            } catch (const std::exception& e) {
                std::cerr << "  BLAD: " << e.what() << std::endl;
            }
        }

        std::cout << "\nZakonczono. Sukces: " << successCount << " / " << totalCount << "\n\n";

    } catch (const std::exception& e) {
        std::cerr << "BLAD: " << e.what() << "\n\n";
    }
}

void Stegozaur::handleExtractMode() {
    std::cout << "\n--- TRYB WYDOBYWANIA ---\n";

    try {
        auto imageName = getUserInput("Nazwa obrazu z ukrytym audio: ");
        auto outputName = getUserInput("Nazwa pliku audio wynikowego: ");

        std::string imagePath = "../stegopics/" + imageName;
        std::string outputPath = "../extracted/" + outputName;

        std::cout << "Wczytywanie obrazu...\n";
        Image stegoImage(imagePath);

        std::cout << "\n--- DOSTEPNE ALGORYTMY ---\n";
        for (size_t i = 0; i < algorithms_.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << algorithms_[i]->getName() << "\n";
        }

        std::cout << "Wybierz algorytm: ";
        int algoChoice = getUserChoice(1, static_cast<int>(algorithms_.size()));

        Audio extractedAudio;
        std::cout << "Wydobywanie danych...\n";
        algorithms_[static_cast<size_t>(algoChoice - 1)]->extract(stegoImage, extractedAudio);

        std::cout << "Zapisywanie audio...\n";
        extractedAudio.save(outputPath);

        std::cout << "SUKCES! Audio wydobyte do: " << outputPath << "\n\n";

    } catch (const std::exception& e) {
        std::cerr << "BLAD: " << e.what() << "\n\n";
    }
}
