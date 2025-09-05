// Aplicación de consola para gestionar cursos
// Requisitos: C++17, CMake. Funciona en Windows y Linux.

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
#include <cstddef>

#if defined(_WIN32)
#include <windows.h>
#endif

using std::cin;
using std::cout;
using std::endl;
using std::size_t;
using std::string;
using std::vector;

static const char* BIN_FILENAME = "cursos.dat";
static const char* WEB_DIR = "web";
static const char* WEB_JSON = "web/cursos.json";

#pragma pack(push, 1)
struct CursoRecord {
    int32_t codigo;       // entero positivo y único
    char nombre[60];      // cadena UTF-8 truncada segura a 60 bytes
    int32_t horas;        // entero >= 0
    double costo;         // double >= 0
};
#pragma pack(pop)

static_assert(sizeof(CursoRecord) == 4 + 60 + 4 + 8, "Tamaño de registro inesperado");

// Utilidades de consola
void clear_stdin() {
    cin.clear();
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void pausa() {
    cout << "\nPresioná Enter para continuar...";
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void limpiar_pantalla() {
#if defined(_WIN32)
    system("cls");
#else
    system("clear");
#endif
}

// Validaciones y entrada robusta
int leer_entero(const string& prompt, int min_val = std::numeric_limits<int>::min()) {
    while (true) {
        cout << prompt;
        string line;
        if (!std::getline(cin, line)) {
            cin.clear();
            continue;
        }
        std::stringstream ss(line);
        long long v; // para chequear overflow
        if (ss >> v && !(ss >> line)) {
            if (v >= min_val && v <= std::numeric_limits<int>::max()) return static_cast<int>(v);
        }
        cout << "Entrada inválida. Intentá de nuevo." << endl;
    }
}

double leer_double(const string& prompt, double min_val = -std::numeric_limits<double>::infinity()) {
    while (true) {
        cout << prompt;
        string line;
        if (!std::getline(cin, line)) {
            cin.clear();
            continue;
        }
        std::stringstream ss(line);
        double v;
        if (ss >> v && !(ss >> line)) {
            if (v >= min_val) return v;
        }
        cout << "Entrada inválida. Intentá de nuevo." << endl;
    }
}

bool leer_confirmacion(const string& prompt) {
    while (true) {
        cout << prompt << " (s/n): ";
        string line;
        if (!std::getline(cin, line)) { cin.clear(); continue; }
        if (line.empty()) continue;
        char c = static_cast<char>(std::tolower(static_cast<unsigned char>(line[0])));
        if (c == 's') return true;
        if (c == 'n') return false;
        cout << "Respuesta inválida. Poné 's' o 'n'." << endl;
    }
}

string leer_linea(const string& prompt) {
    cout << prompt;
    string line;
    std::getline(cin, line);
    return line;
}

// UTF-8 helpers: truncamiento seguro a N bytes sin cortar multibyte
static bool is_utf8_start_byte(unsigned char b) {
    // Para bytes de continuación: 10xxxxxx (0x80-0xBF)
    return (b & 0xC0) != 0x80;
}

static size_t utf8_char_len(unsigned char b) {
    if ((b & 0x80) == 0x00) return 1;          // 0xxxxxxx
    if ((b & 0xE0) == 0xC0) return 2;          // 110xxxxx
    if ((b & 0xF0) == 0xE0) return 3;          // 1110xxxx
    if ((b & 0xF8) == 0xF0) return 4;          // 11110xxx
    return 1; // byte inválido: tratar como 1 para no romper
}

static void utf8_truncate_to_bytes(const string& s, char out[60]) {
    std::memset(out, 0, 60);
    const unsigned char* p = reinterpret_cast<const unsigned char*>(s.data());
    size_t n = s.size();
    size_t written = 0;
    for (size_t i = 0; i < n && written < 60; ) {
        size_t clen = utf8_char_len(p[i]);
        if (i + clen > n) break; // secuencia cortada; no copiar
        if (written + clen > 60) break; // no cortar multibyte
        for (size_t j = 0; j < clen; ++j) {
            out[written++] = static_cast<char>(p[i + j]);
        }
        i += clen;
    }
}

static string utf8_from_fixed(const char in[60]) {
    // Leer hasta primer byte cero
    size_t len = 0;
    while (len < 60 && in[len] != '\0') ++len;
    return string(in, in + len);
}

// Persistencia binaria
size_t record_size() { return sizeof(CursoRecord); }

void asegurar_archivo() {
    // Crear si no existe
    std::fstream f(BIN_FILENAME, std::ios::in | std::ios::binary);
    if (!f.good()) {
        std::ofstream o(BIN_FILENAME, std::ios::binary);
        // archivo vacío
    }
}

vector<CursoRecord> leer_todos() {
    vector<CursoRecord> v;
    std::ifstream f(BIN_FILENAME, std::ios::binary);
    if (!f) return v;
    CursoRecord r{};
    while (f.read(reinterpret_cast<char*>(&r), sizeof(r))) {
        v.push_back(r);
    }
    return v;
}

bool buscar_por_codigo(int codigo, size_t& index_out, CursoRecord& rec_out) {
    std::ifstream f(BIN_FILENAME, std::ios::binary);
    if (!f) return false;
    CursoRecord r{};
    size_t idx = 0;
    while (f.read(reinterpret_cast<char*>(&r), sizeof(r))) {
        if (r.codigo == codigo) {
            index_out = idx;
            rec_out = r;
            return true;
        }
        ++idx;
    }
    return false;
}

bool existe_codigo(int codigo) {
    size_t idx; CursoRecord r;
    return buscar_por_codigo(codigo, idx, r);
}

bool append_record(const CursoRecord& r) {
    std::ofstream f(BIN_FILENAME, std::ios::binary | std::ios::app);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(&r), sizeof(r));
    return static_cast<bool>(f);
}

bool update_costo_inplace(size_t index, double nuevo_costo) {
    std::fstream f(BIN_FILENAME, std::ios::in | std::ios::out | std::ios::binary);
    if (!f) return false;
    std::streamoff offset = static_cast<std::streamoff>(index * sizeof(CursoRecord) + offsetof(CursoRecord, costo));
    f.seekp(offset);
    if (!f.good()) return false;
    f.write(reinterpret_cast<const char*>(&nuevo_costo), sizeof(nuevo_costo));
    return static_cast<bool>(f);
}

// Lógica de negocio
struct Umbrales {
    double economico = 20000.0;
    double premium = 60000.0; // estándar: [económico, premium)
};

string categoria_costo(double costo, const Umbrales& u) {
    if (costo < u.economico) return "Económico";
    if (costo < u.premium) return "Estándar";
    return "Premium";
}

// JSON export
static string json_escape(const string& s) {
    std::ostringstream o;
    for (unsigned char c : s) {
        switch (c) {
            case '"': o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\b': o << "\\b"; break;
            case '\f': o << "\\f"; break;
            case '\n': o << "\\n"; break;
            case '\r': o << "\\r"; break;
            case '\t': o << "\\t"; break;
            default:
                if (c < 0x20) {
                    o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c << std::dec;
                } else {
                    o << c;
                }
        }
    }
    return o.str();
}

bool asegurar_directorio_web() {
#if defined(_WIN32)
    DWORD attr = GetFileAttributesA(WEB_DIR);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        return CreateDirectoryA(WEB_DIR, NULL) != 0;
    }
    return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    // Intentar crear, ignorar si ya existe
    int rc = std::system((string("mkdir -p ") + WEB_DIR).c_str());
    (void)rc;
    return true;
#endif
}

