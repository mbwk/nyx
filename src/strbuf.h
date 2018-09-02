#pragma once

struct strbuf {
    char *b;
    int len;
};

#define STRBUF_INIT {NULL, 0}

void sb_append(struct strbuf *sb, const char *s, int len);
void sb_free(struct strbuf *sb);


