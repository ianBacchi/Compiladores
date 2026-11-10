// Integrantes:
// Ian Bacchi Nascimento - RA 258509
// Joao Pedro da Silva Kawano - RA 2453223

#include <iostream>

#include "Lexico.h"
#include "Sintatico.h"

using namespace std;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "Uso: analisador_sintatico <arquivo.gyh>" << endl;
        return 1;
    }

    Lexico lex(argv[1]);
    if (!lex.abriuArquivo()) {
        cerr << "Erro: nao foi possivel abrir o arquivo " << argv[1] << endl;
        return 1;
    }

    Sintatico sintatico(lex);
    int errosSintaticos = sintatico.analisar();
    int errosLexicos = lex.quantidadeErros();

    if (errosLexicos == 0 && errosSintaticos == 0) {
        cout << "Analise concluida: programa sintaticamente correto." << endl;
        return 0;
    }

    cout << "Analise concluida com " << errosLexicos << " erro(s) lexico(s) e "
         << errosSintaticos << " erro(s) sintatico(s)." << endl;
    return 1;
}
