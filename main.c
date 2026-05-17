#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_WORD_LEN 64
#define MAX_WORDS    100000
#define MAX_MATCHES  5

#define ANSI_GREEN  "\e[38;2;64;160;43m"
#define ANSI_YELLOW "\e[38;2;223;142;29m"
#define ANSI_RESET  "\033[0m"

static char (*dictionary)[MAX_WORD_LEN];
static int   dict_size = 0;

static int load_dictionary(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return 0; }

    dictionary = malloc(MAX_WORDS * sizeof(*dictionary));
    if (!dictionary) { fclose(f); return 0; }

    char line[MAX_WORD_LEN];
    while (dict_size < MAX_WORDS && fgets(line, sizeof(line), f)) {
        int len = (int)strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';
        if (len == 0) continue;
        for (int i = 0; i < len; i++) line[i] = (char)tolower((unsigned char)line[i]);
        strncpy(dictionary[dict_size], line, MAX_WORD_LEN - 1);
        dictionary[dict_size][MAX_WORD_LEN - 1] = '\0';
        dict_size++;
    }

    fclose(f);
    return 1;
}

static void build_set(const char *word, int set[26]) {
    memset(set, 0, 26 * sizeof(int));
    for (int i = 0; word[i]; i++) {
        int c = (unsigned char)word[i] - 'a';
        if (c >= 0 && c < 26) set[c] = 1;
    }
}

static int match_pattern(const char *word, const char *target, const char *pattern) {
    int target_set[26];
    build_set(target, target_set);
    for (int i = 0; pattern[i]; i++) {
        if (pattern[i] == 'x') {
            if (word[i] != target[i]) return 0;
        } else {
            int c = (unsigned char)word[i] - 'a';
            if (c >= 0 && c < 26 && target_set[c]) return 0;
        }
    }
    return 1;
}


static int match_pattern_soft(const char *word, const char *target, const char *pattern) {
    int target_set[26];
    build_set(target, target_set);

    int target_count[26] = {0};
    for (int i = 0; target[i]; i++) {
        int c = (unsigned char)target[i] - 'a';
        if (c >= 0 && c < 26) target_count[c]++;
    }

    int used[26] = {0};

    for (int i = 0; pattern[i]; i++) {
        if (pattern[i] == 'x') {
            int c = (unsigned char)word[i] - 'a';
            if (c < 0 || c >= 26 || !target_set[c]) return 0;
            used[c]++;
            if (used[c] > target_count[c]) return 0;
        } else {
            int c = (unsigned char)word[i] - 'a';
            if (c >= 0 && c < 26 && target_set[c]) return 0;
        }
    }
    return 1;
}

static const char *PATTERNS[] = {
    "__xxx",
    "xxx_x",
    "x___x",
    "xxx_x",
    "x_x__",
    "xxx__",
};
#define NUM_PATTERNS (int)(sizeof(PATTERNS) / sizeof(PATTERNS[0]))

static void highlight_word(const char *word, const char *target, const char *pattern) {
    int target_set[26];
    build_set(target, target_set);

    for (int i = 0; pattern[i] && word[i]; i++) {
        char wc = word[i];
        if (pattern[i] == 'x') {
            if (wc == target[i]) {
                printf(ANSI_GREEN "%c" ANSI_RESET, wc);
            } else {
                int c = (unsigned char)wc - 'a';
                if (c >= 0 && c < 26 && target_set[c])
                    printf(ANSI_YELLOW "%c" ANSI_RESET, wc);
                else
                    printf("%c", wc);
            }
        } else {
            printf("%c", wc);
        }
    }
}

static void word_search(const char *target) {
    for (int p = 0; p < NUM_PATTERNS; p++) {
        const char *pattern = PATTERNS[p];
        char *matches[MAX_MATCHES];
        int   nm = 0;

        for (int w = 0; w < dict_size && nm < MAX_MATCHES; w++)
            if (match_pattern(dictionary[w], target, pattern))
                matches[nm++] = dictionary[w];

        if (nm == 0) {
            for (int w = 0; w < dict_size && nm < MAX_MATCHES; w++)
                if (match_pattern_soft(dictionary[w], target, pattern))
                    matches[nm++] = dictionary[w];
        }

        printf("%d:", p + 1);
        for (int i = 0; i < nm; i++) {
            printf(" ");
            highlight_word(matches[i], target, pattern);
            if (i < nm - 1) printf(",");
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <word>\n", argv[0]);
        return 1;
    }

    char target[MAX_WORD_LEN];
    strncpy(target, argv[1], MAX_WORD_LEN - 1);
    target[MAX_WORD_LEN - 1] = '\0';
    for (int i = 0; target[i]; i++) target[i] = (char)tolower((unsigned char)target[i]);

    if (!load_dictionary("./words.txt")) return 1;

    word_search(target);

    free(dictionary);
    return 0;
}
