// Aplicación de consola para gestionar cursos
// Requisitos: C++17, CMake. Funciona en Windows y Linux.

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

static const char* BIN_FILENAME = "cursos.dat";

#pragma pack(push, 1)
struct CursoRecord {
    int32_t codigo;
    char nombre[60];
    int32_t horas;
    double costo;
};
#pragma pack(pop)

static_assert(sizeof(CursoRecord) == 4 + 60 + 4 + 8, "Tamaño de registro inesperado");

// Utilidades de consola
void pausa() {
    cout << "\nPresioná Enter para continuar...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// Validaciones y entrada robusta
int leer_entero(const string& prompt, int min_val = numeric_limits<int>::min()) {
    while (true) {
        cout << prompt;
        string line;
        if (!getline(cin, line)) { cin.clear(); continue; }
        stringstream ss(line);
        long long v;
        if (ss >> v && !(ss >> line)) {
            if (v >= min_val && v <= numeric_limits<int>::max()) return static_cast<int>(v);
        }
        cout << "Entrada inválida. Intentá de nuevo." << endl;
    }
}

double leer_double(const string& prompt, double min_val = -numeric_limits<double>::infinity()) {
    while (true) {
        cout << prompt;
        string line;
        if (!getline(cin, line)) { cin.clear(); continue; }
        stringstream ss(line);
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
        if (!getline(cin, line)) { cin.clear(); continue; }
        if (line.empty()) continue;
        char c = static_cast<char>(tolower(static_cast<unsigned char>(line[0])));
        if (c == 's') return true;
        if (c == 'n') return false;
        cout << "Respuesta inválida. Poné 's' o 'n'." << endl;
    }
}

string leer_linea(const string& prompt) {
    cout << prompt;
    string line;
    getline(cin, line);
    return line;
}

// UTF-8 helpers: truncamiento seguro a N bytes sin cortar multibyte
static size_t utf8_char_len(unsigned char b) {
    if ((b & 0x80) == 0x00) return 1;          // 0xxxxxxx
    if ((b & 0xE0) == 0xC0) return 2;          // 110xxxxx
    if ((b & 0xF0) == 0xE0) return 3;          // 1110xxxx
    if ((b & 0xF8) == 0xF0) return 4;          // 11110xxx
    return 1; // byte inválido: tratar como 1
}

static void utf8_truncate_to_bytes(const string& s, char out[60]) {
    memset(out, 0, 60);
    const unsigned char* p = reinterpret_cast<const unsigned char*>(s.data());
    size_t n = s.size();
    size_t written = 0;
    for (size_t i = 0; i < n && written < 60;) {
        size_t clen = utf8_char_len(p[i]);
        if (i + clen > n) break;             // secuencia cortada
        if (written + clen > 60) break;      // no cortar multibyte
        for (size_t j = 0; j < clen; ++j) out[written++] = static_cast<char>(p[i + j]);
        i += clen;
    }
}

static string utf8_from_fixed(const char in[60]) {
    size_t len = 0;
    while (len < 60 && in[len] != '\0') ++len;
    return string(in, in + len);
}

// Persistencia binaria
void asegurar_archivo() {
    fstream f(BIN_FILENAME, ios::in | ios::binary);
    if (!f.good()) {
        ofstream o(BIN_FILENAME, ios::binary);
    }
}

vector<CursoRecord> leer_todos() {
    vector<CursoRecord> v;
    ifstream f(BIN_FILENAME, ios::binary);
    if (!f) return v;
    CursoRecord r{};
    while (f.read(reinterpret_cast<char*>(&r), sizeof(r))) v.push_back(r);
    return v;
}

bool buscar_por_codigo(int codigo, size_t& index_out, CursoRecord& rec_out) {
    ifstream f(BIN_FILENAME, ios::binary);
    if (!f) return false;
    CursoRecord r{};
    size_t idx = 0;
    while (f.read(reinterpret_cast<char*>(&r), sizeof(r))) {
        if (r.codigo == codigo) { index_out = idx; rec_out = r; return true; }
        ++idx;
    }
    return false;
}

bool existe_codigo(int codigo) {
    size_t idx; CursoRecord r;
    return buscar_por_codigo(codigo, idx, r);
}

bool append_record(const CursoRecord& r) {
    ofstream f(BIN_FILENAME, ios::binary | ios::app);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(&r), sizeof(r));
    return static_cast<bool>(f);
}

bool update_costo_inplace(size_t index, double nuevo_costo) {
    fstream f(BIN_FILENAME, ios::in | ios::out | ios::binary);
    if (!f) return false;
    streamoff offset = static_cast<streamoff>(index * sizeof(CursoRecord) + offsetof(CursoRecord, costo));
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
    if (costo < u.premium)   return "Estándar";
    return "Premium";
}

