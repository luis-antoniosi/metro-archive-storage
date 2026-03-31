#include <stdio.h>
#include "data.h"

#define INPUT_SIZE 64

// n to gostanddo disso de printar o erro duas vezes nos cases, mas n sei oq fazer

int main()
{
    FILE *input = NULL;
    FILE *output = NULL;

    char buffer[BUF_SIZE];
    char option = '0';
    char input1[INPUT_SIZE], input2[INPUT_SIZE];

    do
    {
        if (!fgets(buffer, BUF_SIZE, stdin))
            break;

        if (sscanf(buffer, "%c", &option) != 1)
        {
            printf("Invalid option, use a number\n");
            continue;
        }

        switch (option)
        {
        // turning .csv into .bin
        case '1':
            if (sscanf(buffer, "%*c %s %s", input1, input2) == 2) // %*c -> the * ignores it
            {
                input = fopen(input1, "r");
                output = fopen(input2, "wb+");

                if (write_bin_file(input, output) == DATA_SUCCESS)
                {
                    printf("Done\n");
                    fclose(input);
                    fclose(output);
                }
                else
                    printf("Falha no processamento do arquivo.\n");
            }
            else
            {
                printf("Falha no processamento do arquivo.\n");
            }

            break;
        // prints all registers
        case '2':
            if (sscanf(buffer, "%*c %s", input1) == 1)
            {
                input = fopen(input1, "rb");

                if (input != NULL)
                {
                    if (print_all_data(input) == DATA_FAILURE)
                        printf("Registro inexistente.");

                    fclose(input);
                }
                else
                {
                    printf("Falha no processamento do arquivo.\n");
                }
            }
            else
            {
                printf("Falha no processamento do arquivo.\n");
            }

            break;
        default:
            break;
        }
    } while (option != '9');

    if (input)
        fclose(input);
    if (output)
        fclose(output);

    return 0;
}