bool exportar_json() {
    if (!asegurar_directorio_web()) {
        cout << "No se pudo asegurar el directorio 'web/'." << endl;
        return false;
    }
    vector<CursoRecord> v = leer_todos();
    std::ofstream out(WEB_JSON, std::ios::binary);
    if (!out) {
        cout << "Error al abrir '" << WEB_JSON << "' para escritura." << endl;
        return false;
    }
    out << "[\n";
    for (size_t i = 0; i < v.size(); ++i) {
        const auto& r = v[i];
        string nombre = utf8_from_fixed(r.nombre);
        out << "  {\n";
        out << "    \"codigo\": " << r.codigo << ",\n";
        out << "    \"nombre\": \"" << json_escape(nombre) << "\",\n";
        out << "    \"horas\": " << r.horas << ",\n";
        out << std::fixed << std::setprecision(2);
        out << "    \"costo\": " << r.costo << "\n";
        out.unsetf(std::ios::floatfield);
        out << "  }" << (i + 1 < v.size() ? "," : "") << "\n";
    }
    out << "]\n";
    cout << "Exportación exitosa a '" << WEB_JSON << "'." << endl;
    return true;
}

// Variante con ubicación robusta (web/ o ../web/)
bool exportar_json2() {
#if defined(_WIN32)
    auto dir_exists = [](const std::string& d)->bool{
        DWORD attr = GetFileAttributesA(d.c_str());
        return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
    };
    auto ensure_dir = [](const std::string& d)->bool{
        DWORD attr = GetFileAttributesA(d.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) {
            return CreateDirectoryA(d.c_str(), NULL) != 0;
        }
        return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
    };
#endif
    std::vector<std::string> candidatos = {"web", "../web"};
    std::string elegido;
#if defined(_WIN32)
    for (const auto& d : candidatos) {
        if (dir_exists(d) || ensure_dir(d)) { elegido = d; break; }
    }
#else
    for (const auto& d : candidatos) {
        std::string cmd = "mkdir -p \"" + d + "\"";
        (void)std::system(cmd.c_str());
        elegido = d; break;
    }
#endif
    if (elegido.empty()) elegido = "web";

    auto v = leer_todos();
    std::string path = elegido + "/cursos.json";
    std::ofstream out(path, std::ios::binary);
    if (!out) { cout << "Error al abrir '" << path << "' para escritura." << std::endl; return false; }
    out << "[\n";
    for (size_t i = 0; i < v.size(); ++i) {
        const auto& r = v[i];
        std::string nombre = utf8_from_fixed(r.nombre);
        out << "  {\n";
        out << "    \"codigo\": " << r.codigo << ",\n";
        out << "    \"nombre\": \"" << json_escape(nombre) << "\",\n";
        out << "    \"horas\": " << r.horas << ",\n";
        out << std::fixed << std::setprecision(2);
        out << "    \"costo\": " << r.costo << "\n";
        out.unsetf(std::ios::floatfield);
        out << "  }" << (i + 1 < v.size() ? "," : "") << "\n";
    }
    out << "]\n";
    cout << "Exportación exitosa a '" << path << "'." << std::endl;
    return true;
}

