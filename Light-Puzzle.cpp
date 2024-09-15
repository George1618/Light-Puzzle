/* Projeto feito por George Harrison de Almeida Mendes, para a disciplina de Computação de Alto Desempenho
do curso de Ciência da Computação na Universidade Federal do Ceará */

#include <iostream>
#include <string>
#include <fstream>
#include <math.h>
#include <limits.h>
#include <chrono>

#define FOUND 1
#define NOTFOUND 2
#define MEMORYLIMIT 3

typedef struct Node {
    int* state;
    int level;
    int move;
    struct Node* parent;
    struct Node* next;
} Node;

int main(int argc, char* argv[])
{
    int length = 0; // número de linhas (e de colunas) do puzzle

    // lê o arquivo para saber o tamanho de cada linha/coluna, e então calcular o número total
    std::cout << "Iniciando leitura rapida do arquivo do puzzle..." << std::endl;
    char* filename = argv[1];
    std::ifstream prepuzzle(filename, std::ios::in); // arquivo que tem o puzzle em seu estado inicial

    std::string line;
    while (std::getline(prepuzzle, line)) { if (line.size() > 0) length++; } // conta o número de linhas não-vazias
    prepuzzle.close();

    int size = length * length; // tamanho total do puzzle (número de valores + 1)
    if (length <= 2) { // o programa só aceita puzzle de tamanho 3x3 em diante
        if (length == 0) {
            std::cout << "Puzzle esta vazio. Verifique se o arquivo esta vazio, ou se o caminho esta correto." << std::endl;
        }
        else { std::cout << "Puzzle " << length << "x" << length << " nao tem tamanho significante..." << std::endl; }
        return 0;
    }

    static const int L = length, S = size, DSL = S - L, LL = L - 1; // alias constantes para cálculos
    static const int UP = -L, DOWN = L, LEFT = -1, RIGHT = 1; // tamanho dos movimentos
    Node* root = new Node{ new int[size] {0}, 0, 0, NULL, NULL }; // cria o estado inicial

    // lê o estado inicial número por número e salva no nó raíz
    std::cout << "Iniciando leitura do arquivo do puzzle..." << std::endl;
    std::ifstream puzzle(filename, std::ios::in); // reabre o arquivo, dessa vez para pegar todo o estado
    bool invalid = false;
    char digit;
    std::string num = "";
    int value = 0, ppos = 0, antval = 0;
    int blankPos = 0; // salva a posição do espaço em branco
    int firstLB = 0; // salva a heurística do estado inicial para servir de Lower Bound inicial
    while (puzzle.get(digit) && !invalid) { // lê cada número do arquivo e salva em cada posição ppos de 0 a size-1
        if (digit != ',' && digit != '\n') { num += digit; } // acumula cada caractere até um dos delimitadores
        else {
            if (num == " ") {
                value = S;  // guarda o espaço em branco como inteiro, sendo o maior valor possível + 1
                blankPos = ppos;
            }
            else {
                try {
                    value = std::stoi(num);  // guarda o número como o inteiro que ele é; se não, um erro será lançado
                    if (value <= 0 || value >= S) { throw ""; } // lança erro se o número não estiver no intervalo [1,size-1]
                    for (int i = 0; i < ppos; i++) { // lança erro se tiver número repetido
                        if (root->state[ppos] == value) { throw ""; }
                    }
                    // calcula a distância de Manhattan da peça atual 
                    antval = value - 1;
                    firstLB += abs((antval % L) - (ppos % L)) + abs((antval / L) - (ppos / L));
                }
                catch (const std::exception&) {
                    invalid = true;
                    std::cout << "Erro ao ler estado do puzzle. Corrija o arquivo de input." << std::endl;
                }
            }
            root->state[ppos] = value; // salva no estado inicial root
            num = ""; // limpa a string de acumulação de caractere para a próxima iteração
            if (ppos < S - 1) ppos++; // itera, desde que não passe do limite de size-1
        }
    }
    if (invalid) return -1;

    // algoritmo BnB
    std::cout << "Iniciando algoritmo..." << std::endl;
    Node* current = root; // a cada visita o nó atual mudará
    Node* best = NULL; // folha do melhor caminho
    int* parentState = root->state; // usado para que o root também tenha um estado referenciável sem criar loop

    int UB = INT_MAX, LB = firstLB; // limites superior (Upper Bound) e inferior (Lower Bound)
    int RESULT = 0; // guarda um dos casos de solução (definidos nas macros no início)
    int count = 0, limit = 32 * 1024;

    for (int i = 0; i < S; i++) {
        std::cout << parentState[i] << ",";
        if (i % L == LL) std::cout << "\n";
        if (parentState[i] < 0 || parentState[i] > S) return -1;
    }

    const auto start = std::chrono::steady_clock::now();
    do { // laço de visita
        if (count % 1024 == 0) std::cout << "KUnits: " << count / 1024 << std::endl;
        if (current == NULL) {  // pai e irmãos não-nulos sempre são visitados antes (SERIAL), então não há mais como encontrar solução
            if (best == NULL) RESULT = NOTFOUND;
            else RESULT = FOUND; // caso não tenha fechado LB=UB mas acabado de visitar todos os nós
        }
        else { // Nó existe e não foi visitado
            int distance = 0; // heurística: distância de Manhattan (e indicador de solução)
            int currBlank = 0, j, val;
            while (parentState[currBlank] != S) { currBlank++; } // encontra posição do branco O(n2)
            int moveBlank = currBlank + current->move; // nova posição do branco pelo movimento dado

            // Atualiza o estado atual fazendo o movimento E calcula a heurística O(n2)
            for (int i = 0; i < S; i++) { // copia o estado pai já fazendo o movimento das duas peças para o estado filho
                if (i == currBlank) { j = moveBlank; } // troca branco por aquele que está abaixo
                else if (i == moveBlank) { j = currBlank; } // troca o que está abaixo pelo branco
                else { j = i; }
                current->state[i] = parentState[j]; // faz a cópia com a movimentação
                // adiciona à heurística a distância de peça atual
                if (i != moveBlank) { // se não é o espaço em branco
                    val = current->state[i] - 1; // enumera de 0 a (n2-1)-1 para o módulo ser de 0 a length-1
                    distance += abs((val % L) - (i % L)) + abs((val / L) - (i / L));
                }
            }
            std::cout << distance << std::endl;
            if (distance == 0) {// o estado está organizado: uma solução foi encontrada
                if (best == NULL || current->level < best->level) { // o atual é o melhor até agora ou é melhor que o de antes
                    best = current;
                    UB = best->level; // uma solução melhor é a que tiver nível menor que o atual, sendo este, portanto, um UB.
                    if (UB == LB) { // esta É uma solução ótima; não é necessário procurar outra
                        RESULT = FOUND;
                    }
                }
            }
            else if (current->level + distance < UB) { // estado atual tem filhos promissores
                int nextLevel = current->level + 1;
                // criação dos filhos, verificando quais ramificações são possíveis baseando-se nos movimentos legais
                if (moveBlank < DSL && current->move != UP) { // pode ser movida para BAIXO
                    try {
                        current->next = new Node{ new int[S], nextLevel, DOWN, current, current->next };
                    }
                    catch (const std::exception&) {
                        RESULT = MEMORYLIMIT; break;
                    }

                }
                if ((moveBlank % L) != LL && current->move != LEFT) { // pode ser movida para a DIREITA
                    try {
                        current->next = new Node{ new int[S], nextLevel, RIGHT, current, current->next };
                    }
                    catch (const std::exception&) {
                        RESULT = MEMORYLIMIT; break;
                    }
                }
                if (moveBlank >= L && current->move != DOWN) { // pode ser movida para CIMA
                    try {
                        current->next = new Node{ new int[S], nextLevel, UP, current, current->next };
                    }
                    catch (const std::exception&) {
                        RESULT = MEMORYLIMIT; break;
                    }
                }
                if ((moveBlank % S) != 0 && current->move != RIGHT) { // pode ser movida para a ESQUERDA
                    try {
                        current->next = new Node{ new int[S], nextLevel, LEFT, current, current->next };
                    }
                    catch (const std::exception&) {
                        RESULT = MEMORYLIMIT; break;
                    }
                }
                // vai para o próximo nó
                current = current->next; parentState = current->parent->state;
            }
            else { // (level+distance>=UB): estado atual não é solução ótima nem tem filhos melhores o maior número de passos já obtido
                current = current->next; parentState = current->parent->state; // então vai direto ao próximo nó
            }
        }
    } while (RESULT == 0 && count++ <= limit);
    const auto end = std::chrono::steady_clock::now();
    const auto time = end - start;
    std::cout << "Fim do algoritmo! (" << time.count() << " nanossegundos)" << std::endl; // fim do BnB

    if (RESULT == MEMORYLIMIT) return 1;

    // salva a execução do algoritmo em um arquivo
    std::cout << "Salvando resultados do algoritmo no arquivo de saida..." << std::endl;
    if (argc >= 3) { // se um arquivo de saída for fornecido
        char* outputname = argv[2];
        std::ofstream puzzleRes(outputname, std::ofstream::trunc); // modo de leitura: limpa e escreve

        std::string result;  // salva o tipo de resultado na primeira linha
        switch (RESULT) {
        case FOUND: result = "Solução encontrada"; break;
        case NOTFOUND: result = "Solução não encontrada"; break;
        case MEMORYLIMIT: result = "Não foi possível encontrar solução antes da memória esgotar"; break;
        default: result = "Resultado inesperado";  break;
        }
        puzzleRes << result << "\n";
        puzzleRes << "LB: " << LB << " ; UB: " << UB << "\n";

        // repete a escrita para a árvore inteira, a partir da raíz
        Node* node = root;
        while (node != NULL) {
            int resValue; // guarda cada número do puzzle em cada iteração
            for (int i = 0; i < length; i++) {
                int row = i * length;
                for (int j = 0; j < length; j++) {
                    resValue = node->state[row + j];
                    if (resValue == size) { puzzleRes << "[ ]"; } // imprime o espaço vazio (representado por size)
                    else { puzzleRes << "[" << resValue << "]"; } // imprime o número da posição atual
                }
                puzzleRes << "\n";
            }
            puzzleRes << &*node;
            if (&*node == best) puzzleRes << "*";
            if (&*node == current) puzzleRes << "!!";
            puzzleRes << "\n" << node->level << "\n";
            if (node->move == 0) puzzleRes << "START";
            else if (node->move == L) puzzleRes << "DOWN";
            else if (node->move == 1) puzzleRes << "RIGHT";
            else if (node->move == -L) puzzleRes << "UP";
            else if (node->move == -1) puzzleRes << "LEFT";
            puzzleRes << "\n" << node->parent;
            puzzleRes << "\n" << node->next << "\n";
            node = node->next;
        }
        // fecha o arquivo de saída
        puzzleRes.close();
        std::cout << "Fim da escrita no arquivo de saida." << std::endl;
    }

    std::cout << "Limpando memoria..." << std::endl;
    // fecha o arquivo de entrada
    puzzle.close();
    // limpa as alocações de memória feitas para a árvore de estados
    Node* temp = NULL, * deleting = root;
    while (deleting != NULL) { // navega o encadeamento da árvore
        temp = deleting;
        deleting = deleting->next; // temp = atual, deleting = próximo, para então apagar o atual (temp) sem perder quem é o próximo
        delete[] temp->state; // única alocação própria da estrutura é o vetor do estado 
        delete temp; // nós também são alocados com new
    }

    std::cout << "Fim do programa." << std::endl;
    return 0;
}
