#include "FS.h"

int main() {
    FileSystem fs;

    vector<string> perro = {
        "Pieza 1p",
        "Pieza 2p",
    };
    
    vector<string> gato = {
        "Pieza 1g",
        "Pieza 2g",
    };

    fs.saveFigure("Perro", perro);
    fs.saveFigure("Gato", gato);

    fs.listFigures();

    fs.readFigure("Perro");
    fs.readFigure("Gato");

    return 0;
}