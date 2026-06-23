import os
import glob
import cv2
import pandas as pd
from tqdm import tqdm
from skimage.metrics import peak_signal_noise_ratio as psnr_metric
from skimage.metrics import structural_similarity as ssim_metric


PATH_COVER_DIR = "/mnt/c/Users/tomek/Desktop/Stegozaur/cover"

# kluczem jest pełna ścieżka do folderu, 
# a wartościami są (Nazwa_Algorytmu, Kategoria_Audio)
FOLDERS_TO_PROCESS = {
    "/mnt/c/Users/tomek/Desktop/2sekundy_SUNIWARD": ("S-UNIWARD", "2.0s"),
    "/mnt/c/Users/tomek/Desktop/5sekund_SUNIWARD": ("S-UNIWARD", "5.0s"),
}

results = []

print("Rozpoczynam obliczanie metryk jakości obrazu...")

for folder, (algo_name, category) in FOLDERS_TO_PROCESS.items():
    if not os.path.exists(folder):
        print(f"[OSTRZEŻENIE] Folder nie istnieje, pomijam: {folder}")
        continue
        
    print(f"\nPrzetwarzanie: {algo_name} ({category}) z folderu: {folder}")
    
    # Szukam wszystkich plików PNG w danym folderze stego
    stego_files = glob.glob(os.path.join(folder, "*.png"))
    
    for stego_path in tqdm(stego_files):
        filename = os.path.basename(stego_path)
        
        # Pobieram pierwsze 4 znaki, np. "0008"
        cover_id = filename[:4]
        
        if not cover_id.isdigit():
            continue
            
        cover_path = os.path.join(PATH_COVER_DIR, f"{cover_id}.png")
        
        if not os.path.exists(cover_path):
            cover_path = os.path.join(PATH_COVER_DIR, f"{cover_id}.jpg")
            if not os.path.exists(cover_path):
                continue

        img_cover = cv2.imread(cover_path)
        img_stego = cv2.imread(stego_path)
        
        if img_cover is None or img_stego is None:
            continue
            
        if img_cover.shape != img_stego.shape:
            continue

        try:
            # Obliczanie PSNR
            psnr_val = psnr_metric(img_cover, img_stego)
            
            # Obliczanie SSIM dla obrazu kolorowego (RGB/BGR)
            # channel_axis=2 informuje bibliotekę, że ostatni wymiar macierzy to kanały kolorów
            ssim_val = ssim_metric(img_cover, img_stego, channel_axis=2)
            
            results.append({
                "Plik": filename,
                "Algorytm": algo_name,
                "Kategoria": category,
                "PSNR": psnr_val,
                "SSIM": ssim_val
            })
        except Exception as e:
            print(f"Błąd przetwarzania pliku {filename}: {e}")

df = pd.DataFrame(results)
df.to_csv("szczegolowe_wyniki_metryk.csv", index=False, encoding='utf-8')
print("\n[+] Zapisano szczegółowe wyniki dla każdego obrazu do 'szczegolowe_wyniki_metryk.csv'")

print("\n" + "="*50)
print(" PODSUMOWANIE STATYSTYCZNE DO LATEXA ")
print("="*50)

# Wyznaczenie średniej i odchylenia standardowego
summary = df.groupby(["Algorytm", "Kategoria"]).agg({
    "PSNR": ["mean", "std"],
    "SSIM": ["mean", "std"]
}).reset_index()

# Czyszczenie nazw kolumn po agregacji
summary.columns = ["Algorytm", "Kategoria", "PSNR_Średnia", "PSNR_Odchylenie", "SSIM_Średnia", "SSIM_Odchylenie"]

print(summary.to_string(index=False))

summary.to_csv("podsumowanie_dla_tabel.csv", index=False, encoding='utf-8')