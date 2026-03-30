#include <stdio.h>
#include "data.h"

int main()
{
    FILE *input = fopen("estacoes.csv", "r");
    FILE *output = fopen("exit.bin", "wb+");

    write_bin_file(input, output);

    fclose(input);
    fclose(output);

    return 0;
}