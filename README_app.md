# App de consola — Gestión de Cursos (C++/CMake)

Aplicación de consola portable (Windows y Linux) que gestiona cursos en `cursos.dat` con registros binarios fijos y exporta a `web/cursos.json`.

## Compilación

Requisitos: CMake ≥ 3.10 y un compilador C++17 (MSVC, MinGW, Clang o GCC).

### Windows (PowerShell / CMD)

1. Crear carpeta de build y generar proyecto:
   - Con MSVC (Visual Studio):
     - `mkdir build && cd build`
     - `cmake -G "Visual Studio 17 2022" ..`
     - `cmake --build . --config Release`
   - Con MinGW:
     - `mkdir build && cd build`
     - `cmake -G "MinGW Makefiles" ..`
     - `cmake --build . --config Release`

2. El ejecutable queda como:
   - MSVC: `build\app\Release\cursos.exe`
   - MinGW: `build\app\cursos.exe`

### Linux (bash)

```
mkdir -p build && cd build
cmake ..
cmake --build . --config Release
```

El ejecutable queda en `build/app/cursos`.

## Uso

1. Ejecutá el binario `cursos` desde la raíz del proyecto o desde `build/`. La app ahora exporta automáticamente a `web/cursos.json` cuando hay cambios (altas, modificaciones, datos de ejemplo).
2. El menú ofrece:
   - Cargar curso (valida código único; nombre UTF‑8 truncado a 60 bytes sin cortar multibyte; horas ≥ 0; costo ≥ 0).
   - Registrar cursos consecutivos (bucle hasta cancelar).
   - Promedio de horas.
   - Búsqueda por código.
   - Conteo de cursos con costo mayor a un umbral.
   - Listado alfabético por nombre (no reordena el binario).
   - Clasificación por costo: Económico (< 20.000), Estándar (20.000–59.999,99), Premium (≥ 60.000). Umbrales modificables para la sesión.
   - Extremos (más caro/s y más barato/s) con empates.
   - Modificar costo in‑place (actualiza solo el campo `double` por offset, sin reescribir todo el archivo).
   - Exportar a JSON (genera/actualiza `web/cursos.json`).
   - Cargar datos de ejemplo.
   - Limpiar pantalla.

3. Exportación a JSON: crea `web/` si no existe y escribe `web/cursos.json` en UTF‑8 (pretty). Si ejecutás desde `build/`, intenta escribir en `../web/cursos.json` automáticamente.

## Notas técnicas

- Registro binario fijo (76 bytes) con `#pragma pack(push,1)` para evitar padding.
- Escrituras de altas en apéndice (`ios::app`).
- Búsqueda por código por lectura secuencial, devolviendo la posición del registro.
- Actualización in‑place del costo: cálculo de offset con `offsetof` y escritura de 8 bytes.
- Truncamiento de nombre en UTF‑8 sin cortar multibyte (hasta 60 bytes, con padding cero).
- Manejo de errores: mensajes claros al usuario; si `cursos.dat` no existe, se crea vacío al iniciar.
- En Windows, la consola se configura a UTF‑8 para que se vean bien las tildes y acentos.

## Datos de ejemplo

Usá la opción “Cargar datos de ejemplo” para insertar ~10 cursos con valores variados (sirve para probar promedio, conteos, ordenamientos, clasificación, extremos y exportación).
