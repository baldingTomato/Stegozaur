import os
import sys
import numpy as np
from PIL import Image

# --- SEKCJA 1: TRANSFORMATY MATEMATYCZNE (IWT/IIWT) ---

def forward_iwt_1d(array):
    """Całkowitoliczbowa transformata falkowa 1D (Haar Lifting Scheme)"""
    even = array[0::2].astype(np.int32)
    odd = array[1::2].astype(np.int32)
    d = odd - even
    s = even + (d >> 1)
    return np.concatenate([s, d])

def inverse_iwt_1d(array):
    """Odwrotna całkowitoliczbowa transformata 1D"""
    half = len(array) // 2
    s = array[:half]
    d = array[half:]
    even = s - (d >> 1)
    odd = d + even
    res = np.empty_like(array)
    res[0::2] = even
    res[1::2] = odd
    return res

def forward_iwt_2d(block):
    """Transformata IWT 2D dla bloku 8x8"""
    row_transformed = np.array([forward_iwt_1d(row) for row in block])
    col_transformed = np.array([forward_iwt_1d(row_transformed[:, i]) for i in range(8)]).T
    return col_transformed

def inverse_iwt_2d(block):
    """Odwrotna transformata IWT 2D dla bloku 8x8"""
    col_inverted = np.array([inverse_iwt_1d(block[:, i]) for i in range(8)]).T
    row_inverted = np.array([inverse_iwt_1d(row) for row in col_inverted])
    return np.clip(row_inverted, 0, 255).astype(np.uint8)


# --- SEKCJA 2: SFLA - SZUKANIE OPTYMALNYCH BLOKÓW ---

def calculate_block_complexity(block_iwt):
    """
    Obliczenie złożoności na podstawie podpasma LL (lewy górny kwadrat 4x4).
    """
    ll_subband = block_iwt[:4, :4]
    return float(np.std(ll_subband))

def run_sfla(blocks_iwt, total_bits_needed, num_frogs=30, iterations=10):
    """Deterministyczny algorytm SFLA"""
    print(f"[SFLA] Start optymalizacji. Potrzeba bitów: {total_bits_needed}")
    
    # Stałe ziarno losowości
    np.random.seed(42)
    
    num_blocks = len(blocks_iwt)
    complexities = np.array([calculate_block_complexity(b) for b in blocks_iwt])
    median_comp = np.median(complexities)
    high_comp = np.percentile(complexities, 80)
    print(f"[SFLA] Obliczono złożoność dla {num_blocks} bloków.")
    
    frogs = []
    for _ in range(num_frogs):
        noise = np.random.uniform(0.8, 1.2, size=num_blocks)
        score = complexities * noise
        chosen_blocks = np.zeros(num_blocks, dtype=np.int32)
        sorted_indices = np.argsort(score)[::-1]
        
        bits_allocated = 0
        for idx in sorted_indices:
            if bits_allocated >= total_bits_needed:
                break
            
            if complexities[idx] > high_comp:
                bits = 32
            elif complexities[idx] > median_comp:
                bits = 16
            else:
                bits = 4
                
            bits = min(bits, total_bits_needed - bits_allocated)
            chosen_blocks[idx] = bits
            bits_allocated += bits
            
        frogs.append(chosen_blocks)

    best_frog = frogs[0]
    best_fitness = -1.0
    
    for frog in frogs:
        total_capacity = np.sum(frog)
        penalty = 0 if total_capacity >= total_bits_needed else -9999999
        fitness = np.sum(complexities[frog > 0]) + penalty
        
        if fitness > best_fitness:
            best_fitness = fitness
            best_frog = frog
                
    return best_frog


# --- SEKCJA 3: EMBEDDING I EKSTRAKCJA ---

