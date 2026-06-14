#ifndef TYPES_H
#define TYPES_H

//-----------------------------------------------//
//                    CONSTANTS                  //
//----------------------------------------------//

#define BUF_SIZE        256 // set buffer size used for strings
#define EXPECTED_SIZE   300 // used in header.c, malloc of seenStations and seenPairs

// fixed sizes defined by the project specifications
#define HEADER_SIZE     17
#define REGISTER_SIZE   80

// file status and trash character
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

#endif