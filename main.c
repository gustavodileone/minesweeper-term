#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// Definitivamente não vai sobrescrever um arquivo com esse nome, né?
#define LEADERBOARD_FILE "./.leaderboard"

#define CHAR_MINE '*'
#define CHAR_FLAG '!'
#define CHAR_UNKNOWN '?'
#define CHAR_EMPTY ' '

#define ANSI_WHITE "\e[0;37m"
#define ANSI_RED "\033[91m"
#define ANSI_YELLOW "\e[0;93m"
#define ANSI_BLUE "\e[0;94m"
#define ANSI_GREEN "\e[0;92m"

#define clear_screen() printf("\e[1;1H\e[2J")

/**
 * slp - API padrão para função `sleep` windows/linux
 * 
 * @ms: Número de milisegundos para dormir
 * Return: void
 */
void slp(unsigned int ms) {
    #ifdef _WIN32
        Sleep(ms);
    #else
        usleep(ms * 1000);
    #endif
}

/**
 * Bitwise flags que cada célula pode possuir
 */
typedef enum minesweeper_flags {
    HAS_MINE = 1,
    HAS_FLAG = 2,
    IS_KNOWN = 4,
    //_       = 8,
    //__       = 16,
    //___       = 32,
    //____       = 64,
    //_____       = 128,
} minesweeper_flags;

/**
 * Instância das células do jogo.
 */
typedef struct minesweeper_slot {
    unsigned char mines_around;
    unsigned char flags;
    unsigned char row;
    unsigned char col;
} minesweeper_slot;

unsigned char minesweeper_slot_has_flag(minesweeper_slot* slot, minesweeper_flags flag) {
    return (slot->flags & flag) == flag;
}

void minesweeper_slot_set_flag(minesweeper_slot* slot, minesweeper_flags flag) {
    slot->flags = slot->flags | flag;
}

void minesweeper_rm_flag(minesweeper_slot* slot, minesweeper_flags flag) {
    slot->flags = slot->flags & ~flag;
}

/**
 * Possíveis status para o jogo.
 */
typedef enum minesweeper_status {
    VICTORY,
    DEFEAT,
    RUNNING,
} minesweeper_status;

/**
 * Dificuldade do jogo, 5x5 ou 8x8
 */
typedef enum minesweeper_difficulty {
    EASY = 5,
    MEDIUM = 8
} minesweeper_difficulty;

/**
 * A instância do jogo.
 */
typedef struct minesweeper {
    minesweeper_slot **map;
    unsigned char length;
    minesweeper_status status;
    minesweeper_difficulty difficulty;

    time_t timestamp;
    unsigned char remaining_slots;
    unsigned char quantity_mines;
} minesweeper;

/**
 * Placar do jogo
 */
typedef struct leaderboard {
    time_t time;
    minesweeper_difficulty difficulty;
    minesweeper_status status;
    unsigned char remaining_slots;
} leaderboard;

/**
 * minesweeper_difficulty_string - Converte enum em string constante.
 * 
 * @diff: enum de dificuldade do jogo
 * Return: String constante ou nulo caso a dificuldade seja inválida.
 */
char* minesweeper_difficulty_string(minesweeper_difficulty diff) {
    switch(diff) {
        case EASY:
            return "Fácil";
        break;
        case MEDIUM:
            return "Médio";
        break;
    }

    return NULL;
}

/**
 * minesweeper_status_string - Converte enum em string constante.
 * 
 * @status: enum de status do jogo
 * Return: String constante ou nulo caso o status seja inválido.
 */
char* minesweeper_status_string(minesweeper_status status) {
    switch(status) {
        case VICTORY:
            return "Vitória";
        break;
        case DEFEAT:
            return "Derrota";
        break;
        case RUNNING:
            return "Em jogo";
        break;
    }

    return NULL;
}

/**
 * update_minesweeper_slot_mines_around - Atualiza os números ao redor de uma célula-bomba.
 * 
 * @game: instância do jogo
 * @slot: célula-bomba
 * Return: void
 */
