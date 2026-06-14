#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parseUtils.h"

char *custom_strtok(char **buff, char delim)
{
    if (!buff || !(*buff))
        return NULL;

    char *start = *buff;
    char *delimPos = strchr(start, delim);

    if (delimPos)
    {
        *delimPos = '\0';
        *buff = delimPos + 1;
    }
    else
        *buff = NULL;

    return start;
}

int check_for_null(char *str)
{
    return !str || str[0] == '\0' || strcspn(str, "\r\n") == 0 || strcmp(str, "NULO") == 0 ? -1 : atoi(str);
}

char *check_quotes(char *str, char *buf)
{
    if (!str)
        return NULL;

    if (str[0] == '\"')
    {
        buf[0] = '\0';
        char *insideQuotes = str + 1; // since some field values can be inside quotes, this pointer refers to the string inside those quotes

        while (insideQuotes)
        {
            char *closingQuotes = strchr(insideQuotes, '\"');
            if (closingQuotes)
            {
                *closingQuotes = '\0';
                strcat(buf, insideQuotes);
                return buf;
            }

            strcat(buf, insideQuotes);
            strcat(buf, " ");
            insideQuotes = strtok(NULL, " \n\r");
            if (!insideQuotes)
                return buf;
        }
    }

    return str;
}