// Mostrar curso
void imprimir_curso(const CursoRecord& r, const Umbrales& u) {
    cout << "Código: " << r.codigo << endl;
    cout << "Nombre: " << utf8_from_fixed(r.nombre) << endl;
    cout << "Horas:  " << r.horas << endl;
    cout << std::fixed << std::setprecision(2);
    cout << "Costo:  $" << r.costo << " (" << categoria_costo(r.costo, u) << ")" << endl;
    cout.unsetf(std::ios::floatfield);
}

// Operaciones
void cargar_un_curso() {
    cout << "\n--- Cargar curso ---\n";
    int codigo;
    while (true) {
        codigo = leer_entero("Código (entero positivo): ", 1);
        if (existe_codigo(codigo)) cout << "Ya existe un curso con ese código. Probá otro." << endl;
        else break;
    }
    string nombre = leer_linea("Nombre (hasta 60 bytes UTF-8): ");
    int horas = leer_entero("Horas (>= 0): ", 0);
    double costo = leer_double("Costo (>= 0): ", 0.0);

    CursoRecord r{};
    r.codigo = codigo;
    utf8_truncate_to_bytes(nombre, r.nombre);
    r.horas = horas;
    r.costo = costo;

    if (append_record(r)) cout << "Curso guardado correctamente (apéndice)." << endl;
    else cout << "Error al guardar el curso." << endl;
}

void registrar_en_bucle() {
    cout << "\n--- Registro consecutivo de cursos ---\n";
    while (true) {
        cargar_un_curso();
        if (!leer_confirmacion("¿Querés cargar otro?")) break;
    }
}

void promedio_horas() {
    auto v = leer_todos();
    if (v.empty()) { cout << "No hay cursos cargados." << endl; return; }
    long long total = 0;
    for (auto& r: v) total += r.horas;
    double prom = static_cast<double>(total) / v.size();
    cout << std::fixed << std::setprecision(2);
    cout << "Promedio de horas: " << prom << endl;
    cout.unsetf(std::ios::floatfield);
}

void buscar_por_codigo_op(const Umbrales& u) {
    int codigo = leer_entero("Código a buscar: ", 1);
    size_t idx; CursoRecord r;
    bool found = buscar_por_codigo(codigo, idx, r);
    if ( !found) { cout <<  "No se encontro el curso." << endl; return; } 
    cout << "Encontrado en posición #" << idx << ":\n";
    imprimir_curso(r, u);
}

void contar_mayores_a_umbral() {
    double umbral = leer_double("Umbral de costo: $", 0.0);
    auto v = leer_todos();
    if (v.empty()) { cout << "No hay cursos cargados." << endl; return; }
    size_t c = 0;
    for (auto& r: v) if (r.costo > umbral) ++c;
    cout << c << " curso(s) superan $" << std::fixed << std::setprecision(2) << umbral << "." << endl;
    cout.unsetf(std::ios::floatfield);
}

