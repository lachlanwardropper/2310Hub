#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>

#include "utilities.h"
#include "players.h"


/* Removes a card from the players hand and resizes
 * it to be one less than it originally was, removing the played
 * card.
 */
void remove_card(struct Card card, struct Game* game) {
    char suit = card.suit;
    int rank = card.rank;
    int counter;
    struct Card* newHand = malloc(sizeof(struct Card) * game->handSize - 1);

    for (int i = 0; i < game->handSize; i++) {
        if (game->hand[i].suit == suit && game->hand[i].rank == rank) {
            counter = i;
        }
    }

    for (int i = 0; i < counter; i++) {
        newHand[i] = game->hand[i];
    }

    for (int i = counter; i < game->handSize - 1; i++) {
        newHand[i] = game->hand[i + 1];
    }

    free(game->hand);

    game->hand = malloc(sizeof(struct Card) * game->handSize - 1);

    for (int i = 0; i < game->handSize; i++) {
        game->hand[i] = newHand[i];
    }

    free(newHand);
}

/* Finds the highest card from a given suit, specified by the order of
 * the suit array. If there are no cards in the first suit, then the 
 * next suit is checked, and so on, until at least one card of a suit
 * is found.
 */
void find_highest(struct Game* game, struct Card* cards, char suit[]) {
    int handSize = game->handSize;
    int counter = 0;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < handSize; j++) {
            if (game->hand[j].suit == suit[i]) {
                cards = realloc(cards, sizeof(struct Card) * counter + 1);
                cards[counter] = game->hand[j];
                counter++;
            } else {
                continue;
            }
        }

        if (!counter) {
            continue;
        }   

        for (int i = 1; i < counter; i++) {
            if (cards[0].rank < cards[i].rank) {
                cards[0] = cards[i];
            }   
        }
        return;
    }
    return;
}


/* Finds the lowest card from a given suit, specified by the order of
 * the suit array. If there are no cards in the first suit, then the 
 * next suit is checked, and so on, until at least one card of a suit
 * is found.
 */
void find_lowest(struct Game* game, struct Card* cards, char suit[]) {
    int handSize = game->handSize;
    int counter = 0;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < handSize; j++) {
            if (game->hand[j].suit == suit[i]) {
                cards = realloc(cards, sizeof(struct Card) * counter + 1);
                cards[counter] = game->hand[j];
                counter++;
            } else {
                continue;
            }
        }

        if (!counter) {
            continue;
        }   

        for (int i = 1; i < counter; i++) {
            if (cards[0].rank > cards[i].rank) {
                cards[0] = cards[i];
            }   
        }
        return;
    }

    return;
}


/* Attempts to find the lowest card of the specified suit, and returns true if
 * there is at least one card. Otherwise, there are no cards and false is 
 * returned.
 */
bool find_lowest_suit(struct Game* game, struct Card* cards, char suit) {
    int handSize = game->handSize;
    int counter = 0;

    for (int i = 0; i < handSize; i++) {
        if (game->hand[i].suit == suit) {
            cards = realloc(cards, sizeof(struct Card) * counter + 1);
            cards[counter] = game->hand[i];
            counter++;
        } else {
            continue;
        }
    }

    if (!counter) {
        return false;
    }

    for (int i = 1; i < counter; i++) {

        if (cards[0].rank > cards[i].rank) {
            cards[0] = cards[i];
        }
    }

    return true;
}


/* Attempts to find the highest card of the specified suit, and returns true if
 * there is at least one card. Otherwise, there are no cards and false is 
 * returned.
 */
bool find_highest_suit(struct Game* game, struct Card* cards, char suit) {
    int handSize = game->handSize;
    int counter = 0;

    for (int i = 0; i < handSize; i++) {
        if (game->hand[i].suit == suit) {
            cards = realloc(cards, sizeof(struct Card) * counter + 1);
            cards[counter] = game->hand[i];
            counter++;
        } else {
            continue;
        }
    }

    if (!counter) {
        return false;
    }

    for (int i = 1; i < counter; i++) {

        if (cards[0].rank < cards[i].rank) {
            cards[0] = cards[i];
        }
    }

    return true;
}


/* Verifies that the given card is valid, and returns the relevant
 * exit status. Status 0 is returned upon normal exit.
 */
enum ExitMessage check_valid_number(char* input, enum ExitMessage errorCode, 
        int* gameArg) {
    for (int i = 0; i < (int)strlen(input); i++) {
        if (!(input[i] >= '0' && input[i] <= '9')) {
            return errorCode;
        }
    }

    *gameArg = atoi(input);
    return NORMAL_EXIT;
}


/* Checks command line arguments are valid as defined by the spec. Returns 0
 * if the values are within a valid range. If the given values are invalid,
 * the relevant exit status is returned. 
 */
enum ExitMessage check_valid_args(struct Game* game, int argc, char** argv) {
    enum ExitMessage errorMessage = 0;

    if (argc != 5) {
        return ARGUMENT_LENGTH;
    }

    errorMessage = check_valid_number(argv[1], INVALID_PLAYERS, 
            &game->numPlayers);
    if (errorMessage) {
        return errorMessage;
    }

