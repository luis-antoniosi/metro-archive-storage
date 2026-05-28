#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "binarioNaTela.h"

void BinarioNaTela(char *arquivo)
{
    FILE *fs;
    if (arquivo == NULL || !(fs = fopen(arquivo, "rb")))
    {
        fprintf(stderr,
                "ERRO AO ESCREVER O BINARIO NA TELA (função binarioNaTela): "
                "não foi possível abrir o arquivo que me passou para leitura. "
                "Ele existe e você tá passando o nome certo? Você lembrou de "
                "fechar ele com fclose depois de usar?\n");
        return;
    }

    fseek(fs, 0, SEEK_END);
    size_t fl = ftell(fs);

    fseek(fs, 0, SEEK_SET);
    unsigned char *mb = (unsigned char *)malloc(fl);
    if(!fread(mb, 1, fl, fs)){
        return;
    };

    unsigned long cs = 0;
    for (unsigned long i = 0; i < fl; i++)
    {
        cs += (unsigned long)mb[i];
    }

    printf("%lf\n", (cs / (double)100));

    free(mb);
    fclose(fs);
}