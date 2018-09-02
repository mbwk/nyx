#include "strbuf.h"

#include <stdlib.h>
#include <string.h>

void sb_append(struct strbuf *sb, const char *s, int len)
{
    char *new = realloc(sb->b, sb->len + len);

    if (new == NULL) return;
    memcpy(&new[sb->len], s, len);
    sb->b = new;
    sb->len += len;
}

void sb_free(struct strbuf *sb)
{
    free(sb->b);
}

