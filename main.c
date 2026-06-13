#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include "types.h"
#include "header/header.h"
#include "register/register.h"
#include "binFile/binFile.h"

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
    char filePath[INPUT_SIZE], outputPath[INPUT_SIZE];
    int iterations = 0;
    int option = -1;

    if (!fgets(buffer, BUF_SIZE, stdin))
        return 1;

    if (sscanf(buffer, "%d", &option) != 1)
    {
        printf("Option is not a number\n");
        return 1;
    }

    switch (option)
    {
    // turning .csv into .bin, prints checksum
    case 1:
        if (sscanf(buffer, "%*d %s %s", filePath, outputPath) == 2) // %*d -> the * ignores the int
        {
            FILE *csv = fopen(filePath, "r");
            bin = fopen(outputPath, "wb+");

            if (csv && bin && write_bin_file(csv, bin) == SUCCESS) // need to check for bin in every case so it can be safely closed
            {
                fclose(bin);
                bin = NULL;

                binary_on_screen(outputPath);
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
    case 2:
        if (sscanf(buffer, "%*d %s", filePath) == 1)
        {
            bin = fopen(filePath, "rb");

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
    case 3:
        if (sscanf(buffer, "%*d %s %d", filePath, &iterations) == 2)
        {
            bin = fopen(filePath, "rb");

            if (bin)
            {
                print_all_data_where(bin, iterations);

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
    case 4:
        if (sscanf(buffer, "%*d %s %d", filePath, &iterations) == 2)
        {
            bin = fopen(filePath, "rb+");

            if (bin)
            {
                delete_all_data_where(bin, iterations);

                fclose(bin);
                bin = NULL;

                binary_on_screen(filePath);
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
    case 5:
        if (sscanf(buffer, "%*d %s %d", filePath, &iterations) == 2)
        {
            bin = fopen(filePath, "rb+");

            if (bin)
            {
                insert_data(bin, iterations);

                fclose(bin);
                bin = NULL;

                binary_on_screen(filePath);
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
    case 6:
        if (sscanf(buffer, "%*d %s %d", filePath, &iterations) == 2)
        {
            bin = fopen(filePath, "rb+");

            if (bin)
            {
                update_data_where(bin, iterations);

                fclose(bin);
                bin = NULL;

                binary_on_screen(filePath);
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
    case 7:
        if (sscanf(buffer, "%*d %s %s", filePath, outputPath) == 2)
        {
            bin = fopen(filePath, "rb");
            FILE *index = fopen(outputPath, "wb+");

            if (bin && index)
            {
                create_index(bin, index);
                fclose(bin);

                change_status(index, STATUS_CONSISTENT); // change this and other functions to another .c
                fclose(index);
                bin = NULL;

                binary_on_screen(outputPath);
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
    case 8:
        if (sscanf(buffer, "%*d %s %s %d", filePath, outputPath, &iterations) == 3)
        {
            bin = fopen(filePath, "rb");
            FILE *index = fopen(outputPath, "rb");

            if (bin && index)
            {
                search_with_index(bin, index, iterations);
                fclose(bin);

                fclose(index);
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
    case 9:
        if (sscanf(buffer, "%*d %s %s %d", filePath, outputPath, &iterations) == 3)
        {
            bin = fopen(filePath, "rb+");
            FILE *index = fopen(outputPath, "rb+");

            if (bin && index)
            {
                insert_index(bin, index, iterations);
                change_status(bin, STATUS_CONSISTENT);
                fclose(bin);
                bin = NULL;

                fclose(index);
                
                binary_on_screen(filePath);
                binary_on_screen(outputPath);
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
    case 10:
        if (sscanf(buffer, "%*d %s %s %d", filePath, outputPath, &iterations) == 3)
        {
            bin = fopen(filePath, "rb+");
            FILE *index = fopen(outputPath, "rb+");

            if (bin && index)
            {
                delete_index(bin, index, iterations);
                
                fclose(bin);
                bin = NULL;

                fclose(index);

                binary_on_screen(filePath);
                binary_on_screen(outputPath);
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