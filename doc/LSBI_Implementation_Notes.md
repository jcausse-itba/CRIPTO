# Documento de requerimientos técnicos y decisiones de diseño acordadas con la cátedra

---

## 1. Conclusiones y Decisiones de Diseño

### 1. Manejo de Padding en BMP
* **Decisión:** Para **LSBI**, asumir que no se procesan imágenes portadoras con padding (las imágenes de prueba de la cátedra cumplen que `(width * 3) % 4 == 0`). Si la imagen tuviera padding, al utilizar LSBI (solamente ese algoritmo), el _logger_ deberá mostrar un mensaje de nivel `WARN`.
* **Motivo:** El padding desalinea el patrón de salteo de bytes por canal ($B, G, R$). Al no haber padding, el descarte del canal rojo se mantiene matemáticamente regular en todo el flujo de bytes.
* *Nota para otros algoritmos (LSB1, LSB4):* Según la pauta inicial, si una imagen tuviera padding, esos algoritmos usan todo el bloque de bytes contiguo (píxeles + padding). Para LSBI, simplemente no se soportan imágenes con padding.

### 2. Formato del Payload y Orden en el Portador
El flujo de bytes a embeber tiene una cabecera fija de 4 bits (1 por byte, utilizando LSB1 estándar) antes de los datos:

$$\text{Estructura embebida} = \text{Map (4 bits)} \mathbin{\Vert} \text{Payload}$$

* **Modo no encriptado:**  
  `[4 bits mapa] || [tamaño real (4B)] || [bytes archivo] || [extensión + '\0']`
* **Modo encriptado:**  
  `[4 bits mapa] || [tamaño cifrado (4B)] || [encripción(tamaño real || bytes archivo || extensión)]`

---

## 2. Reglas del Algoritmo LSBI

### A. Los 4 Bits de Control (Mapa de Inversión)
* **Dónde se guardan:** En los **primeros 4 bytes consecutivos** del cuerpo de la imagen (área de datos de píxeles), usando **LSB1 estándar**.
* **Canales usados:** Se graban en los 4 bytes contiguos **sin saltear ningún canal** (usan canales $B, G, R, B$).
* **Significado y Orden:**  
  Corresponden exactamente al orden de patrones: **`00`**, **`01`**, **`10`**, **`11`** (determinados por el 2° y 3° LSB de cada byte portador evaluado).
  * `1`: El patrón fue invertido.
  * `0`: El patrón NO fue invertido.
  * *Ejemplo:* Si se invirtieron `00` y `10`, los bits son `1010`.

### B. Embebido del Payload (A partir del 5° byte)
* **Canales válidos:** Se utilizan **únicamente los canales Azul (B) y Verde (G)**. Se **saltea el canal Rojo (R)** (actúa como ruido).
* **Mapeo de patrones:** Para cada byte portador válido ($B$ o $G$), se evalúan los bits en las posiciones 1 y 2 (2° y 3° LSB).
* **Inversión condicional:**
  1. Se simula/aplica el LSB estándar del payload sobre los bytes $B$ y $G$.
  2. Para cada uno de los 4 patrones (`00`, `01`, `10`, `11`), se cuenta cuántos bytes cambiaron respecto al cover original.
  3. Si $\text{Cambiados} > \text{No Cambiados}$, ese patrón se invierte en todos sus bytes y su bit en el mapa de 4 bits se fija en `1`. Caso contrario, no se invierte y se fija en `0`.
  4. Los 4 bits resultantes se guardan en los primeros 4 bytes de la imagen.

---

## 3. Algoritmo de Extracción (De-Steganography)

1. **Leer Metadatos:** Extraer el bit LSB de los primeros 4 bytes de la imagen. Esto da el mapa de 4 bits para `[00, 01, 10, 11]`.
2. **Iterar Bytes Portadores:** A partir del 5° byte, procesar solo canales $B$ y $G$ (saltear siempre el 3° byte correspondiente al canal $R$).
3. **Re-invertir:** Si el 2° y 3° LSB del byte corresponden a un patrón marcado con `1` en el mapa, invertir su LSB (`bit ^ 1`). Si está en `0`, mantener su LSB.
4. **Reconstruir Payload:** Concatenar los LSB resultantes para obtener el tamaño, los datos del archivo y la extensión.

---

## 4. Nota sobre la Eficiencia de los algoritmos

El algoritmo de inserción (Steganography) requiere dos pasadas:
- La primera, para embeber todos los datos dentro de la portadora. Aquí, a medida que se hace la pasada, también se llevan los contadores del mapa de bits.
- La segunda, para luego de terminada la primera, invertir aquellos bits que sea necesario (esta pasada puede omitirse si el resultado fuera que no es necesario invertir).

El algoritmo de etracción (De-Steganography) debe poder leer los datos utilizando solo una pasada. Al leer primero los datos del mapa de inversión, el algoritmo puede decidir si invertir o no el bit leído al momento de leerlo. Al hacer la extracción, conviene utilizar un array de 4 posiciones de tipo `uint8_t` para guardar los valores (0 o 1) para aplicar la operación XOR sin la necesidad de utilizar condicionales.
