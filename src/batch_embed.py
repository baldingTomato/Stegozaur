import os
import sys
from pathlib import Path

try:
    from hybrid import hide_audio_in_image
except ImportError:
    print("BŁĄD: Plik 'hybrid.py' musi znajdować się w tym samym katalogu co skrypt!")
    sys.exit(1)

def run_batch_embedding(images_dir, audio_path, output_dir):
    if not os.path.exists(audio_path):
        print(f"BŁĄD: Plik audio '{audio_path}' nie istnieje!")
        sys.exit(1)

    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
        print(f"[*] Utworzono katalog wyjściowy: {output_dir}")

    print(f"[*] Wczytywanie pliku audio: {audio_path}")
    with open(audio_path, 'rb') as f:
        audio_bytes = f.read()

    audio_name = Path(audio_path).stem
    print(f"[*] Nazwa audio: {audio_name}")

    png_files = [f for f in os.listdir(images_dir) if f.lower().endswith('.png')]
    png_files.sort()
    total = len(png_files)
    if total == 0:
        print(f"BŁĄD: Brak PNG w '{images_dir}'")
        sys.exit(1)

    print(f"[*] Znaleziono {total} obrazów.")
    print("-" * 50)

    success = 0
    for idx, img_name in enumerate(png_files, 1):
        cover_path = os.path.join(images_dir, img_name)
        img_base = Path(img_name).stem
        output_name = f"{img_base}_5_{audio_name}_HYBRID.png"
        stego_path = os.path.join(output_dir, output_name)

        print(f"\n[{idx}/{total}] {img_name} -> {output_name}")
        try:
            hide_audio_in_image(cover_path, audio_bytes, stego_path)
            success += 1
        except Exception as e:
            print(f"[BŁĄD] {img_name}: {e}")

    print("-" * 50)
    print(f"[+] Sukces: {success}/{total}")

if __name__ == '__main__':
    if len(sys.argv) != 4:
        print("Użycie: python3 batch_embed.py <katalog_z_png> <audio.wav> <katalog_wyjsciowy>")
        sys.exit(1)
    run_batch_embedding(sys.argv[1], sys.argv[2], sys.argv[3])