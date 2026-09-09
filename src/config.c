/*
 * config.c — config.ini settings persistence + keybinds.ini bridge for the
 * desktop host.
 *
 * config.ini stores the launcher-exposed settings under [Settings]. The
 * write is a surgical in-place update (SMW's WriteConfigFile pattern): the
 * existing file text is preserved verbatim — comments, key order, and any
 * sections the launcher itself owns (e.g. the hotkey editor's [KeyMap]) —
 * and only the keys below are rewritten. A key present in its section is
 * replaced in place; a missing key is appended to the end of [Settings];
 * a missing section is appended at the end of the file.
 *
 * keybinds.ini is the file the recomp-ui Controller page edits ([player1] /
 * [player2], SDL scancode names). This host seeds it with its own keyboard
 * layout when the file is missing (so first-run matches the pre-launcher
 * host) and then applies [player1] and [player2] onto per-player scancode ->
 * runner input-bit maps at boot. Edits from the launcher apply on the next
 * launch, matching the file's own "Restart the game to apply" contract.
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "desktop/sdl_compat.h"

/* ── config.ini [Settings] ─────────────────────────────────────────────── */

#define ST_INI_SECTION "Settings"

typedef struct CfgKV {
    const char *key;   /* canonical key name */
    char        val[64];
    int         done;  /* written into the output yet? */
} CfgKV;

typedef struct CfgBuf {
    char  *p;
    size_t len, cap;
} CfgBuf;

static void CfgBuf_AddN(CfgBuf *b, const char *s, size_t n) {
    if (b->len + n + 1 > b->cap) {
        size_t cap = (b->len + n + 1) * 2;
        char *p = (char *)realloc(b->p, cap);
        if (!p) return;  /* best-effort: leave the old file untouched */
        b->p = p;
        b->cap = cap;
    }
    memcpy(b->p + b->len, s, n);
    b->len += n;
    b->p[b->len] = '\0';
}

static void CfgBuf_Str(CfgBuf *b, const char *s) {
    CfgBuf_AddN(b, s, strlen(s));
}

static int StStrEqNoCase(const char *a, const char *b) {
    for (; *a && *b; ++a, ++b) {
        char ca = (*a >= 'A' && *a <= 'Z') ? (char)(*a + 32) : *a;
        char cb = (*b >= 'A' && *b <= 'Z') ? (char)(*b + 32) : *b;
        if (ca != cb) return 0;
    }
    return *a == 0 && *b == 0;
}

/* Trim leading/trailing whitespace in place; returns the trimmed start. */
static char *StTrim(char *s) {
    char *start = s;
    while (*start == ' ' || *start == '\t') start++;
    size_t n = strlen(start);
    while (n > 0 && (start[n - 1] == ' ' || start[n - 1] == '\t' ||
                     start[n - 1] == '\r' || start[n - 1] == '\n'))
        start[--n] = '\0';
    return start;
}

static void SettingsFillKVs(CfgKV *kvs, int n,
                            const SuperTennisSettings *s) {
    const struct { const char *key; const int *v; } ints[] = {
        {"OutputMethod",  &s->output_method},
        {"WindowScale",   &s->window_scale},
        {"Fullscreen",    &s->fullscreen},
        {"IgnoreAspect",  &s->ignore_aspect},
        {"LinearFilter",  &s->linear_filter},
        {"EnableAudio",   &s->enable_audio},
        {"AudioFreq",     &s->audio_freq},
        {"Volume",        &s->volume},
        {"Player1Source", &s->player_src[0]},
        {"Player2Source", &s->player_src[1]},
        {"Deadzone1",     &s->deadzone[0]},
        {"Deadzone2",     &s->deadzone[1]},
        {"SkipLauncher",  &s->skip_launcher},
    };
    for (int i = 0; i < n; i++) {
        kvs[i].key = ints[i].key;
        snprintf(kvs[i].val, sizeof(kvs[i].val), "%d", *ints[i].v);
        kvs[i].done = 0;
    }
}

