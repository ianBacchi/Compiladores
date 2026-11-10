#ifndef SINTATICO_H
#define SINTATICO_H

/*
 * Analisador Sintatico Descendente Recursivo (sem backtracking) - Linguagem GYH
 *
 * Gramatica usada (recursao a esquerda eliminada e fatorada):
 *
 *   Programa            -> ':' DEC ListaDeclaracoes ':' PROG ListaComandos EOF
 *   ListaDeclaracoes    -> Declaracao { Declaracao }
 *   Declaracao          -> Var ':' TipoVar
 *   TipoVar             -> INT | REAL
 *
 *   ExpressaoAritmetica -> TermoAritmetico { ('+' | '-') TermoAritmetico }
 *   TermoAritmetico     -> FatorAritmetico { ('*' | '/') FatorAritmetico }
 *   FatorAritmetico     -> NumInt | NumReal | Var | '(' ExpressaoAritmetica ')'
 *
 *   ExpressaoRelacional -> TermoRelacional { OperadorBooleano TermoRelacional }
 *   TermoRelacional     -> ExpressaoAritmetica OpRel ExpressaoAritmetica
 *                        | '(' ExpressaoRelacional ')'
 *   OperadorBooleano    -> E | OU
 *
 *   ListaComandos       -> Comando { Comando }
 *   Comando             -> ComandoAtribuicao | ComandoEntrada | ComandoSaida
 *                        | ComandoCondicao | ComandoRepeticao | SubAlgoritmo
 *   ComandoAtribuicao   -> Var ':=' ExpressaoAritmetica
 *   ComandoEntrada      -> LER Var
 *   ComandoSaida        -> IMPRIMIR Var | IMPRIMIR Cadeia
 *   ComandoCondicao     -> SE ExpressaoRelacional ENTAO Comando [ SENAO Comando ]
 *   ComandoRepeticao    -> ENQTO ExpressaoRelacional [ ENTAO ] Comando
 *   SubAlgoritmo        -> INI ListaComandos FIM
 *
 * Em TermoRelacional, um '(' pode abrir tanto "(a + 1) > 2" (expressao
 * aritmetica) quanto "(a > 1 E b < 2)" (expressao relacional). Para decidir
 * sem backtracking, o conteudo do parentese e analisado por termoMisto(),
 * que devolve se o que foi lido era relacional ou apenas aritmetico.
 *
 * Recuperacao de erros (modo panico): ao encontrar um erro, ele e reportado,
 * o restante da linha e descartado e os tokens seguintes sao ignorados ate o
 * inicio do proximo comando/declaracao, para que a analise continue ate o
 * fim do arquivo.
 */

#include <iostream>
#include <string>

#include "Lexico.h"
#include "Token.h"

using namespace std;

class ErroSintatico {};

class Sintatico {
private:
    Lexico &lex;
    Token *atual;
    int erros;
    int ultimaLinha; // linha do ultimo token consumido

    // ------------------------------------------------------------ utilitarios

    void avancar() {
        if (atual->tipo == "EOF") {
            return;
        }
        ultimaLinha = atual->linha;
        delete atual;
        atual = lex.proximoToken();
    }

    bool eh(const string &tipo) const {
        return atual->tipo == tipo;
    }

    // Reporta o erro sem interromper a analise. Se o token atual ja e um
    // erro lexico, o lexico ja mostrou a mensagem e nao repetimos.
    void reportar(const string &esperado) {
        if (eh("ErroLexico")) {
            return;
        }
        erros++;
        cerr << "Erro sintatico na linha " << atual->linha << ": esperado "
             << esperado << " antes de " << atual->tipo;
        if (!eh("EOF")) {
            cerr << " \"" << atual->lexema << "\"";
        }
        cerr << endl;
    }

    // Reporta e desvia o fluxo para o ponto de recuperacao mais proximo.
    void erro(const string &esperado) {
        reportar(esperado);
        throw ErroSintatico();
    }

    void consumir(const string &tipo, const string &descricao) {
        if (!eh(tipo)) {
            erro(descricao);
        }
        avancar();
    }

    bool inicioComando() const {
        return eh("Var") || eh("PCLer") || eh("PCImprimir") || eh("PCSe") ||
               eh("PCEnqto") || eh("PCIni");
    }

    bool operadorRelacional() const {
        return eh("OpRelMenor") || eh("OpRelMenorIgual") || eh("OpRelMaior") ||
               eh("OpRelMaiorIgual") || eh("OpRelIgual") || eh("OpRelDif");
    }

    bool operadorBooleano() const {
        return eh("OpBoolE") || eh("OpBoolOu");
    }

    // Descarta o restante da linha onde o erro ocorreu, para nao confundir
    // o meio de uma expressao (ex.: "x = y - 1") com o inicio de um comando.
    void descartarLinhaDoErro() {
        while (!eh("EOF") && !eh("PCFim") && !eh("PCSenao") &&
               atual->linha == ultimaLinha) {
            avancar();
        }
    }

    void sincronizarComando() {
        descartarLinhaDoErro();
        while (!eh("EOF") && !inicioComando() && !eh("PCFim") && !eh("PCSenao")) {
            avancar();
        }
    }

    void sincronizarDeclaracao() {
        descartarLinhaDoErro();
        while (!eh("EOF") && !eh("Var") && !eh("Delim") && !inicioComando()) {
            avancar();
        }
    }

    // --------------------------------------------------------------- programa

    // ':' DEC  ou  ':' PROG
    void secao(const string &palavraChave, const string &nome) {
        try {
            consumir("Delim", "delimitador ':'");
            consumir(palavraChave, "palavra-chave " + nome);
        } catch (ErroSintatico &) {
            while (!eh("EOF") && !eh(palavraChave) && !inicioComando()) {
                avancar();
            }
            if (eh(palavraChave)) {
                avancar();
            }
        }
    }

