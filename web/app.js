// Lee cursos.json y ofrece búsqueda, filtros, orden y paginación

const UMBRALES = { econ: 20000, prem: 60000 }; // Deben coincidir con la app (por defecto)

const state = {
  raw: [],
  filt: [],
  q: "",
  cats: new Set(["Económico", "Estándar", "Premium"]),
  min: undefined,
  max: undefined,
  sort: "nombre-asc",
  page: 1,
  pageSize: 10,
};

function categoria(costo){
  if (costo < UMBRALES.econ) return "Económico";
  if (costo < UMBRALES.prem) return "Estándar";
  return "Premium";
}

function fmtMoney(n){ return new Intl.NumberFormat('es-AR', { style:'currency', currency:'ARS', maximumFractionDigits:2 }).format(n); }

async function load(){
  try{
    const res = await fetch('cursos.json', { cache:'no-store' });
    const data = await res.json();
    state.raw = Array.isArray(data) ? data : [];
    recalcKPIs();
    applyFilters();
  }catch(err){
    console.error('Error leyendo cursos.json', err);
    document.getElementById('tbody').innerHTML = `<tr><td colspan="5">No se pudo leer cursos.json. Si estás abriendo el archivo localmente, usá un servidor HTTP o GitHub Pages.</td></tr>`;
  }
}

function recalcKPIs(){
  const t = state.raw.length;
  document.getElementById('kpiTotal').textContent = t;
  if (t === 0){
    document.getElementById('kpiProm').textContent = '0';
    document.getElementById('kpiMax').textContent = '-';
    document.getElementById('kpiMin').textContent = '-';
    return;
  }
  const horas = state.raw.reduce((a,b)=>a+b.horas,0);
  document.getElementById('kpiProm').textContent = (horas/t).toFixed(2);
  const max = Math.max(...state.raw.map(c=>c.costo));
  const min = Math.min(...state.raw.map(c=>c.costo));
  const maxNames = state.raw.filter(c=>c.costo===max).map(c=>`[${c.codigo}] ${c.nombre}`).join('; ');
  const minNames = state.raw.filter(c=>c.costo===min).map(c=>`[${c.codigo}] ${c.nombre}`).join('; ');
  document.getElementById('kpiMax').textContent = `${fmtMoney(max)} — ${maxNames}`;
  document.getElementById('kpiMin').textContent = `${fmtMoney(min)} — ${minNames}`;
}

function applyFilters(){
  const q = state.q.trim().toLowerCase();
  let arr = state.raw.slice();
  if (q){
    arr = arr.filter(c => c.nombre.toLowerCase().includes(q) || String(c.codigo).includes(q));
  }
  arr = arr.filter(c => state.cats.has(categoria(c.costo)));
  if (state.min !== undefined) arr = arr.filter(c => c.costo >= state.min);
  if (state.max !== undefined) arr = arr.filter(c => c.costo <= state.max);

  const [key, dir] = state.sort.split('-');
  const mul = dir === 'asc' ? 1 : -1;
  arr.sort((a,b)=>{
    let va, vb;
    if (key === 'nombre'){ va = a.nombre.toLowerCase(); vb = b.nombre.toLowerCase(); }
    else if (key === 'costo'){ va = a.costo; vb = b.costo; }
    else { va = a.horas; vb = b.horas; }
    if (va < vb) return -1*mul; if (va > vb) return 1*mul; return 0;
  });

  state.filt = arr;
  state.page = 1;
  render();
}

function render(){
  const total = state.filt.length;
  const pages = Math.max(1, Math.ceil(total / state.pageSize));
  state.page = Math.min(state.page, pages);
  const start = (state.page - 1) * state.pageSize;
  const slice = state.filt.slice(start, start + state.pageSize);
  const tb = document.getElementById('tbody');
  if (slice.length === 0){
    tb.innerHTML = `<tr><td colspan="5">No hay cursos para mostrar.</td></tr>`;
  } else {
    tb.innerHTML = slice.map(c => {
      const cat = categoria(c.costo);
      const badge = cat === 'Económico' ? 'badge econ' : (cat === 'Estándar' ? 'badge est' : 'badge prem');
      return `<tr>
        <td>${c.codigo}</td>
        <td>${c.nombre}</td>
        <td>${c.horas}</td>
        <td>${fmtMoney(c.costo)}</td>
        <td><span class="${badge}">${cat}</span></td>
      </tr>`;
    }).join('');
  }
  document.getElementById('pageInfo').textContent = `Página ${state.page} de ${pages} — ${total} resultados`;
  document.getElementById('prev').disabled = state.page<=1;
  document.getElementById('next').disabled = state.page>=pages;
}

// Eventos UI
document.getElementById('q').addEventListener('input', e => { state.q = e.target.value; applyFilters(); });
document.getElementById('q').addEventListener('keydown', e => { if (e.key === 'Enter') applyFilters(); });
document.querySelectorAll('.cat').forEach(chk => {
  chk.addEventListener('change', e => {
    if (e.target.checked) state.cats.add(e.target.value); else state.cats.delete(e.target.value);
    applyFilters();
  });
});
document.getElementById('minCost').addEventListener('input', e => {
  const v = e.target.value; state.min = v === '' ? undefined : Number(v); applyFilters();
});
document.getElementById('maxCost').addEventListener('input', e => {
  const v = e.target.value; state.max = v === '' ? undefined : Number(v); applyFilters();
});
document.getElementById('sort').addEventListener('change', e => { state.sort = e.target.value; applyFilters(); });
document.getElementById('prev').addEventListener('click', () => { if (state.page>1){ state.page--; render(); } });
document.getElementById('next').addEventListener('click', () => { state.page++; render(); });
document.getElementById('pageSize').addEventListener('change', e => { state.pageSize = Number(e.target.value); state.page = 1; render(); });
document.getElementById('reload').addEventListener('click', () => { load(); });

// Tema
const root = document.documentElement;
document.getElementById('themeToggle').addEventListener('click', () => {
  const isLight = root.getAttribute('data-theme') === 'light';
  root.setAttribute('data-theme', isLight ? 'dark' : 'light');
  localStorage.setItem('theme', isLight ? 'dark' : 'light');
});
const saved = localStorage.getItem('theme');
if (saved) root.setAttribute('data-theme', saved);

load();