void SuperTennisSettingsLoad(const char *path, SuperTennisSettings *s) {
    FILE *f = fopen(path, "r");
    if (!f) return;
    int in_settings = 0;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *p = StTrim(line);
        if (!p[0] || p[0] == '#' || p[0] == ';') continue;
        if (p[0] == '[') {
            char *end = strchr(p, ']');
            if (end) *end = '\0';
            in_settings = StStrEqNoCase(p + 1, ST_INI_SECTION);
            continue;
        }
        if (!in_settings) continue;
        char *eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = StTrim(p);
        char *val = StTrim(eq + 1);
        const struct { const char *key; int *v; } ints[] = {
            {"OutputMethod",  &s->output_method},
            {"WindowScale",   &s->window_scale},
            {"Fullscreen",    &s->fullscreen},
            {"IgnoreAspect",  &s->ignore_aspect},
            {"LinearFilter",  &s->linear_filter},
            {"EnableAudio",   &s->enable_audio},
            {"AudioFreq",     &s->audio_freq},
            {"Volume",        &s->volume},
            {"Player1Source", &s->player_src[0]},
            {"Player2Source", &s->player_src[1]},
            {"Deadzone1",     &s->deadzone[0]},
            {"Deadzone2",     &s->deadzone[1]},
            {"SkipLauncher",  &s->skip_launcher},
        };
        for (size_t i = 0; i < sizeof(ints) / sizeof(ints[0]); i++) {
            if (StStrEqNoCase(key, ints[i].key)) {
                long v = strtol(val, NULL, 10);
                *ints[i].v = (int)v;
                break;
            }
        }
    }
    fclose(f);
}

void SuperTennisSettingsSave(const char *path,
                             const SuperTennisSettings *s) {
    CfgKV kvs[13];
    SettingsFillKVs(kvs, (int)(sizeof(kvs) / sizeof(kvs[0])), s);
    const int N = (int)(sizeof(kvs) / sizeof(kvs[0]));

    CfgBuf out = {0};
    FILE *in = fopen(path, "r");
    if (in) {
        char line[512];
        char cur_section[64] = "";
        while (fgets(line, sizeof(line), in)) {
            char *p = StTrim(line);
            if (p[0] == '[') {
                /* Leaving a section: append any of our keys it lacked. */
                for (int i = 0; i < N; i++)
                    if (!kvs[i].done &&
                        StStrEqNoCase(cur_section, ST_INI_SECTION)) {
                        CfgBuf_Str(&out, kvs[i].key);
                        CfgBuf_Str(&out, " = ");
                        CfgBuf_Str(&out, kvs[i].val);
                        CfgBuf_Str(&out, "\n");
                        kvs[i].done = 1;
                    }
                /* Emit the header before parsing it (strchr below would
                 * otherwise strip the closing bracket from the copy). */
                CfgBuf_Str(&out, p);  /* StTrim stripped the trailing newline */
                CfgBuf_Str(&out, "\n");
                char *end = strchr(p, ']');
                if (end) *end = '\0';
                snprintf(cur_section, sizeof(cur_section), "%s", p + 1);
                continue;
            }
            /* Replace an existing assignment in place (not commented out). */
            int replaced = 0;
            if (StStrEqNoCase(cur_section, ST_INI_SECTION)) {
                char *body = p;
                while (*body == ' ' || *body == '\t') body++;
                if (body[0] != '#' && body[0] != ';') {
                    char *eq = strchr(body, '=');
                    if (eq) {
                        char saved = *eq;
                        *eq = '\0';
                        char *key = StTrim(body);
                        for (int i = 0; i < N && !replaced; i++) {
                            if (!kvs[i].done &&
                                StStrEqNoCase(key, kvs[i].key)) {
                                CfgBuf_Str(&out, kvs[i].key);
                                CfgBuf_Str(&out, " = ");
                                CfgBuf_Str(&out, kvs[i].val);
                                CfgBuf_Str(&out, "\n");
                                kvs[i].done = 1;
                                replaced = 1;
                            }
                        }
                        *eq = saved;
                    }
                }
            }
            if (!replaced) {
                CfgBuf_Str(&out, p);  /* StTrim stripped the trailing newline */
                CfgBuf_Str(&out, "\n");
            }
        }
        /* End of file: flush any of our keys the section still lacked. */
        if (StStrEqNoCase(cur_section, ST_INI_SECTION))
            for (int i = 0; i < N; i++)
                if (!kvs[i].done) {
                    CfgBuf_Str(&out, kvs[i].key);
                    CfgBuf_Str(&out, " = ");
                    CfgBuf_Str(&out, kvs[i].val);
                    CfgBuf_Str(&out, "\n");
                    kvs[i].done = 1;
                }
        fclose(in);
    }

    /* Keys never placed (no [Settings] section, or no file): append one. */
    int pending = 0;
    for (int i = 0; i < N; i++)
        if (!kvs[i].done) pending = 1;
    if (pending) {
        if (out.len) CfgBuf_Str(&out, "\n");
        CfgBuf_Str(&out, "[" ST_INI_SECTION "]\n");
        for (int i = 0; i < N; i++)
            if (!kvs[i].done) {
                CfgBuf_Str(&out, kvs[i].key);
                CfgBuf_Str(&out, " = ");
                CfgBuf_Str(&out, kvs[i].val);
                CfgBuf_Str(&out, "\n");
            }
    }

    FILE *f = fopen(path, "w");
    if (!f) {
        free(out.p);
        return;
    }
    if (out.p) fwrite(out.p, 1, out.len, f);
    fclose(f);
    free(out.p);
    fprintf(stderr, "[Config] Wrote %s\n", path);
}

