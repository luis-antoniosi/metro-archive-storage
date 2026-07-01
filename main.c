#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <stdarg.h>

#include "types.h"
#include "utils/utils.h"

#include "register/register.h"
#include "binFile/dataFile.h"
#include "binFile/indexFile.h"

// TODO: update buff variables(?)
// TODO: update if order (checking for function SUCCESS)
// TODO: should probably check if a file is consistent before updating/reading it..

#define CLOSE_FILES_FAILURE(...) close_files_in_failure(__VA_ARGS__, NULL)

/*
Aluno:  Luís Gustavo Vieira Antoniosi   | NºUSP: 17067476
Aluno:  Luiz Filipe Sá Vioto            | NºUSP: 16886252
*/

typedef enum Option
{
    CSV_TO_BIN = 1,
    PRINT_ALL,
    PRINT_WHERE,
    DELETE_WHERE,
    INSERT,
    UPDATE_WHERE,
    CREATE_INDEX,
    SEARCH_WITH_INDEX,
    INSERT_WITH_INDEX,
    DELETE_WITH_INDEX,
    SELECT_JOIN,
    SELECT_JOIN_INDEX,
    ORDER_BY
} Option;

static void print_file_failure()
{
    printf("Falha no processamento do arquivo.\n");
}

static void close_files_in_failure(FILE *firstFile, ...)
{
    va_list arg;
    va_start(arg, firstFile);

    FILE *currentFile = firstFile;

    while (currentFile)
    {
        fclose(currentFile);
        currentFile = va_arg(arg, FILE*);
    }

    va_end(arg);

    print_file_failure();
}