void update_minesweeper_slot_mines_around(minesweeper* game, minesweeper_slot* slot) {
    for(int i = -1; i <= 1; i++) {
        for(int j = -1; j <= 1; j++) {
            int row = slot->row + i;
            int col = slot->col + j;

            if(row < 0 || row >= game->length || col < 0 || col >= game->length)
                continue;

            if(i == 0 && j == 0)
                continue;

            game->map[row][col].mines_around++;
        }
    }
}

/**
 * initiate_minesweeper_map - Cria e inicializa a matriz mapa do jogo.
 * 
 * @game: instância do jogo
 * Return: void
 */
void initiate_minesweeper_map(minesweeper* game) {
    unsigned char qtd_mines = (game->length * game->length) / 4;
    game->quantity_mines = qtd_mines;
    game->remaining_slots = (game->length * game->length) - qtd_mines;

    for(int i = 0; i < game->length; i++) {
        for(int j = 0; j < game->length; j++) {
            game->map[i][j] = (struct minesweeper_slot){0};

            game->map[i][j].row = i;
            game->map[i][j].col = j;
        }
    }

    while(qtd_mines) {
        unsigned char row = rand() % game->length;
        unsigned char col = rand() % game->length;

        minesweeper_slot* slot = &game->map[row][col];

        if(minesweeper_slot_has_flag(slot, HAS_MINE))
            continue;

        minesweeper_slot_set_flag(slot, HAS_MINE);
        update_minesweeper_slot_mines_around(game, slot);

        qtd_mines--;
    }

    game->timestamp = time(NULL);
}

/**
 * create_minesweeper - Cria e inicializa a instância do jogo de acordo com a dificuldade.
 * 
 * @diff: dificuldade do jogo
 * Return: instância `minesweeper*` dinamicamente alocada.
 */
minesweeper* create_minesweeper(minesweeper_difficulty diff) {
    minesweeper* game = (minesweeper*) malloc(sizeof(minesweeper));
    game->difficulty = diff;
    game->status = RUNNING;
    game->length = (unsigned char) game->difficulty;

    game->map = (minesweeper_slot**) malloc(sizeof(minesweeper_slot*) * game->length);
    for(int i = 0; i < game->length; i++) {
        game->map[i] = (minesweeper_slot*) malloc(sizeof(minesweeper_slot) * game->length);
    }

    initiate_minesweeper_map(game);

    return game;
}

/**
 * print_minesweeper_map_row - Printa as linhas separadoras do mapa de acordo com o tamanho.
 * 
 * @game: Instância do jogo
 * Return: void
 */
void print_minesweeper_map_row(minesweeper* game) {
    printf("\r\n|---");

    for(int i = 0; i < game->length; i++) {
        printf("----");
    }

    printf("|");
}

/**
 * print_minesweeper_commands - Printa os comandos durante O JOGO ;)
 * 
 * Return: void
 */
void print_minesweeper_commands() {
    printf("1) Selecionar | 2) Bandeira | 3) Desistir\r\n");
}

/**
 * print_minesweeper_mines_around - Printa com coloração ANSI o número da célula
 * 
 * @slot: Célula do jogo
 * Return: void
 */
void print_minesweeper_mines_around(minesweeper_slot* slot) {
    if(slot->mines_around >= 4) {
        printf(ANSI_RED);
    } else if(slot->mines_around == 3) {
        printf(ANSI_YELLOW);
    } else if(slot->mines_around == 2) {
        printf(ANSI_BLUE);
    } else if(slot->mines_around == 1) {
        printf(ANSI_GREEN);
    }

    if(slot->mines_around == 0) {
        printf(" %c", CHAR_EMPTY);
    } else {
        printf(" %d", slot->mines_around);
    }
}

/**
 * print_minesweeper_situation - Printa a quantidade de minas e espaços vazios.
 * 
 * @game: Instância do jogo
 * Return: void
 */
