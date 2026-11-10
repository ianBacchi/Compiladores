#ifndef LEITOR_ARQUIVO_H
#define LEITOR_ARQUIVO_H

#include <fstream>
#include <string>

using namespace std;

class LeitorArquivo {
private:
    ifstream file;
    int linha;
    char ultimoCaractere;

public:
    LeitorArquivo(string arquivo) : linha(1), ultimoCaractere('\0') {
        file.open(arquivo);
    }

    ~LeitorArquivo() {
        if (file.is_open()) {
            file.close();
        }
    }

    bool abriu() const {
        return file.is_open();
    }

    int lerProxCaracter() {
        char caractere;
        if (!file.is_open() || !file.get(caractere)) {
            return -1;
        }

        ultimoCaractere = caractere;
        if (caractere == '\n') {
            linha++;
        }

        return caractere;
    }

    void devolverCaracter() {
        if (file.is_open()) {
            file.unget();
            if (ultimoCaractere == '\n') {
                linha--;
            }
        }
    }

    int linhaAtual() const {
        return linha;
    }
};

#endif
