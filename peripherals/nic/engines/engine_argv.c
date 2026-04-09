#include "engine.h"

#include <string.h>

static void collapse_quotes(char *s)
{
    char *w = s;
    const char *r = s;
    char q = 0;

    while (*r)
    {
        if (q)
        {
            if (*r == q)
            {
                q = 0;
                r++;
                continue;
            }
            *w++ = *r++;
        }
        else
        {
            if (*r == '\'' || *r == '"')
            {
                q = *r++;
                continue;
            }
            *w++ = *r++;
        }
    }
    *w = '\0';
}

int engine_argv_parse(char *buf, char *argv[], int max_argv)
{
    const int n = (int)strlen(buf);
    int argc = 0;
    int i = 0;

    while (i < n)
    {
        while (i < n && buf[i] == ' ')
            i++;
        if (i >= n)
            break;

        const int start = i;
        char quote = 0;

        while (i < n)
        {
            if (quote)
            {
                if (buf[i] == (unsigned char)quote)
                    quote = 0;
                i++;
                continue;
            }
            if (buf[i] == ' ')
                break;
            if (buf[i] == '\'' || buf[i] == '"')
            {
                quote = buf[i];
                i++;
                continue;
            }
            i++;
        }

        if (quote != 0)
            return -1;

        const int end = i;
        const char tmp = buf[end];
        buf[end] = '\0';
        collapse_quotes(buf + start);

        if (argc >= max_argv)
            return -1;
        argv[argc++] = buf + start;

        i = end;
        if (tmp == ' ')
            i++;
    }

    return argc;
}
