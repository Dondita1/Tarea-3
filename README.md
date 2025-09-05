## Gestión de Cursos — App de consola + Web (GitHub Pages)

Este proyecto incluye:

- Aplicación de consola portable (Windows/Linux) que gestiona cursos en un archivo binario fijo `cursos.dat` y exporta a JSON legible `web/cursos.json`.
- Sitio web estático (sin frameworks) en `web/` que lee `web/cursos.json` y permite búsqueda, filtros, ordenamientos y paginación. Funciona en GitHub Pages.
- Documentación paso a paso para compilar/usar la app y publicar la web.

### Flujo de trabajo

1. Usá la app de consola para cargar/editar cursos. El archivo binario `cursos.dat` es la fuente de verdad.
2. Ejecutá la opción “Exportar a JSON” en la app para generar/actualizar `web/cursos.json` (UTF‑8, pretty).
3. Commit y push al repositorio (incluyendo `web/cursos.json`).
4. La web (en GitHub Pages) lee el JSON actualizado y muestra los datos.

> Nota: GitHub Pages sirve contenido estático. La edición de datos se hace desde la app; luego se exporta y se sube el `web/cursos.json` al repo.

### Estructura del repo

- `app/`: código fuente C++ y `CMakeLists.txt` de la app de consola.
- `web/`: sitio estático (`index.html`, `styles.css`, `app.js`, y `cursos.json`).
- `cursos.dat`: archivo binario (se crea/actualiza al usar la app).
- `README_app.md`: compilación y uso de la app en Windows/Linux.
- `README_web.md`: publicación y uso en GitHub Pages.
- `CMakeLists.txt`: orquesta el build del subproyecto `app/`.
- `.gitignore`: ignora directorios de build y binarios.

### Requisitos cumplidos (resumen)

- Binario fijo `cursos.dat` con registros de 76 bytes: `int32 codigo`, `char nombre[60]` (UTF‑8 truncamiento seguro), `int32 horas`, `double costo`.
- Altas por apéndice, búsqueda secuencial por código, actualización in‑place del costo por offset.
- Menú en español (Argentina), entradas validadas, mensajes claros, confirmaciones.
- Reportes: promedio de horas, filtros por costo, clasificación por categorías configurable en sesión, extremos con empates, listado alfabético en memoria.
- Exportación `web/cursos.json` UTF‑8 pretty.
- Web: búsqueda, filtros, ordenamientos, KPIs, tabla responsiva con paginación, modo oscuro.

Para compilar/usar la app y publicar la web, seguí los pasos en `README_app.md` y `README_web.md`.