    errorMessage = check_valid_number(argv[2], INVALID_POSITION, 
            &game->playerId);
    if (errorMessage) {
        return errorMessage;
    }

    errorMessage = check_valid_number(argv[3], INVALID_THRESHOLD, 
            &game->threshold);
    if (errorMessage) {
        return errorMessage;
    }

    errorMessage = check_valid_number(argv[4], INVALID_HAND_SIZE, 
            &game->handSize);
    if (errorMessage) {
        return errorMessage;
    }

    if (game->numPlayers < 2) {
        return INVALID_PLAYERS;
    } else if (game->playerId < 0 || game->playerId >= game->numPlayers) {
        return INVALID_POSITION;
    } else if (game->threshold < 2) {
        return INVALID_THRESHOLD;
    } else if (game->handSize < 1) {
        return INVALID_HAND_SIZE;
    } else {
        fputs("@", stdout);
        fflush(stdout);
        return NORMAL_EXIT;
    }
}


/* Handles the initialisation of a player's hand by checking cards
 * are valid, and adding them to the player to be used throughout
 * the game.
 */
enum ExitMessage handle_new_hand(struct Game* game, char* input) {
    const char* currentValue;
    const char substring[2] = ",";
    char* card;
    unsigned int cardCounter = 0;
    char* error;
    char* currentCard;

    currentValue = &input[4];
    unsigned int numCards = strtoul(currentValue, &error, 10);
    struct Card* cardContents = malloc(sizeof(struct Card) * numCards);

    if (numCards >= 10) {
        currentValue += 3;
    } else {
        currentValue += 2;
    }

    currentCard = (char*)currentValue;
    card = strtok(currentCard, substring);


    while (card != NULL) {
        cardCounter++;
        if (cardCounter > numCards) {
            return INVALID_MESSAGE;
        }

        cardContents[cardCounter - 1].suit = card[0];
        cardContents[cardCounter - 1].rank = decode_rank(card[1]);

        card = strtok(NULL, substring);
    }

    if (cardCounter < numCards || numCards != (unsigned int)game->handSize) {
        return INVALID_MESSAGE;
    }

    game->hand = cardContents;
    return NORMAL_EXIT;
}


/* Handles the beginning of a new round by resetting the state of
 * a round, as well as checking whether this player is leader. Returns
 * status 0 on normal exit, and the relevant message otherwise.
 */
enum ExitMessage handle_new_round(struct Game* game, char* input) {
    unsigned int leadPlayer; 
    const char* leader;

    game->hasPlayed = false;

    leader = &input[8];
    leadPlayer = atoi(leader);

    if ((int)leadPlayer > game->numPlayers) {
        return INVALID_MESSAGE;
    }

    game->leadPlayer = leadPlayer;
    game->roundWinner = leadPlayer;
    game->currentPlayer = leadPlayer;
    game->roundDiamonds = 0;
    strcpy(game->cardsPlayed, "");

    if (game->leadPlayer == game->playerId) {
        make_new_move(game);
    }

    return NORMAL_EXIT;
}


/* Displays the end of round information to stderr, including
 * the leader for the round, as well as all cards played. for that
 * round.
 */
void handle_round_info(struct Game* game) {
    char message[80];
    char cardInfo[10];

    sprintf(message, "Lead player=%d:", game->leadPlayer);

    for (int i = 0; i < game->numPlayers; i++) {
        cardInfo[0] = ' ';
        cardInfo[1] = game->cardsPlayed[2 * i];
        cardInfo[2] = '.';
        cardInfo[3] = game->cardsPlayed[2 * i + 1];
        cardInfo[4] = '\0';

        strcat(message, cardInfo);
    }

    fprintf(stderr, "%s\n", message);
}


/* Calculates the number of diamond cards played in that round,
 * and adds the quantity to the winner of the previous round.
 */
void calculate_num_diamonds(struct Game* game) {
    game->numDiamondCards[game->roundWinner] += game->roundDiamonds;
}


/* Determines which player is currently leading the round, as well
 * as incrementing the number of diamonds played in each round so that
 * the value can be added to the round winner's score at the end of
 * each round.
 */
void check_round_leader(struct Game* game, char* card) {

    if (card[0] == game->leadCard.suit && 
            card[1] - '0' > game->leadCard.rank) {
        game->roundWinner = game->currentPlayer;
        game->leadCard.suit = card[0];
        game->leadCard.rank = card[1] - '0';
    }

    if (card[0] == 'D') {
        game->roundDiamonds++;
    }
}


/* Handles an input in which another player has played a card by
 * first checking whether they were the leader, and if so storing
 * their card information to determine the player's move. Otherwise,
 * the player track's the current player, and once it is their turn
 * outputs their move. Once all players have moved, the end of round
 * information is determined. Returns the relevant exit message.
 */
enum ExitMessage handle_player_move(struct Game* game, char* input) {
    char* played = "PLAYED";
    char* playerDetails = &input[6];
    char* playInfo;
    int rank;
    struct Card leadCard;
    const char substring[2] = ",";

    if (strstr(input, played) == NULL) {
        return INVALID_MESSAGE;
    }