static string tolower_ascii(const string& s) {
    string t = s;
    for (char& c : t) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return t;
}

void listar_ordenados_nombre(const Umbrales& u) {
    auto v = leer_todos();
    if (v.empty()) { cout << "No hay cursos cargados." << endl; return; }
    std::sort(v.begin(), v.end(), [](const CursoRecord& a, const CursoRecord& b){
        string na = tolower_ascii(utf8_from_fixed(a.nombre));
        string nb = tolower_ascii(utf8_from_fixed(b.nombre));
        return na < nb;
    });
    cout << "Listado alfabético por nombre (" << v.size() << "):\n";
    for (auto& r: v) {
        cout << "- [" << r.codigo << "] " << utf8_from_fixed(r.nombre) << " | " << r.horas << " h | $" << std::fixed << std::setprecision(2) << r.costo << " (" << categoria_costo(r.costo, u) << ")\n";
        cout.unsetf(std::ios::floatfield);
    }
}

void clasificar_por_costo(const Umbrales& u) {
    auto v = leer_todos();
    if (v.empty()) { cout << "No hay cursos cargados." << endl; return; }
    vector<CursoRecord> econ, est, prem;
    for (auto& r: v) {
        if (r.costo < u.economico) econ.push_back(r);
        else if (r.costo < u.premium) est.push_back(r);
        else prem.push_back(r);
    }
    cout << "Umbrales: Económico < $" << u.economico << ", Estándar < $" << u.premium << ", Premium ≥ ese valor.\n";
    cout << "Económicos: " << econ.size() << "\n";
    for (auto& r: econ) cout << "  - [" << r.codigo << "] " << utf8_from_fixed(r.nombre) << " ($" << std::fixed << std::setprecision(2) << r.costo << ")\n";
    cout << "Estándar: " << est.size() << "\n";
    for (auto& r: est) cout << "  - [" << r.codigo << "] " << utf8_from_fixed(r.nombre) << " ($" << std::fixed << std::setprecision(2) << r.costo << ")\n";
    cout << "Premium: " << prem.size() << "\n";
    for (auto& r: prem) cout << "  - [" << r.codigo << "] " << utf8_from_fixed(r.nombre) << " ($" << std::fixed << std::setprecision(2) << r.costo << ")\n";
    cout.unsetf(std::ios::floatfield);
}

void extremos_costo(const Umbrales& u) {
    auto v = leer_todos();
    if (v.empty()) { cout << "No hay cursos cargados." << endl; return; }
    double minc = v.front().costo, maxc = v.front().costo;
    for (auto& r: v) { minc = std::min(minc, r.costo); maxc = std::max(maxc, r.costo); }
    cout << std::fixed << std::setprecision(2);
    cout << "Más barato(s) — $" << minc << ":\n";
    for (auto& r: v) if (r.costo == minc) cout << "  - [" << r.codigo << "] " << utf8_from_fixed(r.nombre) << "\n";
    cout << "Más caro(s) — $" << maxc << ":\n";
    for (auto& r: v) if (r.costo == maxc) cout << "  - [" << r.codigo << "] " << utf8_from_fixed(r.nombre) << "\n";
    cout.unsetf(std::ios::floatfield);
}

// Variante compatible sin std::optional
void buscar_por_codigo_op2(const Umbrales& u) {
    int codigo = leer_entero("CA3digo a buscar: ", 1);
    size_t idx; CursoRecord r;
    bool found = buscar_por_codigo(codigo, idx, r);
    if (!found) { cout << "No se encontrA3 el curso." << endl; return; }
    cout << "Encontrado en posiciA3n #" << idx << ":\n";
    imprimir_curso(r, u);
}

void modificar_costo_inplace_op2() {
    int codigo = leer_entero("CA3digo a modificar: ", 1);
    size_t idx; CursoRecord r;
    bool found = buscar_por_codigo(codigo, idx, r);
    if (!found) { cout << "No se encontrA3 el curso." << endl; return; }
    double nuevo = leer_double("Nuevo costo (>= 0): $", 0.0);
    if (update_costo_inplace(idx, nuevo)) cout << "Costo actualizado in-place correctamente." << endl;
    else cout << "Error al actualizar el costo." << endl;
}