void print_minesweeper_situation(minesweeper* game) {
    printf("Minas: ");
    printf(ANSI_RED);
    printf("%d", game->quantity_mines);
    printf(ANSI_WHITE);
    printf(" | Espaços livres:");
    printf(ANSI_GREEN);
    printf(" %d \r\n\r\n", game->remaining_slots);
    printf(ANSI_WHITE);
}

/**
 * print_minesweeper_map - Printa o mapa do jogo em si
 * 
 * @game: Instância do jogo
 * Return: void
 */
void print_minesweeper_map(minesweeper* game) {
    clear_screen();

    print_minesweeper_situation(game);
    print_minesweeper_commands();

    print_minesweeper_map_row(game);
    printf("\n");

    printf("|    ");
    for(int i = 0; i < game->length; i++) {
        printf(" %d |", i);
    }
    print_minesweeper_map_row(game);

    for(int i = 0; i < game->length; i++) {
        printf("\n| %d |", i);

        for(int j = 0; j < game->length; j++) {
            minesweeper_slot* slot = &game->map[i][j];

            if(minesweeper_slot_has_flag(slot, IS_KNOWN)) {
                if(minesweeper_slot_has_flag(slot, HAS_MINE)) {
                    printf(ANSI_RED);
                    printf(" %c", CHAR_MINE);
                } else {
                    print_minesweeper_mines_around(slot);
                }
            } else {
                if(minesweeper_slot_has_flag(slot, HAS_FLAG)) {
                    printf(ANSI_RED);
                    printf(" %c", CHAR_FLAG);
                } else {
                    printf(" %c", CHAR_UNKNOWN);
                }
            }

            printf(ANSI_WHITE);
            printf(" |");
        }

        printf(ANSI_WHITE);
        print_minesweeper_map_row(game);
    }

    printf("\n");
}

/**
 * destroy_minesweeper - Desaloca da memória o jogo
 * 
 * @game: Instância do jogo
 * Return: void
 */
void destroy_minesweeper(minesweeper* game) {
    for(int i = 0; i < game->length; i++) {
        free(game->map[i]);
    }

    free(game->map);
    free(game);
}

/**
 * expand_minesweep_slot - Função recursiva para expansão de células adjacentes vazias.
 * 
 * @game: Instância do jogo
 * @row: Próxima linha da célula
 * @col: Próxima coluna da célula
 * Return: void
 */
void expand_minesweep_slot(minesweeper* game, int row, int col) {
    if(row < 0 || row >= game->length || col < 0 || col >= game->length) 
        return;

    minesweeper_slot* slot = &game->map[row][col];

    if(minesweeper_slot_has_flag(slot, IS_KNOWN) || minesweeper_slot_has_flag(slot, HAS_MINE)) 
        return;

    minesweeper_slot_set_flag(slot, IS_KNOWN);
    game->remaining_slots--;

    if(slot->mines_around > 0)
        return;

    for(int i = -1; i <= 1; i++) {
        for(int j = -1; j <= 1; j++) {
            if(i == 0 && j == 0) 
                continue;

            expand_minesweep_slot(game, row + i, col + j);
        }
    }
}

/**
 * check_minesweep_slot - Verifica uma célula específica.
 * 
 * @game: Instância do jogo
 * @slot: Célula selecionada
 * Return: void
 */
void check_minesweep_slot(minesweeper* game, minesweeper_slot* slot) {
    if(minesweeper_slot_has_flag(slot, HAS_MINE)) {
        minesweeper_slot_set_flag(slot, IS_KNOWN);

        game->status = DEFEAT;
        return;
    }

    expand_minesweep_slot(game, slot->row, slot->col);
}

/**
 * reveal_minesweep - Revela o mapa inteiro do jogo.
 * 
 * @game: Instância do jogo
 * Return: void
 */
