# Web estática — Visualización de Cursos (GitHub Pages)

Sitio estático sin frameworks en `web/` que lee `web/cursos.json` y permite búsqueda, filtros, ordenamientos y paginación.

## Funcionamiento

- La página `web/index.html` carga `web/cursos.json` con `fetch` y muestra:
  - Búsqueda por nombre o código.
  - Filtros por categoría (Económico/Estándar/Premium) y por rango de costo (mín/máx).
  - Ordenamiento por nombre (A→Z, Z→A), costo (↑/↓) y horas (↑/↓).
  - KPIs arriba: total, promedio de horas, curso(s) más caro(s) y más barato(s).
  - Tabla responsiva con paginación simple y modo oscuro/claro.

Importante: `fetch` requiere servir por HTTP(s). Si abrís `index.html` directamente como `file://`, el navegador puede bloquear la lectura de `cursos.json`. Usá GitHub Pages o un servidor local.

## Publicación en GitHub Pages

1. Hacé commit y push de todo el repo incluyendo `web/cursos.json`.
2. En la configuración del repositorio, activá GitHub Pages (Source: "Deploy from a branch" y Branch: `main`/`master`, carpeta `/` o `/docs`).
3. La web queda disponible en la URL del repositorio. Accedé a `/web/` (por ejemplo: `https://usuario.github.io/tu-repo/web/`).
4. Cada vez que exportes un JSON nuevo desde la app de consola, volvé a hacer commit/push para que la web tome los cambios.

## Umbrales de categorías

Los umbrales por defecto del sitio son: Económico < $20.000; Estándar < $60.000; Premium ≥ ese valor. Coinciden con la app por defecto.
