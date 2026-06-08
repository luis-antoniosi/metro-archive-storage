#ifndef UTILS_H
#define UTILS_H

/**
 * @brief Optimized strtok to deal with consecutive delimiter characters
 * @param buff Double pointer to the string to be parsed. The inside pointer is
 *  updated in each call to point to the beggining of the next token
 * @param delim Delimiter char
 * @return Pointer to the beggining of the token, or NULL if the string ended
 *  or buff is invalid.
 */
char *custom_strtok(char **buff, char delim);

/**
 * @brief evaluates a string and converts it to an integer
 *
 * This function verifies if the string is NULL, empty or is a single '\n'
 * character. If is valid, it parses it into a integer.
 *
 * @param str Pointer to the string
 *
 * @return Converted integer or -1 if the string is NULL, empty or '\n'
 */
int check_for_null(char *str);

/**
 * @brief remakes a string of more than one word that was parsed by strtok
 *
 * @param str initial string token
 * @param buff already allocated array of char used to controy the string
 *
 * @return char* Pointer to the reconstructed string or the original string pointer
 */
char *check_quotes(char *str, char *buf);

#endif