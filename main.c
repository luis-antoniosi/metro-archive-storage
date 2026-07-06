#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include <stdarg.h> // used for close_files

#include "types.h"
#include "utils/utils.h"

#include "register/register.h"
#include "binFile/dataFile.h"
#include "binFile/indexFile.h"

#define CLOSE_FILES_FAILURE(...)            \
    {                                       \
        close_files(__VA_ARGS__, END_FILE); \
        print_file_failure();               \
    }

// TODO: maybe add gotos

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
    ORDER_BY,
    SELECT_JOIN_ORDER_BY
} Option;

static void print_file_failure()
{
    printf("Falha no processamento do arquivo.\n");
}

int main()
{
    setlocale(LC_ALL, ".UTF8"); // needed to print utf-8 characters like ç on console (not really needed after they fixed the test cases but nice to have)

    char buffer[BUF_SIZE];
    int option = -1;

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
    {
        char csvPath[BUF_SIZE];
        char binPath[BUF_SIZE];

        if (sscanf(buffer, "%*d %s %s", csvPath, binPath) != 2) // %*d -> the * ignores the int
        {
            print_file_failure();
            break;
        }

        FILE *csv = fopen(csvPath, "r");
        FILE *data = fopen(binPath, "wb+"); // wb+ because the "update_header_count" function needs to write

        if (write_data_file(csv, data) == SUCCESS)
        {
            CLOSE_FILES(csv, data);

            binary_on_screen(binPath);
        }
        else
        {
            CLOSE_FILES_FAILURE(csv, data);
        }

        break;
    }
    case PRINT_ALL:
    {
        char binPath[BUF_SIZE];

        if (sscanf(buffer, "%*d %s", binPath) != 1)
        {
            print_file_failure();
            break;
        }

        FILE *data = fopen(binPath, "rb");

        if (print_all_data(data) == SUCCESS)
        {
            CLOSE_FILES(data);
        }
        else
        {
            CLOSE_FILES_FAILURE(data);
        }

        break;
    }
    case PRINT_WHERE:
    {
        char binPath[BUF_SIZE];
        int iterations = 0;

        if (sscanf(buffer, "%*d %s %d", binPath, &iterations) != 2)
        {
            print_file_failure();
            break;
        }

        FILE *data = fopen(binPath, "rb");

        if (print_all_data_where(data, iterations) == SUCCESS)
        {
            CLOSE_FILES(data);
        }
        else
        {
            CLOSE_FILES_FAILURE(data);
        }

        break;
    }
    case DELETE_WHERE:
    {
        char binPath[BUF_SIZE];
        int iterations = 0;

        if (sscanf(buffer, "%*d %s %d", binPath, &iterations) != 2)
        {
            print_file_failure();
            break;
        }

        FILE *data = fopen(binPath, "rb+");

        if (delete_all_data_where(data, iterations) == SUCCESS)
        {
            CLOSE_FILES(data);

            binary_on_screen(binPath);
        }
        else
        {
            CLOSE_FILES_FAILURE(data);
        }

        break;
    }
    case INSERT:
    {
        char binPath[BUF_SIZE];
        int iterations = 0;

        if (sscanf(buffer, "%*d %s %d", binPath, &iterations) != 2)
        {
            print_file_failure();
            break;
        }

        FILE *data = fopen(binPath, "rb+");

        if (insert_data(data, iterations) == SUCCESS)
        {
            CLOSE_FILES(data);

            binary_on_screen(binPath);
        }
        else
        {
            CLOSE_FILES_FAILURE(data);
        }

        break;
    }
    case UPDATE_WHERE:
    {
        char binPath[BUF_SIZE];
        int iterations = 0;

        if (sscanf(buffer, "%*d %s %d", binPath, &iterations) != 2)
        {
            print_file_failure();
            break;
        }

        FILE *data = fopen(binPath, "rb+");

        if (update_data_where(data, iterations) == SUCCESS)
        {
            CLOSE_FILES(data);

            binary_on_screen(binPath);
        }
        else
        {
            CLOSE_FILES_FAILURE(data);
        }

        break;
    }
    case CREATE_INDEX:
    {
        char binPath[BUF_SIZE], idxPath[BUF_SIZE];

        if (sscanf(buffer, "%*d %s %s", binPath, idxPath) != 2)
        {
            print_file_failure();
            break;
        }

        FILE *data = fopen(binPath, "rb");
        FILE *index = fopen(idxPath, "wb+");

        if (create_index(data, index) == SUCCESS)
        {
            CLOSE_FILES(data, index);

            binary_on_screen(idxPath);
        }
        else
        {
            CLOSE_FILES_FAILURE(data, index);
        }

        break;
    }
    case SEARCH_WITH_INDEX:
    {
        char binPath[BUF_SIZE], idxPath[BUF_SIZE];
        int iterations = 0;

        if (sscanf(buffer, "%*d %s %s %d", binPath, idxPath, &iterations) != 3)
        {
            print_file_failure();
            break;
        }

        FILE *data = fopen(binPath, "rb");
        FILE *index = fopen(idxPath, "rb");

        if (search_with_index(data, index, iterations) == SUCCESS)
        {
            CLOSE_FILES(data, index);
        }
        else
        {
            CLOSE_FILES_FAILURE(data, index);
        }

        break;
    }
    case INSERT_WITH_INDEX:
    {
        char binPath[BUF_SIZE], idxPath[BUF_SIZE];
        int iterations = 0;

        if (sscanf(buffer, "%*d %s %s %d", binPath, idxPath, &iterations) != 3)
        {
            print_file_failure();
            break;
        }

        FILE *data = fopen(binPath, "rb+");
        FILE *index = fopen(idxPath, "rb+");

        if (insert_index(data, index, iterations) == SUCCESS)
        {
            CLOSE_FILES(data, index);

            binary_on_screen(binPath);
            binary_on_screen(idxPath);
        }
        else
        {
            CLOSE_FILES_FAILURE(data, index);
        }

        break;
    }
    case DELETE_WITH_INDEX:
    {
        char binPath[BUF_SIZE], idxPath[BUF_SIZE];
        int iterations = 0;

        if (sscanf(buffer, "%*d %s %s %d", binPath, idxPath, &iterations) != 3)
        {
            print_file_failure();
            break;
        }

        FILE *data = fopen(binPath, "rb+");
        FILE *index = fopen(idxPath, "rb+");

        if (delete_index(data, index, iterations) == SUCCESS)
        {
            CLOSE_FILES(data, index);

            binary_on_screen(binPath);
            binary_on_screen(idxPath);
        }
        else
        {
            CLOSE_FILES_FAILURE(data, index);
        }
    }
    break;
    case SELECT_JOIN:
    {
        char sourcePath[BUF_SIZE], joinPath[BUF_SIZE];

        if (sscanf(buffer, "%*d %s %*s %s %*s", sourcePath, joinPath) != 2)
        {
            print_file_failure();
            break;
        }

        if (select_join(sourcePath, joinPath) == FAILURE)
        {
            print_file_failure();
        }

        break;
    }
    case SELECT_JOIN_INDEX:
    {
        char sourcePath[BUF_SIZE], joinPath[BUF_SIZE], idxPath[BUF_SIZE];

        if (sscanf(buffer, "%*d %s %*s %s %*s %s", sourcePath, joinPath, idxPath) != 3)
        {
            print_file_failure();
            break;
        }

        FILE *sourceFile = fopen(sourcePath, "rb");
        FILE *joinFile = fopen(joinPath, "rb");
        FILE *indexFile = fopen(idxPath, "rb");

        if (select_join_index(sourceFile, joinFile, indexFile) == SUCCESS)
        {
            CLOSE_FILES(sourceFile, joinFile, indexFile);
        }
        else
        {
            CLOSE_FILES_FAILURE(sourceFile, joinFile, indexFile);
        }

        break;
    }
    case ORDER_BY:
    {
        char sourcePath[BUF_SIZE], orderedPath[BUF_SIZE], orderKey[BUF_SIZE];

        if (sscanf(buffer, "%*d %s %s %s", sourcePath, orderKey, orderedPath) != 3)
        {
            print_file_failure();
            break;
        }

        if (order_by(sourcePath, orderKey, orderedPath) == SUCCESS)
        {
            binary_on_screen(orderedPath);
        }
        else
        {
            print_file_failure();
        }

        break;
    }
    case SELECT_JOIN_ORDER_BY:
    {
        char sourcePath[BUF_SIZE], joinPath[BUF_SIZE];

        if (sscanf(buffer, "%*d %s %*s %s %*s", sourcePath, joinPath) != 2)
        {
            print_file_failure();
            break;
        }

        if (select_join_order_by(sourcePath, joinPath) == FAILURE)
        {
            print_file_failure();
        }

        break;
    }
    default:
        printf("Invalid option! The cases go from 1 to 14.\n");
        break;
    }

    return 0;
}