    playInfo = strtok(playerDetails, substring);
    game->currentPlayer = atoi(playInfo);
    game->numCardsPlayed++;
    playInfo = strtok(NULL, substring);
    
    if (!valid_card(playInfo[0], playInfo[1])) {
        return INVALID_MESSAGE;
    }

    check_round_leader(game, playInfo);
    strcat(game->cardsPlayed, playInfo);

    if (game->currentPlayer == game->leadPlayer) {
        rank = (int)playInfo[1] - 48;
        leadCard.suit = playInfo[0];
        leadCard.rank = rank;
        game->leadCard = leadCard;
    }
    
    if (game->currentPlayer == game->playerId - 1 || 
            game->currentPlayer - game->playerId == game->currentPlayer) {

        if (!game->hasPlayed) {
            make_new_move(game);
        }
        game->currentPlayer = game->playerId;
        check_round_leader(game, game->currentCard);
    }

    if (game->numCardsPlayed == game->numPlayers) {
        calculate_num_diamonds(game);
        handle_round_info(game);
        game->numCardsPlayed = 0;
        game->leadPlayer = game->roundWinner;
        game->handSize--;
    }

    return NORMAL_EXIT;
}


/* Classifies the messages being inputted by the hub,
 * in order for the player to determine their next move.
 */
enum HubMessage classify_hub_message(char* input) {
    if (strstr(input, "HAND") == input) {
        return HAND;
    } else if (strstr(input, "NEWROUND") == input) {
        return NEWROUND;
    } else if (strstr(input, "PLAYED") == input) {
        return PLAYED;
    } else if (strstr(input, "GAMEOVER") == input) {
        return GAMEOVER;
    } else {
        return INVALID;
    }
}


/* Initialises each player's number of diamond cards to be zero
 * at the beginning of a new game.
 */
void initialise_num_diamonds(struct Game* game) {
    game->numDiamondCards = malloc(sizeof(int) * game->numPlayers);

    for (int i = 0; i < game->numPlayers; i++) {
        game->numDiamondCards[i] = 0;
    }
}


/* Handles the entire game once everything is initialised. If the
 * player has successully been created, then it continuously checks
 * for input from the hub, and classifies the information sent through
 * the pipe in order to determine the current state of the round. 
 * This function returns exit status 0 upon normal gameover, and the 
 * relevant exit status otherwise.
 */
enum ExitMessage play_game(struct Game* game) {
    bool isHand = false;
    bool isNewRound = false;
    char* input = malloc(sizeof(char) * 80);
    enum ExitMessage errorMessage = 0;
    game->cardsPlayed = malloc(sizeof(char) * (2 * game->numPlayers + 1));
    game->currentCard = malloc(sizeof(char) * 3);
    game->numCardsPlayed = 0;
    initialise_num_diamonds(game);

    while (1) {
        input = get_line(stdin);

        if (input == NULL) {
            return EOF_SIGNAL;
        }

        enum HubMessage hubMessage = classify_hub_message(input);
        switch (hubMessage) {
            case HAND:
                errorMessage = handle_new_hand(game, input);
                isHand = true;
                break;
            case NEWROUND:
                if (!isHand) {
                    return INVALID_MESSAGE;
                }
                errorMessage = handle_new_round(game, input);
                isNewRound = true;
                break;
            case PLAYED:
                if (!isHand || !isNewRound) {
                    return INVALID_MESSAGE;
                }
                errorMessage = handle_player_move(game, input);
                break;
            case GAMEOVER:
                return NORMAL_EXIT;
            case INVALID:
                return INVALID_MESSAGE;
        }

        if (errorMessage) {
            return errorMessage;
        }

        if (game->handSize == 0) {
            return NORMAL_EXIT;
        }
    }
}


/* Classifies and handles all error messages found in the program
 * and determines the appropriate output upon the completion of a
 * game - whether that be due to an error or a complete match.
 */
void handle_game_over(enum ExitMessage exitMessage) {
    switch (exitMessage) {
        case NORMAL_EXIT:
            break;
        case ARGUMENT_LENGTH:
            fprintf(stderr, "Usage: player players myid threshold handsize\n");
            break;
        case INVALID_PLAYERS:
            fprintf(stderr, "Invalid players\n");
            break;
        case INVALID_POSITION:
            fprintf(stderr, "Invalid position\n");
            break;
        case INVALID_THRESHOLD:
            fprintf(stderr, "Invalid threshold\n");
            break;
        case INVALID_HAND_SIZE:
            fprintf(stderr, "Invalid hand size\n");
            break;
        case INVALID_MESSAGE:
            fprintf(stderr, "Invalid message\n");
            break;
        case EOF_SIGNAL:
            fprintf(stderr, "EOF\n");
            break;
    }

    exit(exitMessage);
}


int main(int argc, char** argv) {
    enum ExitMessage errorMessage;
    struct Game game;
    
    errorMessage = check_valid_args(&game, argc, argv);
    if (errorMessage) {
        handle_game_over(errorMessage);
    }

    errorMessage = play_game(&game);
    if (errorMessage) {
        handle_game_over(errorMessage);
    }

    return 0;
}

