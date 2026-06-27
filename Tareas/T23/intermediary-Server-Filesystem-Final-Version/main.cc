#include "FileSystem.h"
#include <iostream>

using namespace std;

void menu() {
    cout << endl << "===== MINI FILESYSTEM =====" << endl;
    cout << "1. Crear figura" << endl;
    cout << "2. Leer figura" << endl;
    cout << "3. Ver directorio" << endl;
    cout << "4. Ver bitmap" << endl;
    cout << "5. Salir" << endl;
    cout << "Seleccione una opcion: ";
}

int main() {
    FileSystem fs("disk.bin", 100);

    int opcion;

    do {
        menu();
        cin >> opcion;
        cin.ignore();

        if (opcion == 1) {
            string nombre, contenido;

            cout << "Nombre de la figura: ";
            getline(cin, nombre);

            cout << "Contenido (escriba varias lineas, termine con END):" << endl;

            string linea;
            contenido.clear();

            while (true) {
                getline(cin, linea);
                if (linea == "END") break;
                contenido += linea + "\n";
            }

            fs.writeFile(nombre, contenido);
            cout << "Figura guardada." << endl;

        } 
        else if (opcion == 2) {
            string nombre;

            cout << "Nombre de la figura: ";
            getline(cin, nombre);

            string data = fs.readFile(nombre);

            if (data.empty()) {
                cout << "No existe la figura." << endl;
            } else {
                cout << "Contenido:" << endl << data << endl;
            }

        } 
        else if (opcion == 3) {
            fs.printDirectory();

        } 
        else if (opcion == 4) {
            fs.printBitmap();

        } 
        else if (opcion == 5) {
            cout << "Saliendo..." << endl;
        }

    } while (opcion != 5);

    return 0;
}