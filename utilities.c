#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>

#include "utilities.h"


/* Decodes a hexidecimal character into an integer value,
 * in order to easily compare scores of each individual player.
 */
int decode_rank(char value) {
    if (value >= '1' && value <= '9') {
        return value - '0';
    }
    
    switch (value) {
        case 'a':
            return 10;
            break;
        case 'b':
            return 11;
            break;
        case 'c':
            return 12;
            break;
        case 'd':
            return 13;
            break;
        case 'e':
            return 14;
            break;
        case 'f':
            return 15;
            break;
    }

    return value - '0';
}


/* Encodes an integer (decimal) rank into hexidecimal in
 * order to easily output cards to other players/hub.
 */
char encode_rank(int value) {
    if (value < 10) {
        return value + '0';
    }

    switch (value) {
        case 10:
            return 'a';
            break;
        case 11:
            return 'b';
            break;
        case 12:
            return 'c';
            break;
        case 13:
            return 'd';
            break;
        case 14:
            return 'e';
            break;
        case 15:
            return 'f';
            break;
    }

    return value + '0';
}


/* Handles the communication between a player and the hub. The function
 * checks a message sent through the pipe until it reaches a newline, or
 * EOF. If the message is invalid, NULL is returned, otherwise the message
 * sent from either party is returned.
 */
char* get_line(FILE* input) {
    int size = 80;
    char* message = malloc(sizeof(char) * size);
    int index = 0;
    int i = 0;

    while(1) {
        i = fgetc(input);

        if (i == EOF) {
            return NULL;
        }
        
        if (i == '\n') {
            message[index] = '\0';
            return message;
        }
       
        message[index++] = (char)i;

        if (index > size - 1) {
            size *= 2;
            message = realloc(message, sizeof(char) * size);
        }
    }
}


/* Checks that a card is valid by ensuring the suit and rank are within
 * valid ranges. Returns true if valid, false otherwise.
 */
bool valid_card(char suit, char rank) {
    return (((suit == 'C') || (suit == 'D') ||
            (suit == 'S') || (suit == 'H')) && ((rank >= '1' && rank <= '9') ||
            (rank >= 'a' && rank <= 'f')));
}
