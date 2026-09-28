#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <locale.h>
#include <signal.h>

/* ---------- ANSI ЦВЕТА И СТИЛИ ---------- */
#define RESET       "\033[0m"
#define BOLD        "\033[1m"
#define RED         "\033[31m"
#define GREEN       "\033[32m"
#define YELLOW      "\033[33m"
#define BLUE        "\033[34m"
#define MAGENTA     "\033[35m"
#define CYAN        "\033[36m"
#define BG_DARK     "\033[48;5;234m"

/* ---------- Termux-совместимый терминал ---------- */
static void stty_raw(void)     { system("stty -echo -icanon min 1 time 0"); }
static void stty_restore(void) { system("stty echo icanon"); }

static void cleanup(void) {
    stty_restore();
    printf("\033[?25h" RESET "\n");
    fflush(stdout);
}

static void handle_sigint(int sig) { (void)sig; cleanup(); _exit(0); }

void clear(void) { printf("\033[2J\033[H"); }
void hide(void)  { printf("\033[?25l"); }

void wait_enter(void) {
    printf(BOLD YELLOW "\n Нажмите [Enter] для продолжения..." RESET);
    fflush(stdout);
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

/* ---------- РУССКАЯ АЗБУКА МОРЗЕ ---------- */
static const char *morse_ru(unsigned int cp) {
    switch (cp) {
        case 0x0410: case 0x0430: return ".-";     /* А */
        case 0x0411: case 0x0431: return "-...";   /* Б */
        case 0x0412: case 0x0432: return ".--";    /* В */
        case 0x0413: case 0x0433: return "--.";    /* Г */
        case 0x0414: case 0x0434: return "-..";    /* Д */
        case 0x0415: case 0x0435: return ".";      /* Е */
        case 0x0416: case 0x0436: return "...-";   /* Ж */
        case 0x0417: case 0x0437: return "--..";   /* З */
        case 0x0418: case 0x0438: return "..";     /* И */
        case 0x0419: case 0x0439: return ".---";   /* Й */
        case 0x041A: case 0x043A: return "-.-";    /* К */
        case 0x041B: case 0x043B: return ".-..";   /* Л */
        case 0x041C: case 0x043C: return "--";     /* М */
        case 0x041D: case 0x043D: return "-.";     /* Н */
        case 0x041E: case 0x043E: return "---";    /* О */
        case 0x041F: case 0x043F: return ".--.";   /* П */
        case 0x0420: case 0x0440: return ".-.";    /* Р */
        case 0x0421: case 0x0441: return "...";    /* С */
        case 0x0422: case 0x0442: return "-";      /* Т */
        case 0x0423: case 0x0443: return "..-";    /* У */
        case 0x0424: case 0x0444: return "..-.";   /* Ф */
        case 0x0425: case 0x0445: return "....";   /* Х */
        case 0x0426: case 0x0446: return "-.-.";   /* Ц */
        case 0x0427: case 0x0447: return "---.";   /* Ч */
        case 0x0428: case 0x0448: return "----";   /* Ш */
        case 0x0429: case 0x0449: return "--.-";   /* Щ */
        case 0x042A: case 0x044A: return "--.--";  /* Ъ */
        case 0x042B: case 0x044B: return "-.--";   /* Ы */
        case 0x042C: case 0x044C: return "-..-";   /* Ь */
        case 0x042D: case 0x044D: return "..-..";  /* Э */
        case 0x042E: case 0x044E: return "..--";   /* Ю */
        case 0x042F: case 0x044F: return ".-.-";   /* Я */
        case 0x0451: case 0x0401: return ".";      /* ё / Ё */
        case '0': return "-----";
        case '1': return ".----";
        case '2': return "..---";
        case '3': return "...--";
        case '4': return "....-";
        case '5': return ".....";
        case '6': return "-....";
        case '7': return "--...";
        case '8': return "---..";
        case '9': return "----.";
        case '.': return ".-.-.-";
        case ',': return "--..--";
        case '?': return "..--..";
        case '!': return "-.-.--";
        case ':': return "---...";
        case ';': return "-.-.-.";
        case '-': return "-....-";
        case '/': return "-..-.";
        case '(': return "-.--.";
        case ')': return "-.--.-";
        case '\'': return ".----.";
        case '"': return ".-..-.";
        case '=': return "-...-";
        case '+': return ".-.-.";
        case '@': return ".--.-.";
        default: return NULL;
    }
}

static int utf8_decode(const char *s, unsigned int *cp) {
    unsigned char c = (unsigned char)s[0];
    if (c < 0x80) { *cp = c; return 1; }
    if ((c & 0xE0) == 0xC0 && s[1]) {
        *cp = ((c & 0x1F) << 6) | (s[1] & 0x3F); return 2;
    }
    if ((c & 0xF0) == 0xE0 && s[1] && s[2]) {
        *cp = ((c & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
        return 3;
    }
    if ((c & 0xF8) == 0xF0 && s[1] && s[2] && s[3]) {
        *cp = ((c & 0x07) << 18) | ((s[1] & 0x3F) << 12) |
              ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
        return 4;
    }
    return 0;
}

void morse_print(const char *s) {
    int first_word = 1, col = 0;
    const char *p = s;
    printf(CYAN);
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\n') p++;
        if (!*p) break;
        if (!first_word) {
            if (col + 3 > 60) { printf("\n"); col = 0; }
            printf(" / "); col += 3;
        }
        first_word = 0;
        while (*p && *p != ' ' && *p != '\t' && *p != '\n') {
            unsigned int cp = 0;
            int n = utf8_decode(p, &cp);
            if (n == 0) { p++; continue; }
            const char *code = morse_ru(cp);
            if (code) {
                int len = (int)strlen(code) + 1;
                if (col + len > 60) { printf("\n"); col = 0; }
                printf("%s ", code); col += len;
            }
            p += n;
        }
    }
    printf(RESET "\n");
}

/* ---------- ЯЗЫК ---------- */
typedef enum { LANG_EN = 0, LANG_MORSE = 1 } Lang;
static Lang LANG = LANG_EN;

void say(const char *en, const char *ru_for_morse) {
    if (LANG == LANG_EN) {
        printf(BOLD "%s\n" RESET, en);
    } else {
        morse_print(ru_for_morse ? ru_for_morse : en);
    }
}

/* ---------- АРТЫ И ГРАФИКА ---------- */
void print_header(const char* title) {
    printf(GREEN "+==========================================================+\n");
    printf("  " BOLD "%s \n", title);
    printf(GREEN "+==========================================================+\n" RESET);
}

void art_rat(void) {
    printf(MAGENTA);
    printf("            .--.       \n");
    printf("           /    \\      \n");
    printf("          | " RESET BOLD "o  o" MAGENTA " |     \n");
    printf("          |  <>  |     \n");
    printf("          |  --  |     \n");
    printf("     .---.'      '.---.\n");
    printf("    /   /          \\   \\\n");
    printf("   (   (    " RESET "RAT" MAGENTA "     )   )\n");
    printf("    \\   \\          /   /\n");
    printf("     '---'--------'---'\n");
    printf("         |  |    |  |\n" RESET);
}

void art_zombie(void) {
    printf(RED);
    printf("        _____\n");
    printf("       /     \\\n");
    printf("      | " RESET BOLD "() ()" RED " |\n");
    printf("      |   __   |\n");
    printf("      |  /  \\  |\n");
    printf("       \\_____/\n");
    printf("      /| " RESET "ZOMBIE" RED "|\\\n");
    printf("     / |       | \\\n");
    printf("       |       |\n");
    printf("      /         \\\n" RESET);
}

void art_cheese(void) {
    printf(YELLOW);
    printf("        ________\n");
    printf("       /       /|\n");
    printf("      /  o    / |\n");
    printf("     /     o /  |\n");
    printf("    /  o    /   |\n");
    printf("   /_______/____|\n" RESET);
}

void art_hole(void) {
    printf(BLUE);
    printf("      ___________\n");
    printf("     /           \\\n");
    printf("    /   .     .   \\\n");
    printf("   |  " RESET BOLD "(O)   (O)" BLUE "  |\n");
    printf("    \\             /\n");
    printf("     \\___________/\n" RESET);
}

void art_table(void) {
    printf(CYAN);
    printf("    ______________\n");
    printf("   |              |\n");
    printf("   |   " RESET YELLOW "[CHEESE]" CYAN "   |\n");
    printf("   |______________|\n");
    printf("       |      |\n");
    printf("       |      |\n" RESET);
}

void art_raven(void) {
    printf(BOLD BG_DARK);
    printf("           __    \n");
    printf("          / _ )  \n");
    printf("     .--./ /     \n");
    printf("    /    \\/      \n");
    printf("   |  o o  |     \n");
    printf("    \\  ^  /      \n");
    printf("     |||||       \n");
    printf("    /     \\      \n" RESET);
}

void art_feather(void) {
    printf(CYAN "           /\\\n          /  \\\n         /    \\\n        /______\\\n          ||||\n" RESET);
}

void art_city(void) {
    printf(BLUE "       _   _   _\n      | |_| |_| |\n      |         |\n      |  _   _  |\n      |_|_|_|_|_|\n        GOROKUR\n" RESET);
}

void art_farm(void) {
    printf(YELLOW "     ______________________\n    /  [ ABANDONED FARM ]  /|\n   /______________________/ |\n   |  ___    ___          | /\n   | |___|  |___|         |/ \n" RESET);
}

void art_roosters(void) {
    printf(RED "    / _)   / _)   / _)\n   | o o | | o o | | o o |\n    \\ ^ /   \\ ^ /   \\ ^ /\n" RESET);
}

void art_cat(void) {
    printf(BOLD MAGENTA "       /\\_/\\\n      ( o.o )\n       > ^ <   [DARK CAT]\n" RESET);
}

void art_chicken(void) {
    printf(YELLOW "        __\n       / _)\n      | o o |\n       \\ ^ /\n       |||||\n" RESET);
}

void art_chess_pawn(void) {
    printf(BOLD "        ( )\n       /   \\\n      |_____|\n      /_____\\\n" RESET);
}

void art_cheese_rot(void) {
    printf(RED "        ________\n       / X   X /|\n      /  POISON / \n     /_________/  \n" RESET);
}

void art_basement(void) {
    printf(RED "    _________________\n   |   HENHOUSE      |\n   |   BASEMENT      | [BARS]\n   |_________________|\n" RESET);
}

void print_status(int zombie_dist) {
    if (zombie_dist < 0) zombie_dist = 0;
    if (zombie_dist > 40) zombie_dist = 40;
    printf(BOLD RED "\n[ ДИСТАНЦИЯ ДО ЗОМБИ: %d метров ]\n" RESET, zombie_dist);
    printf("[");
    for (int i = 0; i < 20; i++) {
        if (i < zombie_dist / 2) printf(GREEN "=");
        else if (i == zombie_dist / 2) printf(RED "🧟");
        else printf(" ");
    }
    printf("]\n\n");
}

/* ---------- МЕНЮ ЯЗЫКА ---------- */
static int menu_language(void) {
    const char *items_en[2] = { "ENGLISH", "MORSE" };
    const char *items_mo[2] = { ".- -. --. .-.. .. ... ....", "-- --- .-. ... ." };
    int sel = 0;
    for (;;) {
        clear();
        print_header("         A S C I I   R A T   Q U E S T         ");
        printf("\n   CHOOSE LANGUAGE / ВЫБЕРИ ЯЗЫК:\n\n");
        for (int i = 0; i < 2; i++) {
            if (i == sel) {
                printf(BOLD GREEN " -> %-10s %s\n" RESET, items_en[i], items_mo[i]);
            } else {
                printf("    %-10s %s\n", items_en[i], items_mo[i]);
            }
        }
        printf(YELLOW "\n [h / l] или [<- / ->] - Выбор [Enter] - Подтвердить\n" RESET);
        fflush(stdout);

        int c = getchar();
        if (c == 27) {
            int c1 = getchar();
            if (c1 == '[' || c1 == 'O') {
                int c2 = getchar();
                if (c2 == 'D') sel = 0;
                if (c2 == 'C') sel = 1;
            }
        } else if (c == 'h') sel = 0;
        else if (c == 'l') sel = 1;
        else if (c == '\n' || c == '\r') return sel;
    }
}

/* ---------- ПРАВИЛА ---------- */
static void print_rules(void) {
    clear();
    print_header(" ПРАВИЛА / RULES ");
    if (LANG == LANG_EN) {
        printf(" You are a rat running from a zombie.\n");
        printf(" On each sign you must answer a question.\n");
        printf(" Answer by jumping:\n");
        printf(GREEN " <- short jump (left)\n" RESET);
        printf(BLUE  " -> long jump (right)\n" RESET);
        printf(" Correct answer: run further.\n");
        printf(RED   " Wrong answer: zombie gets closer.\n" RESET);
        printf(" In Termux you can also use h / l.\n");
    } else {
        morse_print("ТЫ КРЫСА БЕЖИШЬ ОТ ЗОМБИ");
        morse_print("НА КАЖДОЙ ТАБЛИЧКЕ ВОПРОС");
        morse_print("ОТВЕТ ВЫБИРАЕТСЯ ПРЫЖКОМ");
        morse_print("КОРОТКИЙ ПРЫЖОК ВЛЕВО ДЛИННЫЙ ВПРАВО");
        morse_print("ОТВЕТИШЬ ВЕРНО БЕЖИШЬ ДАЛЬШЕ");
        morse_print("ОШИБЁШЬСЯ ЗОМБИ БЛИЖЕ");
        morse_print("В ТЕРМУКСЕ МОЖНО ЖАТЬ H И L");
    }
    wait_enter();
}

/* ---------- ЗАПРОС С ВАРИАНТАМИ ---------- */
int ask(const char *q_en, const char *q_ru,
        const char *a_en, const char *a_ru,
        const char *b_en, const char *b_ru)
{
    clear();
    print_header(" ТАБЛИЧКА / SIGN ");
    if (LANG == LANG_EN) {
        printf(BOLD CYAN "\n%s\n\n" RESET, q_en);
        printf(GREEN " [ h / <- ] %s\n" RESET, a_en);
        printf(BLUE  " [ l / -> ] %s\n" RESET, b_en);
    } else {
        morse_print(q_ru);
        printf("\n");
        printf(GREEN " [ h / <- ] "); morse_print(a_ru);
        printf(BLUE  " [ l / -> ] "); morse_print(b_ru);
    }
    fflush(stdout);
    for (;;) {
        int c = getchar();
        if (c == 27) {
            int c1 = getchar();
            if (c1 == '[' || c1 == 'O') {
                int c2 = getchar();
                if (c2 == 'D') return 0;
                if (c2 == 'C') return 1;
            }
        }
        if (c == 'h') return 0;
        if (c == 'l') return 1;
    }
}

/* ---------- MAIN ---------- */
int main(void) {
    setlocale(LC_ALL, "");
    signal(SIGINT, handle_sigint);
    atexit(cleanup);
    stty_raw();
    hide();
    srand((unsigned)time(NULL));

    int lang_choice = menu_language();
    LANG = (lang_choice == 0) ? LANG_EN : LANG_MORSE;

    print_rules();

    int zombie = 20;
    int cheese = 1;

    /* ---------- ИНТРО ---------- */
    clear();
    print_header(" ASCII-RAT: ESCAPE FROM ZOMBIE ");
    art_rat();
    printf("\n");
    say("You are a rat. You are running. A zombie is behind you.",
        "ТЫ КРЫСА ТЫ БЕЖИШЬ ЗА ТОБОЙ ЗОМБИ");
    art_zombie();
    printf("\n");
    wait_enter();

    /* ---------- 1 ---------- */
    clear();
    print_status(zombie);
    art_table();
    int ok = ask("Where is the cheese?", "ГДЕ СЫР",
                 "Cheese is on the table", "СЫР НА СТОЛЕ",
                 "Cheese is in the hole", "СЫР В НОРЕ");
    if (ok == 0) {
        clear(); art_cheese();
        say("Jump onto the table. Cheese found.", "ПРЫЖОК НА СТОЛ СЫР НАЙДЕН");
        cheese = 1;
        zombie += 5;
    } else {
        clear(); art_hole();
        say("Jump missed. Zombie is closer.", "ПРЫЖОК МИМО ЗОМБИ БЛИЖЕ");
        zombie -= 5;
    }
    wait_enter();

    /* ---------- 2 ---------- */
    clear();
    print_status(zombie);
    ok = ask("You must drag the half-eaten cheese to the hole. Which way?",
             "НЕДОЕДЕННЫЙ СЫР НАДО ДОТАЩИТЬ ДО НОРЫ КУДА ИДТИ",
             "Left, to the hole", "НАЛЕВО К НОРЕ",
             "Right, to the crossroads", "НАПРАВО К РАСПУТЬЮ");
    if (ok == 0) {
        clear(); art_hole();
        say("You drag the cheese to the hole. Almost home.",
            "ТЫ ТАЩИШЬ СЫР К НОРЕ ПОЧТИ ДОМА");
    } else {
        clear();
        printf(RED " /\\\n / \\\n /____\\\n\n" RESET);
        say("Crossroads. The zombie shuffles nearby.",
            "РАСПУТЬЕ ЗОМБИ ШАРКАЕТ РЯДОМ");
        zombie -= 4;
    }
    wait_enter();

    /* ---------- 3 ---------- */
    clear();
    art_raven();
    say("Suddenly a raven attacks and takes the cheese.",
        "ВНЕЗАПНО НАПАДАЕТ ВОРОНА ВОРОНА ЗАБИРАЕТ КУСОК СЫРА");
    wait_enter();

    ok = ask("What do you do?", "ЧТО ДЕЛАТЬ",
             "Write RUN in Morse and run home",
             "НА АЗБУКЕ МОРЗЕ НАПИСАТЬ БЕЖАТЬ И БЕЖАТЬ ДОМОЙ",
             "Fight the raven: time a BITE",
             "НАПАСТЬ НА ВОРОНУ НА ВРЕМЯ ОТВЕТИТЬ УКУС");
    if (ok == 0) {
        clear();
        say("MORSE:", "МОРЗЕ");
        morse_print("БЕЖАТЬ ДОМОЙ");
        say("You tap Morse with your paws and run home.",
            "ТЫ СТРОЧИШЬ ЛАПКАМИ МОРЗЯНКУ И УЛЕПЁТЫВАЕШЬ");
        cheese = 0;
    } else {
        clear(); art_raven();
        say("The raven circles. You must bite in time.",
            "ВОРОНА КРУЖИТ НУЖНО УКУСИТЬ ВОВРЕМЯ");
        wait_enter();
        int r = rand() % 100;
        if (r < 55) {
            clear();
            say("BITE! You fight it off and eat the raven.",
                "УКУС ТЫ ОТБИВАЕШЬСЯ И ЕШЬ ВОРОНУ");
            say("Cheese is lost, but you are alive.",
                "СЫР ПОТЕРЯН НО ТЫ ЖИВА");
            cheese = 0;
        } else {
            clear();
            say("Miss. The raven escapes with the cheese.",
                "ПРОМАХ ВОРОНА ВЫРЫВАЕТ СЫР И УЛЕТАЕТ");
            say("Zombie is closer.", "ЗОМБИ БЛИЖЕ");
            zombie -= 6;
        }
    }
    wait_enter();

    /* ---------- 4 ---------- */
    clear();
    print_status(zombie);
    say("You are not a rat. You are a human. Return to your human form.",
        "ТЫ НЕ КРЫСА ТЫ ЧЕЛОВЕК ВЕРНИСЬ В ЧЕЛОВЕЧЕСКУЮ ФОРМУ");
    wait_enter();

    /* ---------- 5 ---------- */
    clear(); art_feather();
    say("You find a chicken feather on the floor.",
        "НА ПОЛУ КУРИНОЕ ПЕРО");
    wait_enter();

    clear(); art_city();
    say("You understand the path leads to the great city GOROKUR.",
        "ТЫ ПОНИМАЕШЬ ПУТЬ ВЕДЁТ В ВЕЛИЧЕСТВЕННЫЙ ГОРОД ГОРОКУР");
    wait_enter();

    clear(); art_farm(); art_chicken();
    say("But instead of GOROKUR you reach an abandoned farm.",
        "НО ВМЕСТО ГОРОКУРА ТЫ ПРИХОДИШЬ НА ЗАБРОШЕННУЮ ФЕРМУ");
    wait_enter();

    /* ---------- 6 ---------- */
    clear(); art_roosters();
    say("ROOSTER GUARDS: Stop! Say the password.",
        "ПЕТУХИ СТРАЖНИКИ СТОЙ НАЗОВИ ПАРОЛЬ");
    char password[64] = "UNKNOWN";
    printf(YELLOW "\nИспользовать блокнот? [y/n]: " RESET);
    fflush(stdout);
    int ch = getchar();
    if (ch == 'y' || ch == 'Y') {
        clear();
        say("On the page is written:", "НА СТРАНИЦЕ НАПИСАНО");
        morse_print("КУРЯТИНА");
        strcpy(password, "КУРЯТИНА");
        wait_enter();
    }
    clear(); art_roosters();
    if (strcmp(password, "КУРЯТИНА") == 0) {
        say("The roosters step aside. You enter.",
            "ПЕТУХИ РАССТУПАЮТСЯ ТЫ ВХОДИШЬ");
    } else {
        say("The roosters are unhappy, but let you in.",
            "ПЕТУХИ НЕДОВОЛЬНЫ НО ПРОПУСКАЮТ");
    }
    wait_enter();

    /* ---------- 7 ---------- */
    clear(); art_cat();
    ok = ask("It is dark in the henhouse. What do you do?",
             "В КУРЯТНИКЕ ТЕМНО ЧТО ДЕЛАТЬ",
             "Start threatening everyone",
             "НАЧАТЬ УГРОЖАТЬ ВСЕМ",
             "Quietly sit next to the cat",
             "ТИХО ПОДСЕСТЬ К КОТУ В ТЁМНОМ УГЛУ");
    if (ok == 0) {
        clear(); art_roosters();
        say("You start threatening. Kicked out to the street.",
            "ТЫ НАЧИНАЕШЬ УГРОЖАТЬ ТЕБЯ ВЫГОНЯЮТ");
        wait_enter();
        clear(); art_chess_pawn();
        say("You notice a chess pawn.",
            "НО ТЫ УСПЕВАЕШЬ ЗАМЕТИТЬ В ЛАПАХ У НЕЁ ПЕШКА");
        wait_enter();
    } else {
        clear();
        say("The cat whispers: Find the pawn-eater hen.",
            "КОТ ШЕПЧЕТ ИЩИ СТРАННУЮ КУРУ");
        wait_enter();
    }

    /* ---------- 10 ---------- */
    clear();
    ok = ask("You need a bite. Cheese or chicken?",
             "ТЫ ДОЛЖНА ПЕРЕКУСИТЬ СЫР ИЛИ КУРИЦА",
             "Cheese", "СЫР",
             "Chicken", "КУРИЦА");
    if (ok == 0) {
        clear(); art_cheese_rot();
        say("END: cheese poisoning.", "КОНЕЦ ОТРАВЛЕНИЕ СЫРОМ");
    } else {
        clear(); art_basement();
        say("END: henhouse basement.", "КОНЕЦ ПОДВАЛ КУРЯТНИКА");
    }

    printf("\n[Enter]\n");
    wait_enter();
    return 0;
}