int main()
{
    setlocale(LC_ALL, ".UTF8"); // needed to print utf-8 characters like ç on console (not really needed after they fixed the test cases but nice to have)

    char buffer[BUF_SIZE], filePath[BUF_SIZE], outputPath[BUF_SIZE], idxPath[BUF_SIZE];
    int iterations = 0, option = -1;

    if (!fgets(buffer, BUF_SIZE, stdin))
        return 1;

    if (sscanf(buffer, "%d", &option) != 1)
    {
        printf("Option is not a number\n");
        return 1;
    }

    switch ((Option)option)
    {
    case CSV_TO_BIN:
        if (sscanf(buffer, "%*d %s %s", filePath, outputPath) == 2) // %*d -> the * ignores the int
        {
            FILE *csv = fopen(filePath, "r");
            FILE *data = fopen(outputPath, "wb+"); // wb+ because the "update_header_count" function uses it

            if (csv && data && write_data_file(csv, data) == SUCCESS)
            {
                fclose(csv);
                fclose(data);

                binary_on_screen(outputPath);
            }
            else
            {
                CLOSE_FILES_FAILURE(csv, data);
            }
        }
        else
        {
            print_file_failure();
        }

        break;
    case PRINT_ALL:
        if (sscanf(buffer, "%*d %s", filePath) == 1)
        {
            FILE *data = fopen(filePath, "rb");

            if (data && print_all_data(data) == SUCCESS)
            {
                fclose(data);
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
    case PRINT_WHERE:
        if (sscanf(buffer, "%*d %s %d", filePath, &iterations) == 2)
        {
            FILE *data = fopen(filePath, "rb");

            if (data && print_all_data_where(data, iterations) == SUCCESS)
            {
                fclose(data);
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
    case DELETE_WHERE:
        if (sscanf(buffer, "%*d %s %d", filePath, &iterations) == 2)
        {
            FILE *data = fopen(filePath, "rb+");

            if (data && delete_all_data_where(data, iterations) == SUCCESS)
            {
                fclose(data);

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
    case INSERT:
        if (sscanf(buffer, "%*d %s %d", filePath, &iterations) == 2)
        {
            FILE *data = fopen(filePath, "rb+");

            if (data && insert_data(data, iterations) == SUCCESS)
            {
                fclose(data);

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
    case UPDATE_WHERE:
        if (sscanf(buffer, "%*d %s %d", filePath, &iterations) == 2)
        {
            FILE *data = fopen(filePath, "rb+");

            if (data && update_data_where(data, iterations) == SUCCESS)
            {
                fclose(data);

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
    case CREATE_INDEX:
        if (sscanf(buffer, "%*d %s %s", filePath, outputPath) == 2)
        {
            FILE *data = fopen(filePath, "rb");
            FILE *index = fopen(outputPath, "wb+");

            if (data && index && create_index(data, index) == SUCCESS)
            {
                fclose(data);
                fclose(index);

                binary_on_screen(outputPath);
            }
            else
            {
                CLOSE_FILES_FAILURE(data, index);
            }
        }
        else
        {
            print_file_failure();
        }
        break;
    case SEARCH_WITH_INDEX:
        if (sscanf(buffer, "%*d %s %s %d", filePath, outputPath, &iterations) == 3)
        {
            FILE *data = fopen(filePath, "rb");
            FILE *index = fopen(outputPath, "rb");

            if (data && index && search_with_index(data, index, iterations) == SUCCESS)
            {
                fclose(data);
                fclose(index);
            }
            else
            {
                CLOSE_FILES_FAILURE(data, index);
            }
        }
        else
        {
            print_file_failure();
        }
        break;
    case INSERT_WITH_INDEX:
        if (sscanf(buffer, "%*d %s %s %d", filePath, outputPath, &iterations) == 3)
        {
            FILE *data = fopen(filePath, "rb+");
            FILE *index = fopen(outputPath, "rb+");

            if (data && index && insert_index(data, index, iterations) == SUCCESS)
            {
                fclose(data);
                fclose(index);

                binary_on_screen(filePath);
                binary_on_screen(outputPath);
            }
            else
            {
                CLOSE_FILES_FAILURE(data, index);
            }
        }
        else
        {
            print_file_failure();
        }
        break;
    case DELETE_WITH_INDEX:
        if (sscanf(buffer, "%*d %s %s %d", filePath, outputPath, &iterations) == 3)
        {
            FILE *data = fopen(filePath, "rb+");
            FILE *index = fopen(outputPath, "rb+");

            if (data && index && delete_index(data, index, iterations) == SUCCESS)
            {
                fclose(data);
                fclose(index);

                binary_on_screen(filePath);
                binary_on_screen(outputPath);
            }
            else
            {
                CLOSE_FILES_FAILURE(data, index);
            }
        }
        else
        {
            print_file_failure();
        }
        break;
    case SELECT_JOIN:
        if (sscanf(buffer, "%*d %s %*s %s %*s", filePath, outputPath) == 2)
        {
            FILE *sourceFile = fopen(filePath, "rb");
            FILE *joinFile = fopen(outputPath, "rb");

            if (sourceFile && joinFile && select_join(sourceFile, joinFile) == SUCCESS)
            {
                fclose(sourceFile);
                fclose(joinFile);
            }
            else
            {
                CLOSE_FILES_FAILURE(sourceFile, joinFile);
            }
        }
        else
        {
            print_file_failure();
        }
        break;
    case SELECT_JOIN_INDEX:
        if (sscanf(buffer, "%*d %s %*s %s %*s %s", filePath, outputPath, idxPath) == 3)
        {
            FILE *sourceFile = fopen(filePath, "rb");
            FILE *joinFile = fopen(outputPath, "rb");
            FILE *indexFile = fopen(idxPath, "rb");

            if (sourceFile && joinFile && indexFile && select_join_index(sourceFile, joinFile, indexFile) == SUCCESS)
            {
                fclose(sourceFile);
                fclose(joinFile);
                fclose(indexFile);
            }
            else
            {
                CLOSE_FILES_FAILURE(sourceFile, joinFile, indexFile);
            }
        }
        break;
    case ORDER_BY:
        // the variable for idxPath is bad, but I'll eventually fix it
        if (sscanf(buffer, "%*d %s %s %s", filePath, idxPath, outputPath) == 3)
        {
            FILE *data = fopen(filePath, "rb");
            FILE *ordered = fopen(outputPath, "wb");

            if (data && ordered && order_by(data, idxPath, ordered) == SUCCESS)
            {
                fclose(data);
                fclose(ordered);

                binary_on_screen(outputPath);
            }
            else
            {
                CLOSE_FILES_FAILURE(data, ordered);
            }
        }
        break;
    default:
        printf("Invalid option! The cases go from 1 to 6.\n");
        break;
    }

    return 0;
}