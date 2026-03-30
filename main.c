#include <stdio.h>
#include "header.h"

int main()
{
    FILE *file = fopen("arquivoSaida.bin", "wb+");

    write_header(file, create_header());

    fclose(file);

    return 0;
}