/* ── keybinds.ini bridge ───────────────────────────────────────────────── */

typedef struct KeyBindKV {
    const char *name;
    uint32_t    bit;  /* runner 12-bit input word bit */
} KeyBindKV;

static const KeyBindKV kKeyBindNames[] = {
    {"a",      0x100},  /* A (right face) */
    {"b",      0x001},  /* B (bottom face) */
    {"x",      0x200},  /* X (top face) */
    {"y",      0x002},  /* Y (left face) */
    {"l",      0x400},
    {"r",      0x800},
    {"start",  0x008},
    {"select", 0x004},
    {"up",     0x010},
    {"down",   0x020},
    {"left",   0x040},
    {"right",  0x080},
};

void SuperTennisKeyBindsDefaults(uint32_t *map, int n) {
    /* Defaults match the pre-launcher host keyboard layout. */
    static const struct { SDL_Scancode sc; uint32_t bit; } d[] = {
        {SDL_SCANCODE_Z,       0x001},  /* B */
        {SDL_SCANCODE_X,       0x100},  /* A */
        {SDL_SCANCODE_A,       0x002},  /* Y */
        {SDL_SCANCODE_S,       0x200},  /* X */
        {SDL_SCANCODE_Q,       0x400},  /* L */
        {SDL_SCANCODE_E,       0x800},  /* R */
        {SDL_SCANCODE_RETURN,  0x008},  /* Start */
        {SDL_SCANCODE_BACKSPACE, 0x004},/* Select */
        {SDL_SCANCODE_UP,      0x010},
        {SDL_SCANCODE_DOWN,    0x020},
        {SDL_SCANCODE_LEFT,    0x040},
        {SDL_SCANCODE_RIGHT,   0x080},
    };
    memset(map, 0, (size_t)n * sizeof(map[0]));
    for (size_t i = 0; i < sizeof(d) / sizeof(d[0]); i++)
        if ((int)d[i].sc >= 0 && (int)d[i].sc < n)
            map[(int)d[i].sc] = d[i].bit;
}

