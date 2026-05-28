#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include "types.h"
#include "headerUtils.h"
#include "register.h"
#include "binFile.h"    
#include "binarioNaTela.h"

#define INPUT_SIZE 64

/*
Aluno:  Luís Gustavo Vieira Antoniosi   | NºUSP: 17067476
Aluno:  Luiz Filipe Sá Vioto            | NºUSP: 16886252
*/

static void print_file_failure()
{
    printf("Falha no processamento do arquivo.\n");
}

int main()
{
    setlocale(LC_ALL, ".UTF8"); // needed to print utf-8 characters like ç on console (not really needed after they fixed the test cases but nice to have)

    FILE *bin = NULL;

    char buffer[BUF_SIZE];
    char input1[INPUT_SIZE], input2[INPUT_SIZE];
    int numInput = 0;
    char option = '0';

    if (!fgets(buffer, BUF_SIZE, stdin))
        return 1;

    if (sscanf(buffer, "%c", &option) != 1)
    {
        printf("Option is not a character\n");
        return 1;
    }

    switch (option)
    {
    // turning .csv into .bin, prints checksum
    case '1':
        if (sscanf(buffer, "%*c %s %s", input1, input2) == 2) // %*c -> the * ignores the char
        {
            FILE *csv = fopen(input1, "r");
            bin = fopen(input2, "wb+");

            if (csv && bin && write_bin_file(csv, bin) == DATA_SUCCESS) // need to check for bin in every case so it can be safely closed
            {
                fclose(bin);
                bin = NULL;

                BinarioNaTela(input2);
            }
            else
            {
                print_file_failure();
            }

            if (csv)
                fclose(csv);
            if (bin)
                fclose(bin);

            bin = NULL;
        }
        else
        {
            print_file_failure();
        }

        break;
    // prints all registers
    case '2':
        if (sscanf(buffer, "%*c %s", input1) == 1)
        {
            bin = fopen(input1, "rb");

            if (bin)
            {
                print_all_data(bin);

                fclose(bin);
                bin = NULL;
            }
            else
            {
                print_file_failure();
            }
        }
        else
        {
            print_file_failure();
        }

        break;
    // prints all registers where (search criteria)
    case '3':
        if (sscanf(buffer, "%*c %s %d", input1, &numInput) == 2)
        {
            bin = fopen(input1, "rb");

            if (bin)
            {
                print_all_data_where(bin, numInput);

                fclose(bin);
                bin = NULL;
            }
            else
            {
                print_file_failure();
            }
        }
        else
        {
            print_file_failure();
        }
        break;
    // deletes registers where (search criteria)
    case '4':
        if (sscanf(buffer, "%*c %s %d", input1, &numInput) == 2)
        {
            bin = fopen(input1, "rb+");

            if (bin)
            {
                delete_all_data_where(bin, numInput);

                fclose(bin);
                bin = NULL;

                BinarioNaTela(input1);
            }
            else
            {
                print_file_failure();
            }
        }
        else
        {
            print_file_failure();
        }
        break;
    // inserts a register
    case '5':
        if (sscanf(buffer, "%*c %s %d", input1, &numInput) == 2)
        {
            bin = fopen(input1, "rb+");

            if (bin)
            {
                insert_data(bin, numInput);

                fclose(bin);
                bin = NULL;

                binary_on_screen(input1);
            }
            else
            {
                print_file_failure();
            }
        }
        else
        {
            print_file_failure();
        }
        break;
    // updates registers where (search criteria)
    case '6':
        if (sscanf(buffer, "%*c %s %d", input1, &numInput) == 2)
        {
            bin = fopen(input1, "rb+");

            if (bin)
            {
                update_data_where(bin, numInput);

                fclose(bin);
                bin = NULL;

                binary_on_screen(input1);
            }
            else
            {
                print_file_failure();
            }
        }
        else
        {
            print_file_failure();
        }
        break;
    default:
        printf("Invalid option! The cases go from 1 to 6.\n");
        break;
    }

    if (bin) // the attribuitions of bin to NULL after closing are so this doesnt close an already closed address
        fclose(bin);

    return 0;
}