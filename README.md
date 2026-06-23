# Stegozaur
Aplikacja konsolowa do eksperymentów z algorytmami steganograficznymi ukrywającymi pliki .WAV w .PNG

## Funkcje

- Implementacja klasycznych algorytmów steganografii przestrzennej:
  - **LSB** (Least Significant Bit)
  - **LSBM** (Least Significant Bit Matching)
  - **PVD** (Pixel-Value Differencing)
- Obsługa obrazów w formacie **PNG** oraz plików audio **WAV** (PCM)
- Tryb wsadowy – ukrywanie tego samego audio we wszystkich obrazach z katalogu
- Własny protokół nagłówka (128 bitów metadanych) umożliwiający autonomiczną ekstrakcję
- Zbudowany w C++17

## Wymagania

- Kompilator z obsługą C++17:
  - **GCC** (MinGW na Windows, Linux) – testowane z wersją 8.1.0
- **CMake** w wersji 3.15 lub nowszej
- System operacyjny: Windows (z WSL lub natywny)

```bash
git clone https://github.com/baldingTomato/stegozaur.git
cd stegozaur
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
cmake --build . -j
./bin/Stegozaur.exe
```
