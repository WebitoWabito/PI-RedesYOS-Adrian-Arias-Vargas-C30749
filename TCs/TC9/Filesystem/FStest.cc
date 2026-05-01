#include "FS.h"

int main() {
    FileSystem fs;

    vector<string> perro = {
        "pieza 1p",
        "pieza 2p",
    };
    
    vector<string> gato = {
        "pieza 1g",
        "pieza 2g",
    };

    fs.saveFigure("Perro", perro);
    fs.saveFigure("Gato", gato);

    fs.listFigures();

    fs.readFigure("Perro");
    fs.readFigure("Gato");

    return 0;
}