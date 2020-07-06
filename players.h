#ifndef PLAYERS_H
#define PLAYERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>


/* Handles all possible exit statuses of player program.
 */
enum ExitMessage {
    NORMAL_EXIT = 0,
    ARGUMENT_LENGTH = 1,
    INVALID_PLAYERS = 2,
    INVALID_POSITION = 3,
    INVALID_THRESHOLD = 4,
    INVALID_HAND_SIZE = 5,
    INVALID_MESSAGE = 6,
    EOF_SIGNAL = 7,
};


/* Classifies all valid messages from the hub.
 */
enum HubMessage {
    HAND,
    NEWROUND,
    PLAYED,
    GAMEOVER,
    INVALID,
};


/* Stores the relevant information of a card in the game.
 */
struct Card {
    // The suit of a card
    char suit;
    // The rank of a card converted from hexadecimal to decimal
    int rank;
};


/* The main way of tracking the current state of the game. Stores all
 * information concerning the game in order to determine the current
 * leaders and progress through a match.
 */
struct Game {
    // The number of players in a game
    int numPlayers;
    // The id of a player in the game
    int playerId;
    // The threshold number of cards
    int threshold;
    // The hand size of a player
    int handSize;
    // The player leading a round
    int leadPlayer;
    // The player currently playing a card
    int currentPlayer;
    // The hand of this player
    struct Card* hand;
    // The lead card for a round
    struct Card leadCard;
    // The number of cards played in a round 
    int numCardsPlayed;
    // The cards played in a round as a String
    char* cardsPlayed;
    // The current card played by the last player
    char* currentCard;
    // The number of diamond cards the player holds
    int* numDiamondCards;
    // The winner of a round
    int roundWinner;
    // The number of diamonds in a certain round
    int roundDiamonds;
    // Whether this player has played their hand or not in a round
    bool hasPlayed;
};


/* Removes a card from the players hand and resizes
 * the previous hand to the correct size.
 */
void remove_card(struct Card card, struct Game* game);


/* Finds the highest card from a given suit, specified by the order of
 * the suit array. If there are no cards in the first suit, then the 
 * next suit is checked, and so on, until at least one card of a suit
 * is found.
 */
void find_highest(struct Game* game, struct Card* cards, char suit[]);


/* Finds the lowest card from a given suit, specified by the order of
 * the suit array. If there are no cards in the first suit, then the 
 * next suit is checked, and so on, until at least one card of a suit
 * is found.
 */
void find_lowest(struct Game* game, struct Card* cards, char suit[]);


/* Attempts to find the lowest card of the specified suit, and returns true if
 * there is at least one card. Otherwise, there are no cards and false is 
 * returned.
 */
bool find_lowest_suit(struct Game* game, struct Card* cards, char suit);


/* Attempts to find the highest card of the specified suit, and returns true if
 * there is at least one card. Otherwise, there are no cards and false is 
 * returned.
 */
bool find_highest_suit(struct Game* game, struct Card* cards, char suit);


/* Verifies that the given card is valid, and returns the relevant
 * exit status. Status 0 is returned upon normal exit.
 */
enum ExitMessage check_valid_number(char* input, enum ExitMessage errorCode, 
        int* gameArg);


/* Checks command line arguments are valid as defined by the spec. Returns 0
 * if the values are within a valid range. If the given values are invalid,
 * the relevant exit status is returned. 
 */
enum ExitMessage check_valid_args(struct Game* game, int argc, char** argv);


/* Handles the initialisation of a player's hand by checking cards
 * are valid, and adding them to the player to be used throughout
 * the game.
 */
enum ExitMessage handle_new_hand(struct Game* game, char* input);


/* Handles the beginning of a new round by resetting the state of
 * a round, as well as checking whether this player is leader. Returns
 * status 0 on normal exit, and the relevant message otherwise.
 */
enum ExitMessage handle_new_round(struct Game* game, char* input);


/* Displays the end of round information to stderr, including
 * the leader for the round, as well as all cards played. for that
 * round.
 */
void handle_round_info(struct Game* game);


/* Calculates the number of diamond cards played in that round,
 * and adds the quantity to the winner of the previous round.
 */
void calculate_num_diamonds(struct Game* game);


/* Determines which player is currently leading the round, as well
 * as incrementing the number of diamonds played in each round so that
 * the value can be added to the round winner's score at the end of
 * each round.
 */
void check_round_leader(struct Game* game, char* card);


/* Checks whether the lead player for the round is this current player.
 * If so, makes move based on this, and otherwise makes a non-lead move.
 */
void make_new_move(struct Game* game);


/* Handles an input in which another player has played a card by
 * first checking whether they were the leader, and if so storing
 * their card information to determine the player's move. Otherwise,
 * the player track's the current player, and once it is their turn
 * outputs their move. Once all players have moved, the end of round
 * information is determined. Returns the relevant exit message.
 */
enum ExitMessage handle_player_move(struct Game* game, char* input);


/* Classifies the messages being inputted by the hub,
 * in order for the player to determine their next move.
 */
enum HubMessage classify_hub_message(char* input);


/* Initialises each player's number of diamond cards to be zero
 * at the beginning of a new game.
 */
void initialise_num_diamonds(struct Game* game);


/* Handles the entire game once everything is initialised. If the
 * player has successully been created, then it continuously checks
 * for input from the hub, and classifies the information sent through
 * the pipe in order to determine the current state of the round. 
 * This function returns exit status 0 upon normal gameover, and the 
 * relevant exit status otherwise.
 */
enum ExitMessage play_game(struct Game* game);


/* Classifies and handles all error messages found in the program
 * and determines the appropriate output upon the completion of a
 * game - whether that be due to an error or a complete match.
 */
void handle_game_over(enum ExitMessage exitMessage);


#endif
