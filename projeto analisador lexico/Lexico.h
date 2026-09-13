#ifndef LEXICO_H
#define LEXICO_H

#include <cctype>
#include <iostream>
#include <map>
#include <string>

#include "LeitorArquivo.h"
#include "Token.h"

using namespace std;

class Lexico {
private:
    LeitorArquivo *arq;
    int proximoId;
    bool fim;
    map<string, string> palavrasChave;

    Token *token(string lexema, string tipo) {
        proximoId++;
        return new Token(lexema, tipo, proximoId);
    }

    void erro(string lexema, int linha) {
        cerr << "Erro Lexico na linha " << linha
             << ": Desconhecido \"" << lexema << "\"" << endl;
    }

    void carregarPalavrasChave() {
        palavrasChave["DEC"] = "PCDec";
        palavrasChave["PROG"] = "PCProg";
        palavrasChave["INT"] = "PCInt";
        palavrasChave["LER"] = "PCLer";
        palavrasChave["REAL"] = "PCReal";
        palavrasChave["IMPRIMIR"] = "PCImprimir";
        palavrasChave["SE"] = "PCSe";
        palavrasChave["SENAO"] = "PCSenao";
        palavrasChave["ENTAO"] = "PCEntao";
        palavrasChave["ENQTO"] = "PCEnqto";
        palavrasChave["INI"] = "PCIni";
        palavrasChave["FIM"] = "PCFim";
        palavrasChave["E"] = "OpBoolE";
        palavrasChave["OU"] = "OpBoolOu";
    }

    Token *lerIdentificadorOuPalavra(int primeiro, int linha) {
        string lexema(1, static_cast<char>(primeiro));
        int c;

        while ((c = arq->lerProxCaracter()) != -1 &&
               (isalnum(static_cast<unsigned char>(c)) != 0)) {
            lexema += static_cast<char>(c);
        }

        if (c != -1) {
            arq->devolverCaracter();
        }

        if (lexema[0] >= 'A' && lexema[0] <= 'Z') {
            map<string, string>::const_iterator palavra = palavrasChave.find(lexema);
            if (palavra != palavrasChave.end()) {
                return token(lexema, palavra->second);
            }
            erro(lexema, linha);
            return nullptr;
        }

        return token(lexema, "Var");
    }

    Token *lerNumero(int primeiro, int linha) {
        string lexema(1, static_cast<char>(primeiro));
        bool temPonto = false;
        int c;

        while ((c = arq->lerProxCaracter()) != -1) {
            if (isdigit(static_cast<unsigned char>(c)) != 0) {
                lexema += static_cast<char>(c);
            } else if (c == '.' && !temPonto) {
                temPonto = true;
                lexema += static_cast<char>(c);
            } else {
                arq->devolverCaracter();
                break;
            }
        }

        if (lexema.back() == '.') {
            erro(lexema, linha);
            return nullptr;
        }

        return token(lexema, temPonto ? "NumReal" : "NumInt");
    }

    Token *lerCadeia(int linha) {
        string lexema;
        int c;

        while ((c = arq->lerProxCaracter()) != -1 && c != '"') {
            if (c == '\n') {
                erro(lexema, linha);
                return nullptr;
            }
            lexema += static_cast<char>(c);
        }

        if (c == -1) {
            erro(lexema, linha);
            return nullptr;
        }

        return token(lexema, "Cadeia");
    }

public:
    Lexico(string arquivo) : arq(new LeitorArquivo(arquivo)), proximoId(0), fim(false) {
        carregarPalavrasChave();
    }

    ~Lexico() {
        delete arq;
    }

    bool abriuArquivo() const {
        return arq->abriu();
    }

    Token *proximoToken() {
        if (fim) {
            return nullptr;
        }

        int c;
        while ((c = arq->lerProxCaracter()) != -1) {
            int linha = arq->linhaAtual();

            if (isspace(static_cast<unsigned char>(c)) != 0) {
                continue;
            }

            if (c == '#') {
                while ((c = arq->lerProxCaracter()) != -1 && c != '\n') {
                }
                continue;
            }

            if (isalpha(static_cast<unsigned char>(c)) != 0) {
                return lerIdentificadorOuPalavra(c, linha);
            }

            if (isdigit(static_cast<unsigned char>(c)) != 0) {
                return lerNumero(c, linha);
            }

            if (c == '"') {
                return lerCadeia(linha);
            }

            switch (c) {
                case ':':
                    c = arq->lerProxCaracter();
                    if (c == '=') return token(":=", "Atrib");
                    if (c != -1) arq->devolverCaracter();
                    return token(":", "Delim");
                case '<':
                    c = arq->lerProxCaracter();
                    if (c == '=') return token("<=", "OpRelMenorIgual");
                    if (c != -1) arq->devolverCaracter();
                    return token("<", "OpRelMenor");
                case '>':
                    c = arq->lerProxCaracter();
                    if (c == '=') return token(">=", "OpRelMaiorIgual");
                    if (c != -1) arq->devolverCaracter();
                    return token(">", "OpRelMaior");
                case '=':
                    c = arq->lerProxCaracter();
                    if (c == '=') return token("==", "OpRelIgual");
                    if (c != -1) arq->devolverCaracter();
                    erro("=", linha);
                    return nullptr;
                case '!':
                    c = arq->lerProxCaracter();
                    if (c == '=') return token("!=", "OpRelDif");
                    if (c != -1) arq->devolverCaracter();
                    erro("!", linha);
                    return nullptr;
                case '+': return token("+", "OpAritSoma");
                case '-': return token("-", "OpAritSub");
                case '*': return token("*", "OpAritMult");
                case '/': return token("/", "OpAritDiv");
                case '(': return token("(", "AbrePar");
                case ')': return token(")", "FechaPar");
                default:
                    erro(string(1, static_cast<char>(c)), linha);
                    return nullptr;
            }
        }

        fim = true;
        return token("EOF", "EOF");
    }
};

#endif
