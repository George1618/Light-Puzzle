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

void printChild(Node* current) {
    std::cout << current;
    Node* child = current->next; int childc = 0;
    while (child != NULL && childc < 10){ std::cout << " -> " << child; child = child->next; childc++; };
    if (child == NULL) std::cout << "-> .";
    std::cout << "\n" << std::endl;
}

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

            current->state = new int[S]; // como não foi inicializado pelo pai, inicializa aqui
            // Copia o estado atual do pai, fazendo o movimento e já calculando a heurística O(n2)
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
                std::cout << "((" << count << ")) "; printChild(current);
                int nextLevel = current->level + 1;
                // criação dos filhos, verificando quais ramificações são possíveis baseando-se nos movimentos legais
                int distance1 = -1, distance2 = -1, distance3 = -1, distance4 = -1, distanceAux;
                int nextBlank, nextValue, delta; // calcula a heurística para ser usada na ordenação dos filhos (OTIMIZAÇÃO)
                if (moveBlank < DSL && current->move != UP) { // pode ser movida para BAIXO
                    try {
                        current->next = new Node{ NULL, nextLevel, DOWN, current, current->next };

                        nextBlank = moveBlank + DOWN;
                        nextValue = current->state[nextBlank] - 1;
                        // diferença das distâncias de linhas na mudança (antes de mover - depois de mover)
                        delta = abs((nextBlank / length) - (nextValue / length)) - abs((moveBlank / length) - (nextValue / length));
                        // com delta < 0, a distância aumentou, logo -delta/abs(delta)=-(-1)=+1; com > 0, a variação fica -(+1) = -1.
                        distance1 = distance - (delta / abs(delta));

                        std::cout << "(DOWN) ";  printChild(current);
                    }
                    catch (const std::exception&) {
                        RESULT = MEMORYLIMIT; break;
                    }
                }
                if ((moveBlank % L) != LL && current->move != LEFT) { // pode ser movida para a DIREITA
                    try {
                        current->next = new Node{ NULL, nextLevel, RIGHT, current, current->next };

                        nextBlank = moveBlank + RIGHT;
                        nextValue = current->state[nextBlank] - 1;
                        // diferença das distâncias de colunas na mudança (antes de mover - depois de mover)
                        delta = abs((nextBlank % length) - (nextValue % length)) - abs((moveBlank % length) - (nextValue % length));
                        distance2 = distance - (delta / abs(delta));

                        // primeira possível decisão de ordem (máx. 2 filhos)
                        if (distance1 != -1 && distance1 < distance2) { // tem os 2 filhos (BAIXO e DIREITA)
                            Node* CRIGHT = current->next, * CDOWN = current->next->next;
                            CRIGHT->next = CDOWN->next; CDOWN->next = CRIGHT; current->next = CDOWN; // troca para -> BX. -> DIR.
                        }
                        else { // não tem outro filho ou o atual DIREITA é o melhor (já está na ordem DIR. -> BX.); troca as dists.
                            distanceAux = distance1; distance1 = distance2; distance2 = distanceAux;
                        }

                        std::cout << "(RIGHT) ";  printChild(current);
                    }
                    catch (const std::exception&) {
                        RESULT = MEMORYLIMIT; break;
                    }
                }
                if (moveBlank >= L && current->move != DOWN) { // pode ser movida para CIMA
                    try {
                        current->next = new Node{ NULL, nextLevel, UP, current, current->next };

                        nextBlank = moveBlank + UP;
                        nextValue = current->state[nextBlank] - 1;
                        // diferença das distâncias de linhas na mudança (antes de mover - depois de mover)
                        delta = abs((nextBlank / length) - (nextValue / length)) - abs((moveBlank / length) - (nextValue / length));
                        distance3 = distance - (delta / abs(delta));

                        // segunda possível decisão de ordem (máx. 3 filhos)
                        if (distance2 != -1) { // tem os 3 filhos (CIMA, BAIXO e DIREITA)
                            Node* CUP = current->next, * CPREV1 = CUP->next, * CPREV2 = CPREV1->next;
                            // ordena os três sem precisar saber quem é cada anterior
                            if (distance2 < distance3) { // CIMA tem que virar o último
                                CUP->next = CPREV2->next; CPREV2->next = CUP; current->next = CPREV1;
                            }
                            else if (distance1 < distance3) { // CIMA tem que estar no meio
                                CUP->next = CPREV2; CPREV1->next = CUP; current->next = CPREV1;
                                distanceAux = distance2; distance2 = distance3; distance3 = distanceAux; // mantém 2 < 3
                            }
                            else { // PREV1 < PREV2 garantido antes, então CIMA < ANT1 < ANT2 (como já está); troca as distâncias
                                distanceAux = distance2; distance2 = distance3; distance3 = distanceAux;
                                distanceAux = distance1; distance1 = distance2; distance1 = distanceAux;
                            }
                        }
                        else if (distance1 != -1) { // tem este e exatamente 1 dos anteriores (ANTERIOR = BAIXO ou DIREITA)
                            Node* CUP = current->next, * CPREV = CUP->next;
                            distance2 = distance3; distance3 = -1; // move para a segunda distância
                            // a distância já é trocada para estar no distance1
                            if (distance1 < distance2) { // ANTERIOR é melhor; troca para -> ANT. -> CIMA
                                CUP->next = CPREV->next; CPREV->next = CUP; current->next = CPREV;
                            }
                            else { // ANTERIOR não é melhor; troca as distâncias
                                distanceAux = distance1; distance1 = distance2; distance2 = distanceAux;
                            }
                        }
                        else { // não tem outros filhos, só precisa mover para primeira distância
                            distance1 = distance3; distance3 = -1;
                        }

                        std::cout << "(UP) ";  printChild(current);
                    }
                    catch (const std::exception&) {
                        RESULT = MEMORYLIMIT; break;
                    }
                }
                if ((moveBlank % S) != 0 && current->move != RIGHT) { // pode ser movida para a ESQUERDA
                    try {
                        current->next = new Node{ NULL, nextLevel, LEFT, current, current->next };

                        nextBlank = moveBlank + LEFT;
                        nextValue = current->state[nextBlank] - 1;
                        // diferença das distâncias de colunas na mudança (antes de mover - depois de mover)
                        delta = abs((nextBlank % length) - (nextValue % length)) - abs((moveBlank % length) - (nextValue % length));
                        distance4 = distance - (delta / abs(delta));

                        //  terceira possível decisão de ordem (máx. 4 filhos, e apenas 3 depois que há movimento contrário cortado)
                        if (distance3 != -1) { // os quatro (este e todos os 3 acima) existem (isso ocorre apenas na raíz)
                            Node* CLEFT = current->next, * CPREV1 = CLEFT->next, * CPREV2 = CPREV1->next, * CPREV3 = CPREV2->next;
                            if (distance3 < distance4) { // quarto pior
                                CLEFT->next = CPREV3->next; CPREV3->next = CLEFT; current->next = CPREV1;
                            }
                            else if (distance2 < distance4) { // terceiro pior
                                CLEFT->next = CPREV3; CPREV2->next = CLEFT; current->next = CPREV1;
                            }
                            else if (distance1 < distance4) { // segundo pior
                                CLEFT->next = CPREV2; CPREV1->next = CLEFT; current->next = CPREV1;
                            }
                        }
                        else if (distance2 != -1) { // três filhos existem (este e outros 2, não importando quais são)
                            Node* CLEFT = current->next, * CPREV1 = CLEFT->next, * CPREV2 = CPREV1->next;
                            if (distance2 < distance4) {
                                CLEFT->next = CPREV2->next; CPREV2->next = CLEFT; current->next = CPREV1;
                            }
                            else if (distance1 < distance4) {
                                CLEFT->next = CPREV2; CPREV1->next = CLEFT; current->next = CPREV1;
                            }
                            // else: mantém como está; não troca as distâncias pois não há mais cálculo com elas
                        }
                        else if (distance1 != -1) { // dois filhos existem (este e algum outro)
                            Node* CLEFT = current->next, * CPREV = CLEFT->next;
                            if (distance1 < distance4) { // ANTERIOR é melhor; troca para -> ANT. -> CIMA
                                CLEFT->next = CPREV->next; CPREV->next = CLEFT; current->next = CPREV;
                            }
                            // else ANTERIOR não é melhor; não precisa trocar as distâncias porque não são mais usadas
                        }
                        // else: filho único; não troca porque não há mais cálculo com as distâncias

                        std::cout << "(LEFT) ";  printChild(current);
                    }
                    catch (const std::exception&) {
                        RESULT = MEMORYLIMIT; break;
                    }
                }
                // vai para o próximo nó
                current = current->next; parentState = current->parent->state;
            }
            else { // (level+distance>=UB): estado atual não é solução ótima nem tem filhos melhores 
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
            if (node->state != NULL) for (int i = 0; i < L; i++) {
                int row = i * L;
                for (int j = 0; j < L; j++) {
                    resValue = node->state[row + j];
                    if (resValue == size) { puzzleRes << " "; } // imprime o espaço vazio (representado por size)
                    else { puzzleRes << resValue; } // imprime o número da posição atual
                    if (j < LL) puzzleRes << ",";
                }
                puzzleRes << "\n";
            }
            else { // o puzzle filho não foi instanciado porque nem precisou
                puzzleRes << "[NULL]\n";
            }
            puzzleRes << node;
            if (node == best) puzzleRes << "*";
            if (node == current) puzzleRes << "!!";
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
