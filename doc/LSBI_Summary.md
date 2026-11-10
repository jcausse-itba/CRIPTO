# Summary: Improved LSB Image Steganography Using Bit-Inverse in 24-Bit Colour Images

**Paper Reference:** Majeed, M. A., & Sulaiman, R. (2015). *An Improved LSB Image Steganography Technique Using Bit-Inverse in 24 Bit Colour Image*. Journal of Theoretical and Applied Information Technology, 80(2), 342–348.

---

## 1. Executive Summary (TL;DR)

Standard Least Significant Bit (LSB) steganography modifies the lowest-order bit of cover image pixels, but alters roughly 50% of targeted bits on average, degrading image fidelity (PSNR). 

This paper introduces two core improvements for 24-bit RGB images:
1. **Selective Channel Use:** Embedding is restricted to the **Green** and **Blue** channels only. The **Red** channel is left untouched to serve as decoy/noise data, increasing attacker complexity.
2. **Pattern-Based Bit Inversion:** Cover bytes are categorized into 4 buckets according to their 2nd and 3rd least significant bits (`00`, `01`, `10`, `11`). If standard LSB substitution modified more than half the bytes within a bucket, all LSBs in that bucket are inverted. This guarantees that fewer than 50% of the bits in that group differ from the original image, significantly boosting the Peak Signal-to-Noise Ratio (PSNR).

---

## 2. Core Idea & Mathematical Intuition

For a group of pixels classified under a specific pattern $P \in \{00, 01, 10, 11\}$:
* Let $N_{\text{total}}$ be the number of pixels in pattern $P$.
* Let $N_{\text{changed}}$ be the count of pixels whose LSB had to be altered to match the secret bit.
* Let $N_{\text{unchanged}}$ be the count where the cover LSB already matched the secret bit ($N_{\text{unchanged}} = N_{\text{total}} - N_{\text{changed}}$).

If $N_{\text{changed}} > N_{\text{unchanged}}$:
* Invert the LSB of **all** pixels in group $P$.
* After inversion:
  $$\text{New Changed Count} = N_{\text{total}} - N_{\text{changed}} = N_{\text{unchanged}} < \frac{N_{\text{total}}}{2}$$
* This guarantees that at most 50% of the pixels in each pattern bucket are modified relative to the cover image.

---

## 3. The Algorithm Step-by-Step

### Phase 1: Embedding & Inversion (Sender)

1. **Channel Selection:**
   * Read the 24-bit RGB cover image.
   * Target only the **Green** and **Blue** color bytes for message hiding (Red is skipped).

2. **Classification (Pattern Mapping):**
   * Inspect the **2nd and 3rd LSBs** (bits 1 and 2, assuming bit 0 is LSB) of each target byte.
   * Group each byte into one of four pattern categories:
     - Pattern `00`
     - Pattern `01`
     - Pattern `10`
     - Pattern `11`

3. **Standard LSB Embedding:**
   * Sequentially replace bit 0 (the LSB) of each target byte with the corresponding secret message bit.

4. **Change Evaluation:**
   * For each of the 4 pattern buckets:
     * Count how many bytes were modified ($N_{\text{changed}}$) vs. unmodified ($N_{\text{unchanged}}$) relative to the original cover image.

5. **Conditional Inversion:**
   * For each pattern bucket where $N_{\text{changed}} > N_{\text{unchanged}}$:
     * Invert bit 0 ($0 \rightarrow 1$ and $1 \rightarrow 0$) for every byte in that pattern.
     * Record a `1` in a 4-bit inversion map corresponding to this pattern.
   * If $N_{\text{changed}} \le N_{\text{unchanged}}$:
     * Leave the LSBs as they are.
     * Record a `0` in the 4-bit inversion map.

6. **Metadata Storage:**
   * Store the 4-bit status map (indicating which patterns were inverted) in a predetermined reserved location in the stego-image.

---

### Phase 2: Extraction (Receiver)

1. **Read Metadata:**
   * Extract the 4-bit inversion map from the reserved location to determine which patterns were inverted.

2. **Pixel Classification:**
   * Scan the Green and Blue channels of the stego-image.
   * Check the 2nd and 3rd LSBs of each byte to classify it into its pattern (`00`, `01`, `10`, or `11`).

3. **Re-Inversion:**
   * If a byte belongs to a pattern marked as inverted in the status map, invert its LSB ($0 \leftrightarrow 1$) to restore the intended payload bit.
   * If not marked as inverted, leave its LSB untouched.

4. **Message Reconstruction:**
   * Read the LSBs sequentially from the Green and Blue channels to assemble the original secret bitstream.

---

## 4. Performance & Results Reported in the Paper

| Cover Image | Standard LSB / Prior Art PSNR (dB) | Proposed Method PSNR (dB) |
| :--- | :--- | :--- |
| **Lena.jpg** | ~49.2 – 53.7 dB | **72.48 dB** |
| **Baboon.jpg** | ~48.8 – 53.7 dB | **60.51 dB** |
| **Peppers.jpg**| ~47.9 – 53.7 dB | **69.77 dB** |

A higher PSNR indicates less perceptual and mathematical difference between the cover image and stego-image, making visual and statistical detection much more difficult.