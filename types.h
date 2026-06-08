#ifndef TYPES_H
#define TYPES_H

//-----------------------------------------------//
//                    CONSTANTS                  //
//----------------------------------------------//

#define BUF_SIZE 256
#define EXPECTED_SIZE 300

//fixed sizes defined by the project specifications
#define HEADER_SIZE 17
#define REGISTER_SIZE 80

//states and trash character
#define STATUS_INCONSISTENT '0'
#define STATUS_CONSISTENT '1'
#define TRASH '$'

//----------------------------------//
//             ENUMS               //
//---------------------------------//

/**
 * @enum Status
 * @brief Indicates success or failure in functions.
 */
typedef enum Status
{
    SUCCESS = 0,
    FAILURE = 1
} Status;

//-------------------------------------//
//        DATA FILE STRUCTURES         //
//-------------------------------------//

/**
 * @struct Header
 * @brief Represents the header record of the binary data file.
 */
typedef struct Header
{
    char status; //0 for inconsisted, 1 for consistent
    int top; //RRN of the top of the removed stack (-1 if empty)
    int nextRRN; //next available RRN
    int numStations; //number of unique stations 
    int numPairStations; //number of unique station pairs
} Header;

/**
 * @struct Register
 * @brief Represents a station data record, mapping its logical fields.
 */
typedef struct Register
{
    char removed; //1 if logically removed, 0 if not
    int next; //RRN of next removed record in the stack, or -1

    int stationCode; //primary key, unique station identifier code
    int lineCode; //line identifier code

    int nextStationCode; //code of next station on the line
    int distNextStation; //distance to the next station

    int codeIntegLine; //integration line code
    int codeIntegStation; //integration station code

    int sizeStationName; //size in bytes of the station name string
    char *stationName; //station name (variable size)

    int sizeLineName; //size in bytes of the line name string
    char *lineName; //line name (variable size)
} Register;

//-----------------------------------//
//       AUXILIARY STRUCTURES        //
//----------------------------------//

/**
 * @struct SearchField
 * @brief Stores the pair (Field Name, Field Value) for search, update, or deletion operations.
 */
typedef struct SearchField
{
    char name[BUF_SIZE]; //field name
    char value[BUF_SIZE]; //searched value
} SearchField;

/**
 * @struct Pair
 * @brief Auxiliary structure to count unique pairs of stations.
 */
typedef struct Pair
{
    int stationCode; //code of the first station of the pair
    int nextStationCode; //code of the second station of the pair
} Pair;

#endif