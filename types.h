#ifndef TYPES_H
#define TYPES_H

#define BUF_SIZE 256

#define EXPECTED_SIZE 300

#define HEADER_SIZE 17
#define REGISTER_SIZE 80

#define REGISTER_PARAM 8

#define TRASH '$'

typedef enum HeaderStatus
{
    HEADER_SUCCESS = 0,
    HEADER_FAILURE = 1
} HeaderStatus;

typedef struct Header
{
    char status;
    int top;
    int nextRRN;
    int numStations;
    int numPairStations;
} Header;

typedef enum DataStatus
{
    DATA_SUCCESS = 0,
    DATA_FAILURE = 1
} DataStatus;

typedef struct Register
{
    char removed;
    int next;

    int stationCode;
    int lineCode;

    int nextStationCode;
    int distNextStation;

    int codeIntegLine;
    int codeIntegStation;

    int sizeStationName;
    char *stationName;

    int sizeLineName;
    char *lineName;
} Register;

typedef struct SearchField
{
    char name[BUF_SIZE];
    char value[BUF_SIZE];
} SearchField;

typedef struct Pair
{
    int stationCode;
    int nextStationCode;
} Pair;

#endif