// Mostrar curso
void imprimir_curso(const CursoRecord& r, const Umbrales& u) {
    cout << "Código: " << r.codigo << endl;
    cout << "Nombre: " << utf8_from_fixed(r.nombre) << endl;
    cout << "Horas:  " << r.horas << endl;
    cout << fixed << setprecision(2);
    cout << "Costo:  $" << r.costo << " (" << categoria_costo(r.costo, u) << ")" << endl;
    cout.unsetf(ios::floatfield);
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

void agregar_sin_sobrescribir() {
    cout << "\n--- Agregar nuevo curso sin sobrescribir ---\n";
    cargar_un_curso();
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
    cout << fixed << setprecision(2);
    cout << "Promedio de horas: " << prom << endl;
    cout.unsetf(ios::floatfield);
}

void contar_mayores_a_umbral() {
    double umbral = leer_double("Umbral de costo: $", 0.0);
    auto v = leer_todos();
    if (v.empty()) { cout << "No hay cursos cargados." << endl; return; }
    size_t c = 0;
    for (auto& r: v) if (r.costo > umbral) ++c;
    cout << c << " curso(s) superan $" << fixed << setprecision(2) << umbral << "." << endl;
    cout.unsetf(ios::floatfield);
}

static string tolower_ascii(const string& s) {
    string t = s;
    for (char& c : t) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return t;
}

void listar_ordenados_nombre(const Umbrales& u) {
    auto v = leer_todos();
    if (v.empty()) { cout << "No hay cursos cargados." << endl; return; }
    sort(v.begin(), v.end(), [](const CursoRecord& a, const CursoRecord& b){
        string na = tolower_ascii(utf8_from_fixed(a.nombre));
        string nb = tolower_ascii(utf8_from_fixed(b.nombre));
        return na < nb;
    });
    cout << "Listado alfabético por nombre (" << v.size() << "):\n";
    for (auto& r: v) {
        cout << "- [" << r.codigo << "] " << utf8_from_fixed(r.nombre)
             << " | " << r.horas << " h | $" << fixed << setprecision(2) << r.costo
             << " (" << categoria_costo(r.costo, u) << ")\n";
        cout.unsetf(ios::floatfield);
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
    for (auto& r: econ) cout << "  - [" << r.codigo << "] " << utf8_from_fixed(r.nombre) << " ($" << fixed << setprecision(2) << r.costo << ")\n";
    cout << "Estándar: " << est.size() << "\n";
    for (auto& r: est) cout << "  - [" << r.codigo << "] " << utf8_from_fixed(r.nombre) << " ($" << fixed << setprecision(2) << r.costo << ")\n";
    cout << "Premium: " << prem.size() << "\n";
    for (auto& r: prem) cout << "  - [" << r.codigo << "] " << utf8_from_fixed(r.nombre) << " ($" << fixed << setprecision(2) << r.costo << ")\n";
    cout.unsetf(ios::floatfield);
}

void extremos_costo(const Umbrales& u) {
    auto v = leer_todos();
    if (v.empty()) { cout << "No hay cursos cargados." << endl; return; }
    double minc = v.front().costo, maxc = v.front().costo;
    for (auto& r: v) { minc = min(minc, r.costo); maxc = max(maxc, r.costo); }
    cout << fixed << setprecision(2);
    cout << "Más barato(s) - $" << minc << ":\n";
    for (auto& r: v) if (r.costo == minc) cout << "  - [" << r.codigo << "] " << utf8_from_fixed(r.nombre) << "\n";
    cout << "Más caro(s) - $" << maxc << ":\n";
    for (auto& r: v) if (r.costo == maxc) cout << "  - [" << r.codigo << "] " << utf8_from_fixed(r.nombre) << "\n";
    cout.unsetf(ios::floatfield);
}

void buscar_por_codigo_op2(const Umbrales& u) {
    int codigo = leer_entero("Código a buscar: ", 1);
    size_t idx; CursoRecord r;
    bool found = buscar_por_codigo(codigo, idx, r);
    if (!found) { cout << "No se encontró el curso." << endl; return; }
    cout << "Encontrado en posición #" << idx << ":\n";
    imprimir_curso(r, u);
}

void modificar_costo_inplace_op2() {
    int codigo = leer_entero("Código a modificar: ", 1);
    size_t idx; CursoRecord r;
    bool found = buscar_por_codigo(codigo, idx, r);
    if (!found) { cout << "No se encontró el curso." << endl; return; }
    double nuevo = leer_double("Nuevo costo (>= 0): $", 0.0);
    if (update_costo_inplace(idx, nuevo)) cout << "Costo actualizado in-place correctamente." << endl;
    else cout << "Error al actualizar el costo." << endl;
}

void menu2() {
    Umbrales umbrales;
    while (true) {
        cout << "\n==============================\n";
        cout << " Gestion de Cursos cursos.dat\n";
        cout << "==============================\n";
        cout << "1) Cargar cursos con codigo nombre horas y costo\n";
        cout << "2) Calcular el promedio de horas\n";
        cout << "3) Agregar nuevos cursos sin sobrescribir\n";
        cout << "4) Buscar un curso por codigo\n";
        cout << "5) Registrar cursos en bucle\n";
        cout << "6) Mostrar cuantos cursos superan cierto costo\n";
        cout << "7) Ordenar cursos alfabeticamente\n";
        cout << "8) Clasificar cursos por costo economicos estandar premium\n";
        cout << "9) Indicar el curso mas caro y el mas barato\n";
        cout << "10) Modificar el costo de un curso directamente en el archivo\n";
        cout << "0) Salir\n";
        int op = leer_entero("Elige una opcion: ");
        switch (op) {
            case 1: cargar_un_curso(); break;
            case 2: promedio_horas(); break;
            case 3: agregar_sin_sobrescribir(); break;
            case 4: buscar_por_codigo_op2(umbrales); break;
            case 5: registrar_en_bucle(); break;
            case 6: contar_mayores_a_umbral(); break;
            case 7: listar_ordenados_nombre(umbrales); break;
            case 8: clasificar_por_costo(umbrales); break;
            case 9: extremos_costo(umbrales); break;
            case 10: modificar_costo_inplace_op2(); break;
            case 0: cout << "Hasta luego!" << endl; return;
            default: cout << "Opcion invalida." << endl; break;
        }
        pausa();
    }
}

int main() {
    asegurar_archivo();
    menu2();
    return 0;
}