void modificar_costo_inplace_op() {
    int codigo = leer_entero("Código a modificar: ", 1);
    size_t idx; CursoRecord r;
    bool found = buscar_por_codigo(codigo, idx, r);
    if ( !found) { cout <<  "No se encontro el curso." << endl; return; } 
    double nuevo = leer_double("Nuevo costo (>= 0): $", 0.0);
    if (update_costo_inplace(idx, nuevo)) cout << "Costo actualizado in-place correctamente." << endl;
    else cout << "Error al actualizar el costo." << endl;
}

void modificar_umbrales(Umbrales& u) {
    cout << "Umbrales actuales: Económico < $" << u.economico << ", Estándar < $" << u.premium << ", Premium ≥ ese valor.\n";
    double nuevo_e = leer_double("Nuevo umbral Económico (< Premium): $", 0.0);
    double nuevo_p = leer_double("Nuevo umbral Premium (> Económico): $", 0.0);
    if (nuevo_p <= nuevo_e) {
        cout << "Valores inválidos: Premium debe ser mayor a Económico. Se mantienen los anteriores." << endl;
        return;
    }
    u.economico = nuevo_e;
    u.premium = nuevo_p;
    cout << "Umbrales actualizados para esta sesión." << endl;
}

void cargar_datos_ejemplo() {
    cout << "\nCargando datos de ejemplo...\n";
    struct C { int codigo; const char* nombre; int horas; double costo; };
    vector<C> s = {
        {101, "Programación en C++", 40, 35000},
        {102, "Introducción a Python", 30, 18000},
        {103, "Bases de Datos SQL", 35, 25000},
        {104, "Desarrollo Web", 50, 60000},
        {105, "Machine Learning", 60, 85000},
        {106, "Excel Avanzado", 25, 15000},
        {107, "Redes y Seguridad", 45, 55000},
        {108, "Análisis de Datos", 55, 60000},
        {109, "Git y GitHub", 12, 8000},
        {110, "Arquitectura de Software", 48, 70000}
    };
    int agregados = 0, duplicados = 0;
    for (auto& c : s) {
        if (existe_codigo(c.codigo)) { ++duplicados; continue; }
        CursoRecord r{};
        r.codigo = c.codigo;
        utf8_truncate_to_bytes(c.nombre, r.nombre);
        r.horas = c.horas;
        r.costo = c.costo;
        if (append_record(r)) ++agregados;
    }
    cout << "Ejemplos agregados: " << agregados << ", ignorados por duplicado: " << duplicados << "." << endl;
}

void menu() {
    Umbrales umbrales; // modificables durante la sesión
    while (true) {
        cout << "\n==============================\n";
        cout << " Gestión de Cursos (cursos.dat)\n";
        cout << "==============================\n";
        cout << "1) Cargar curso\n";
        cout << "2) Registrar cursos en bucle\n";
        cout << "3) Promedio de horas\n";
        cout << "4) Buscar curso por código\n";
        cout << "5) Contar cursos con costo > umbral\n";
        cout << "6) Listar cursos por nombre (A→Z)\n";
        cout << "7) Clasificar por costo (econ/est/prem)\n";
        cout << "8) Curso(s) más caro(s) y más barato(s)\n";
        cout << "9) Modificar costo (actualización in-place)\n";
        cout << "10) Exportar a JSON (web/cursos.json)\n";
        cout << "11) Cargar datos de ejemplo\n";
        cout << "12) Modificar umbrales de categorías\n";
        cout << "13) Limpiar pantalla\n";
        cout << "0) Salir\n";
        int op = leer_entero("Elegí una opción: ");
        switch (op) {
            case 1: cargar_un_curso(); exportar_json2(); break;
            case 2: registrar_en_bucle(); exportar_json2(); break;
            case 3: promedio_horas(); break;
            case 4: buscar_por_codigo_op2(umbrales); break;
            case 5: contar_mayores_a_umbral(); break;
            case 6: listar_ordenados_nombre(umbrales); break;
            case 7: clasificar_por_costo(umbrales); break;
            case 8: extremos_costo(umbrales); break;
            case 9: modificar_costo_inplace_op2(); exportar_json2(); break;
            case 10: exportar_json2(); break;
            case 11: cargar_datos_ejemplo(); exportar_json2(); break;
            case 12: modificar_umbrales(umbrales); break;
            case 13: limpiar_pantalla(); break;
            case 0: cout << "¡Hasta luego!" << endl; return;
            default: cout << "Opción inválida." << endl; break;
        }
        pausa();
    }
}

int main() {
    asegurar_archivo();
    menu();
    return 0;
}
