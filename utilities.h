#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>


/* Decodes a hexidecimal character into an integer value,
 * in order to easily compare scores of each individual player.
 */
int decode_rank(char value);


/* Encodes an integer (decimal) rank into hexidecimal in
 * order to easily output cards to other players/hub.
 */
char encode_rank(int value);


/* Handles the communication between a player and the hub. The function
 * checks a message sent through the pipe until it reaches a newline, or
 * EOF. If the message is invalid, NULL is returned, otherwise the message
 * sent from either party is returned.
 */
char* get_line(FILE* input);


/* Checks that a card is valid by ensuring the suit and rank are within
 * valid ranges. Returns true if valid, false otherwise.
 */
bool valid_card(char suit, char rank);


#endif