    void programa() {
        secao("PCDec", "DEC");
        listaDeclaracoes();
        secao("PCProg", "PROG");

        if (eh("EOF")) {
            reportar("ao menos um comando");
            return;
        }

        // ListaComandos do programa principal: continua ate o fim do arquivo,
        // mesmo que apareca algum token que nao inicia comando.
        while (!eh("EOF")) {
            if (inicioComando()) {
                try {
                    comando();
                } catch (ErroSintatico &) {
                    sincronizarComando();
                }
            } else {
                reportar("comando");
                avancar();
                sincronizarComando();
            }
        }
    }

    // ----------------------------------------------------------- declaracoes

    void listaDeclaracoes() {
        do {
            try {
                declaracao();
            } catch (ErroSintatico &) {
                sincronizarDeclaracao();
            }
        } while (eh("Var"));
    }

    void declaracao() {
        consumir("Var", "declaracao de variavel");
        consumir("Delim", "delimitador ':'");
        tipoVar();
    }

    void tipoVar() {
        if (eh("PCInt") || eh("PCReal")) {
            avancar();
        } else {
            erro("tipo INT ou REAL");
        }
    }

    // ------------------------------------------------- expressoes aritmeticas

    void expressaoAritmetica() {
        termoAritmetico();
        restoExpressaoAritmetica();
    }

    void restoExpressaoAritmetica() {
        while (eh("OpAritSoma") || eh("OpAritSub")) {
            avancar();
            termoAritmetico();
        }
    }

    void termoAritmetico() {
        fatorAritmetico();
        restoTermoAritmetico();
    }

    void restoTermoAritmetico() {
        while (eh("OpAritMult") || eh("OpAritDiv")) {
            avancar();
            fatorAritmetico();
        }
    }

    void fatorAritmetico() {
        if (eh("NumInt") || eh("NumReal") || eh("Var")) {
            avancar();
        } else if (eh("AbrePar")) {
            avancar();
            expressaoAritmetica();
            consumir("FechaPar", "')'");
        } else {
            erro("numero, variavel ou '('");
        }
    }

    // ---------------------------------------------------- expressoes relacionais

    void expressaoRelacional() {
        termoRelacional();
        while (operadorBooleano()) {
            avancar();
            termoRelacional();
        }
    }

    void termoRelacional() {
        if (!termoMisto()) {
            erro("operador relacional");
        }
    }

    // Le um TermoRelacional ou apenas uma ExpressaoAritmetica.
    // Retorna true se o que foi lido e relacional.
    bool termoMisto() {
        if (eh("AbrePar")) {
            avancar();
            bool relacional = expressaoMista();
            consumir("FechaPar", "')'");
            if (relacional) {
                return true; // '(' ExpressaoRelacional ')'
            }
            // Era um fator aritmetico entre parenteses: continua a expressao.
            restoTermoAritmetico();
            restoExpressaoAritmetica();
        } else {
            expressaoAritmetica();
        }

        if (!operadorRelacional()) {
            return false;
        }
        avancar();
        expressaoAritmetica();
        return true;
    }

    // Conteudo de um parentese: ExpressaoRelacional ou ExpressaoAritmetica.
    bool expressaoMista() {
        if (!termoMisto()) {
            return false;
        }
        while (operadorBooleano()) {
            avancar();
            termoRelacional();
        }
        return true;
    }

    // -------------------------------------------------------------- comandos

    void listaComandos() {
        if (!inicioComando()) {
            erro("comando");
        }
        while (inicioComando()) {
            try {
                comando();
            } catch (ErroSintatico &) {
                sincronizarComando();
            }
        }
    }

    void comando() {
        if (eh("Var")) {
            comandoAtribuicao();
        } else if (eh("PCLer")) {
            comandoEntrada();
        } else if (eh("PCImprimir")) {
            comandoSaida();
        } else if (eh("PCSe")) {
            comandoCondicao();
        } else if (eh("PCEnqto")) {
            comandoRepeticao();
        } else if (eh("PCIni")) {
            subAlgoritmo();
        } else {
            erro("comando");
        }
    }

    void comandoAtribuicao() {
        consumir("Var", "variavel");
        consumir("Atrib", "atribuicao ':='");
        expressaoAritmetica();
    }

    void comandoEntrada() {
        consumir("PCLer", "palavra-chave LER");
        consumir("Var", "variavel");
    }

    void comandoSaida() {
        consumir("PCImprimir", "palavra-chave IMPRIMIR");
        if (eh("Var") || eh("Cadeia")) {
            avancar();
        } else {
            erro("variavel ou cadeia");
        }
    }

    void comandoCondicao() {
        consumir("PCSe", "palavra-chave SE");
        expressaoRelacional();
        consumir("PCEntao", "palavra-chave ENTAO");
        comando();
        if (eh("PCSenao")) {
            avancar();
            comando();
        }
    }

    void comandoRepeticao() {
        consumir("PCEnqto", "palavra-chave ENQTO");
        expressaoRelacional();
        if (eh("PCEntao")) { // a gramatica pede ENTAO; o exemplo do professor omite
            avancar();
        }
        comando();
    }

    void subAlgoritmo() {
        consumir("PCIni", "palavra-chave INI");
        listaComandos();
        consumir("PCFim", "palavra-chave FIM");
    }

public:
    Sintatico(Lexico &lex) : lex(lex), atual(nullptr), erros(0), ultimaLinha(1) {}

    ~Sintatico() {
        delete atual;
    }

    // Executa a analise e retorna a quantidade de erros sintaticos.
    int analisar() {
        atual = lex.proximoToken();
        programa();
        return erros;
    }
};

#endif
