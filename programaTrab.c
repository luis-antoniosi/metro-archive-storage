#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include "data.h"
#include "header.h"

#define INPUT_SIZE 64

// n to gostando disso de printar o erro duas vezes nos cases, mas n sei oq fazer

void binary_on_screen(char *fileName);

int main()
{
    setlocale(LC_ALL, ".UTF8"); // needed to print utf-8 characters like ç on console

    FILE *input = NULL;
    FILE *output = NULL;

    char buffer[BUF_SIZE];
    char input1[INPUT_SIZE], input2[INPUT_SIZE];
    int numInput = 0;
    char option = '0';

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
        // turning .csv into .bin, prints checksum
        case '1':
            if (sscanf(buffer, "%*c %s %s", input1, input2) == 2) // %*c -> the * ignores it
            {
                input = fopen(input1, "r");
                output = fopen(input2, "wb+");
                status0(output);

                if (write_bin_file(input, output) == DATA_SUCCESS)
                {
                    fclose(input);
                    status1(output);
                    fclose(output);

                    binary_on_screen(input2);
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

                if (input)
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
        // prints all registers where (search criteria)
        case '3':
            if (sscanf(buffer, "%*c %s %d", input1, &numInput) == 2)
            {
                input = fopen(input1, "rb");

                if (input)
                {
                    print_all_data_where(input, numInput);
                    
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

        case '4':
            if(sscanf(buffer, "%*c %s %d", input1, &numInput) == 2)
            {
                input = fopen(input1, "rb+");
                status0(input);

                if(input)
                {

                    

                    status1(input);
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

            //colocar status 1 ao fechar o arquivo
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

void binary_on_screen(char *fileName)
{
    FILE *file = NULL;

    if (!fileName || !(file = fopen(fileName, "rb")))
        return;

    fseek(file, 0, SEEK_END);
    long totalBytes = ftell(file);

    fseek(file, 0, SEEK_SET);
    unsigned char *bytesStr = malloc(sizeof(unsigned char) * totalBytes);
    if(fread(bytesStr, 1, totalBytes, file) != (long unsigned int)totalBytes){
        printf("Unable to read file\n");
        free(bytesStr);
        return;
    }

    unsigned long byteSum = 0;
    for (long i = 0; i < totalBytes; i++)
        byteSum += (unsigned long)bytesStr[i];

    printf("%lf\n", (byteSum / 100.0));

    free(bytesStr);
    fclose(file);
}