void SuperTennisKeyBindsWriteDefaults(const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f,
        "# Controller Keybinds\n"
        "# Edit values to customize, or rebind from the launcher's Controller\n"
        "# page (both rewrite this file). Restart the game to apply.\n"
        "# Use SDL key names. Common: A B C ... Z, 0-9, F1-F12, Up Down Left\n"
        "# Right, Return, Tab, Space, Backspace, Escape. Use \"None\" to leave\n"
        "# a button unbound.\n"
        "\n"
        "[player1]\n");
    for (size_t i = 0; i < sizeof(kKeyBindNames) / sizeof(kKeyBindNames[0]); i++) {
        /* Map the canonical button to its default scancode by name so the
         * file reads like the launcher's own output. */
        uint32_t bit = kKeyBindNames[i].bit;
        const char *name = NULL;
        static const struct { uint32_t bit; SDL_Scancode sc; } d[] = {
            {0x001, SDL_SCANCODE_Z}, {0x100, SDL_SCANCODE_X},
            {0x002, SDL_SCANCODE_A}, {0x200, SDL_SCANCODE_S},
            {0x400, SDL_SCANCODE_Q}, {0x800, SDL_SCANCODE_E},
            {0x008, SDL_SCANCODE_RETURN}, {0x004, SDL_SCANCODE_BACKSPACE},
            {0x010, SDL_SCANCODE_UP}, {0x020, SDL_SCANCODE_DOWN},
            {0x040, SDL_SCANCODE_LEFT}, {0x080, SDL_SCANCODE_RIGHT},
        };
        for (size_t j = 0; j < sizeof(d) / sizeof(d[0]); j++)
            if (d[j].bit == bit) { name = SDL_GetScancodeName(d[j].sc); break; }
        fprintf(f, "%-7s = %s\n", kKeyBindNames[i].name,
                (name && name[0]) ? name : "None");
    }
    fprintf(f, "\n[player2]\n");
    for (size_t i = 0; i < sizeof(kKeyBindNames) / sizeof(kKeyBindNames[0]); i++)
        fprintf(f, "%-7s = None\n", kKeyBindNames[i].name);
    fclose(f);
}

void SuperTennisKeyBindsLoad(const char *path, uint32_t *maps[2], int n) {
  FILE *f = fopen(path, "r");
  if (!f) return;
  /* -1 outside [player1]/[player2]; 0 = player 1, 1 = player 2. */
  int player = -1;
  char line[256];
  while (fgets(line, sizeof(line), f)) {
    char *p = StTrim(line);
    if (!p[0] || p[0] == '#' || p[0] == ';') continue;
    if (p[0] == '[') {
      char *end = strchr(p, ']');
      if (end) *end = '\0';
      if (StStrEqNoCase(p + 1, "player1")) player = 0;
      else if (StStrEqNoCase(p + 1, "player2")) player = 1;
      else player = -1;
      continue;
    }
    if (player < 0 || player > 1) continue;
    char *eq = strchr(p, '=');
    if (!eq) continue;
    *eq = '\0';
    char *key = StTrim(p);
    char *val = StTrim(eq + 1);
    SDL_Scancode sc = SDL_GetScancodeFromName(val);
    if (sc == SDL_SCANCODE_UNKNOWN) continue;
    if ((int)sc >= 0 && (int)sc < n) {
      for (size_t i = 0; i < sizeof(kKeyBindNames) / sizeof(kKeyBindNames[0]); i++) {
        if (StStrEqNoCase(key, kKeyBindNames[i].name)) {
          /* Moving a button to a new key releases its old one. */
          uint32_t bit = kKeyBindNames[i].bit;
          for (int j = 0; j < n; j++)
            if (maps[player][j] == bit) maps[player][j] = 0;
          maps[player][(int)sc] = bit;
          break;
        }
      }
    }
  }
  fclose(f);
}