def hide_audio_in_image(cover_path, audio_bytes, stego_path):
    print(f"[*] Rozmiar surowego ładunku audio: {len(audio_bytes)} bajtów")
    
    bits = []
    for byte in audio_bytes:
        for i in range(8):
            bits.append((byte >> (7 - i)) & 1)
            
    total_bits = len(bits)
    print(f"[*] Wymagana pojemność: {total_bits} bitów")
    
    img = Image.open(cover_path).convert('RGB')

    # zapobieganie overflow
    img_array = np.array(img).astype(np.int32)
    img_array = np.clip(img_array, 15, 240) 
    
    h, w, channels = img_array.shape
    blocks = []
    block_coords = []
    
    for c in range(3):
        for y in range(0, h - 7, 8):
            for x in range(0, w - 7, 8):
                blocks.append(img_array[y:y+8, x:x+8, c])
                block_coords.append((y, x, c))
                
    print(f"[*] Liczba dostępnych bloków (RGB): {len(blocks)}")
    
    blocks_iwt = [forward_iwt_2d(b) for b in blocks]
    embedding_map = run_sfla(blocks_iwt, total_bits)
    
    max_capacity = np.sum(embedding_map)
    if max_capacity < total_bits:
        print(f"BŁĄD: Mimo zwiększenia pojemności brakuje {total_bits - max_capacity} bitów!")
        sys.exit(1)
        
    # Definicja 48 bezpiecznych indeksów w spłaszczonej macierzy 8x8 (pomijam lewy górny kwadrat 4x4)
    safe_indices = [r * 8 + c for r in range(8) for c in range(8) if not (r < 4 and c < 4)]
    
    header_bits = [(total_bits >> i) & 1 for i in range(31, -1, -1)]
    flat_iwt_0 = blocks_iwt[0].flatten()
    for i in range(32):
        coeff_idx = safe_indices[i]
        flat_iwt_0[coeff_idx] = (flat_iwt_0[coeff_idx] & ~1) | header_bits[i]
    blocks_iwt[0] = flat_iwt_0.reshape(8, 8)
    
    bit_idx = 0
    for block_idx in range(1, len(blocks_iwt)):
        bits_to_hide = embedding_map[block_idx]
        if bits_to_hide == 0 or bit_idx >= total_bits:
            continue
            
        flat_block = blocks_iwt[block_idx].flatten()
        for k in range(bits_to_hide):
            if bit_idx < total_bits:
                coeff_idx = safe_indices[k]
                flat_block[coeff_idx] = (flat_block[coeff_idx] & ~1) | bits[bit_idx]
                bit_idx += 1
        blocks_iwt[block_idx] = flat_block.reshape(8, 8)
        
    for idx, (y, x, c) in enumerate(block_coords):
        img_array[y:y+8, x:x+8, c] = inverse_iwt_2d(blocks_iwt[idx])
        
    # --- Rzutowanie na 8-bitowe piksele przed zapisem ---
    img_array = img_array.astype(np.uint8)
    
    stego_img = Image.fromarray(img_array)
    stego_img.save(stego_path, 'PNG')
    print(f"[+] Sukces! Zapisano jako: {stego_path}")


def extract_audio_from_image(stego_path, output_audio_path):
    img = Image.open(stego_path).convert('RGB')
    img_array = np.array(img)
    
    h, w, channels = img_array.shape
    blocks_iwt = []
    
    for c in range(3):
        for y in range(0, h - 7, 8):
            for x in range(0, w - 7, 8):
                block = img_array[y:y+8, x:x+8, c]
                blocks_iwt.append(forward_iwt_2d(block))
                
    safe_indices = [r * 8 + c for r in range(8) for c in range(8) if not (r < 4 and c < 4)]
    
    flat_iwt_0 = blocks_iwt[0].flatten()
    total_bits = 0
    for i in range(32):
        coeff_idx = safe_indices[i]
        total_bits = (total_bits << 1) | (flat_iwt_0[coeff_idx] & 1)
        
    print(f"[*] Odczytano z nagłówka zapotrzebowanie na: {total_bits} bitów")
    
    embedding_map = run_sfla(blocks_iwt, total_bits)
    
    extracted_bits = []
    bit_idx = 0
    
    for block_idx in range(1, len(blocks_iwt)):
        bits_to_extract = embedding_map[block_idx]
        if bits_to_extract == 0 or bit_idx >= total_bits:
            continue
            
        flat_block = blocks_iwt[block_idx].flatten()
        for k in range(bits_to_extract):
            if bit_idx < total_bits:
                coeff_idx = safe_indices[k]
                extracted_bits.append(flat_block[coeff_idx] & 1)
                bit_idx += 1
                
    extracted_bytes = bytearray()
    for i in range(0, len(extracted_bits), 8):
        byte_bits = extracted_bits[i:i+8]
        if len(byte_bits) == 8:
            byte_val = 0
            for b in byte_bits:
                byte_val = (byte_val << 1) | b
            extracted_bytes.append(byte_val)
            
    with open(output_audio_path, 'wb') as f:
        f.write(extracted_bytes)
    print(f"[+] Sukces! Ekstrakcja czystego pliku audio do: {output_audio_path}")


# --- SEKCJA 4: INTERFEJS ---

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Użycie:")
        print("  Osadzanie: python3 hybrid.py embed <cover.png> <secret.wav> <stego.png>")
        print("  Ekstrakcja: python3 hybrid.py extract <stego.png> <output.wav>")
        sys.exit(1)
        
    mode = sys.argv[1]
    
    if mode == "embed":
        if len(sys.argv) != 5:
            print("Błąd: osadzanie wymaga 3 argumentów: cover.png secret.wav stego.png")
            sys.exit(1)
        cover = sys.argv[2]
        audio_in = sys.argv[3]
        stego_out = sys.argv[4]
        
        with open(audio_in, 'rb') as f:
            audio_data = f.read()
            
        hide_audio_in_image(cover, audio_data, stego_out)
        
    elif mode == "extract":
        if len(sys.argv) != 4:
            print("Błąd: ekstrakcja wymaga 2 argumentów: stego.png output.wav")
            sys.exit(1)
        stego_in = sys.argv[2]
        audio_out = sys.argv[3]
        
        extract_audio_from_image(stego_in, audio_out)
    else:
        print(f"Nieznany tryb: {mode}")
        sys.exit(1)