void reveal_minesweep(minesweeper* game) {
    int time = 400;

    for(int i = 0; i < game->length; i++) {
        for(int j = 0; j < game->length; j++) {
            minesweeper_slot* slot = &game->map[i][j];
            if(minesweeper_slot_has_flag(slot, IS_KNOWN))
                continue;

            minesweeper_slot_set_flag(slot, IS_KNOWN);
            clear_screen();
            print_minesweeper_map(game);
            time = time <= 100 ? 40 : time - 20;
            slp(time);
        }
    }
}

/**
 * select_minesweeper_slot - Seleciona uma linha e coluna para o jogo
 * 
 * @game: Instância do jogo
 * @row: Ponteiro para uma linha
 * @col: Ponteiro para uma coluna
 * Return: void
 */
void select_minesweeper_slot(minesweeper* game, int* row, int* col) {
    int s = 0;

    do {
        printf("Digite a linha: ");
        s = scanf("%d", row);
        if(s != 1 || (*row < 0 || *row >= game->length)) {
            getchar(); // Limpa linefeed para próximo uso do scanf

            clear_screen();
            print_minesweeper_map(game);
            printf("Decisão inválida.\r\n");
            s = 0;
            continue;
        }

        printf("Digite a coluna: ");
        s = scanf("%d", col);
        if(s != 1 || (*col < 0 || *col >= game->length)) {
            getchar(); // Limpa linefeed para próximo uso do scanf

            clear_screen();
            print_minesweeper_map(game);
            printf("Decisão inválida.\r\n");
            s = 0;
            continue;
        }
    } while(s == 0);
}

/**
 * store_minesweeper_game - Salva no placar o jogo de minesweeper
 * 
 * @game: Instância do jogo
 * Return: void
 */
void store_minesweeper_game(minesweeper* game) {
    FILE* f = fopen(LEADERBOARD_FILE, "a");
    if(!f) {
        fprintf(stderr, "Falha ao salvar o placar.\r\n");
        return;
    };

    leaderboard lead = {
        .time = game->timestamp,
        .difficulty = game->difficulty,
        .status = game->status,
        .remaining_slots = game->remaining_slots,
    };

    fwrite(&lead, sizeof(leaderboard), 1, f);
    fclose(f);
}

/**
 * run_minesweeper - Loop principal do jogo
 * 
 * @diff: Dificuldade selecionada
 * Return: void
 */
void run_minesweeper(minesweeper_difficulty diff) {
    minesweeper* game = create_minesweeper(diff);

    clear_screen();
    print_minesweeper_map(game);

    unsigned char c = 0;
    do {
        scanf(" %c", &c);
        int row, col;

        switch(c) {
            case '1':
                select_minesweeper_slot(game, &row, &col);                

                check_minesweep_slot(game, &game->map[row][col]);
                clear_screen();
                print_minesweeper_map(game);

                if(game->remaining_slots == 0)
                    game->status = VICTORY;
            break;
            case '2':
                select_minesweeper_slot(game, &row, &col);
                minesweeper_slot* slot = &game->map[row][col];

                if(!minesweeper_slot_has_flag(slot, HAS_FLAG)) {
                    minesweeper_slot_set_flag(slot, HAS_FLAG);
                } else {
                    minesweeper_rm_flag(slot, HAS_FLAG);
                }

                clear_screen();
                print_minesweeper_map(game);
            break;
            case '3':
                game->status = DEFEAT;
            break;
        }

        c = 0;
    } while(c == 0 && game->status == RUNNING);

    game->timestamp = time(NULL) - game->timestamp;

    reveal_minesweep(game);
    store_minesweeper_game(game);

    printf(
        "\r\n%s. Pressione qualquer tecla para continuar\r\n", 
        minesweeper_status_string(game->status)
    );
    scanf(" %c", &c);

    destroy_minesweeper(game);
}

/**
 * get_leaderboard - Recupera a leaderboard do arquivo.
 * 
 * @n: Ponteiro para inteiro que receberá o tamanho do array
 * Return: Array de structs com os placares.
 */
