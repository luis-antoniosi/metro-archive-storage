#ifndef PARSE_UTILS_H
#define PARSE_UTILS_H

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
 * @brief Evaluates a string and converts it to an integer
 *
 * This function verifies if the string is NULL, empty, is a single '\r\n'
 * character or "NULO". If it's valid, parses it into an integer.
 *
 * @param str Pointer to the string
 *
 * @return Converted integer, -1 otherwise.
 */
int check_for_null(char *str);

/**
 * @brief Remakes a string of more than one word that was parsed by strtok, getting the string that is inside of quotes.
 *
 * @param str Initial string token
 * @param buff Already allocated array of char used to control the string
 *
 * @return char* Pointer to the reconstructed string or the original string pointer
 */
char *check_quotes(char *str, char *buf);

#endif