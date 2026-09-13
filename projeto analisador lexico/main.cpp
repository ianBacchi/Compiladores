// Integrantes:
// Ian Bacchi Nascimento - RA 258509
// Joao Pedro da Silva Kawano - RA 2453223

#include <iostream>
#include "Lexico.h"

using namespace std;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "Uso: analisador_lexico <arquivo.gyh>" << endl;
        return 1;
    }

    Lexico lex(argv[1]);
    if (!lex.abriuArquivo()) {
        cerr << "Erro: nao foi possivel abrir o arquivo " << argv[1] << endl;
        return 1;
    }

    Token *t = lex.proximoToken();
    while (t != nullptr) {
        cout << t->toString() << endl;
        delete t;
        t = lex.proximoToken();
    }

    return 0;
}