leaderboard* get_leaderboard(int* n) {
    FILE* f = fopen(LEADERBOARD_FILE, "r");
    if(!f) {
        fprintf(stderr, "Não foi possível abrir o arquivo.\r\n");
        return NULL;
    }

    leaderboard lead = {0};
    while(fread(&lead, sizeof(lead), 1, f))
        (*n)++;

    rewind(f);
    leaderboard* lboards = (leaderboard*) malloc(sizeof(leaderboard) * (*n));

    for(int i = (*n) - 1; i >= 0; i--) {
        fread(&lead, sizeof(leaderboard), 1, f);
        lboards[i] = lead;
    }

    fclose(f);
    return lboards;
}

/**
 * destroy_leaderboard - Função que desaloca o array do placar.
 */
void destroy_leaderboard(leaderboard* lead) {
    free(lead);
}

/**
 * print_menu_leaderboard - Printa o menu de placar
 * 
 * Return: void
 */
void print_menu_leaderboard(leaderboard* leads, int n) {
    for(int i = 0; i < n; i++) {
        printf("Condição: [%s]\r\nDificuldade: %s\r\nTempo de jogo: %lds\r\nEspaços restantes: %d\r\n", 
            minesweeper_status_string(leads[i].status),
            minesweeper_difficulty_string(leads[i].difficulty),
            leads[i].time,
            leads[i].remaining_slots
        );
        printf("--------------------------------\r\n");
    }
}

/**
 * menu_leaderboard - Gerencia o menu de placar
 * 
 * Return: void
 */
void menu_leaderboard() {
    clear_screen();

    int n = 0;
    leaderboard* leads = get_leaderboard(&n);
    if(!leads) {
        printf("Sem placar.\r\n");
    } else {
        print_menu_leaderboard(leads, n);
        destroy_leaderboard(leads);
    }

    printf("Digite 'c' para voltar.\r\n");

    char _;
    scanf(" %c", &_);
}

/**
 * print_menu_start_game - Printa o menu para selecionar dificuldade
 * 
 * Return: void
 */
void print_menu_start_game() {
    printf("Minesweeper - Selecione a dificuldade\r\n");
    printf("1) Fácil\r\n");
    printf("2) Médio\r\n");
    printf("3) Voltar\r\n");
}

/**
 * menu_start_game - Gerencia o menu para selecionar a dificuldade
 * 
 * Return: 0 para voltar ao menu anterior ou 1 ao ter finalizado o jogo.
 */
void menu_start_game() {
    clear_screen();
    print_menu_start_game();

    unsigned char c = 0;
    do {
        scanf(" %c", &c);

        switch(c) {
            case '1':
                run_minesweeper(EASY);
            break;
            case '2':
                run_minesweeper(MEDIUM);
            break;
            case '3':
            break;
            default:
                clear_screen();
                print_menu_start_game();
                printf("Opção inválida.\r\n");
                c = 0;
            break;
        }
    } while(c == 0);
}

/**
 * print_main_menu - Printa as opções do menu principal
 * 
 * Return: void
 */
void print_main_menu() {
    printf("Minesweeper - Menu principal\r\n");
    printf("1) Iniciar jogo\r\n");
    printf("2) Ver placar\r\n");
    printf("3) Sair do programa\r\n");
}

/**
 * menu_main - Gerencia as decisões do menu principal
 * 
 * Return: void
 */
void menu_main() {
    unsigned char c = 0;
    do {
        clear_screen();
        print_main_menu();

        scanf(" %c", &c);

        switch(c) {
            case '1':
                menu_start_game();
                c = 0;
            break;
            case '2':
                menu_leaderboard();
                c = 0;
            break;
            case '3':
            break;
            default:
                clear_screen();
                print_main_menu();
                printf("Opção inválida.\r\n");
                c = 0;
            break;
        }
    } while(c == 0);
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif
    srand(time(NULL));
    menu_main();
    return EXIT_SUCCESS;
}