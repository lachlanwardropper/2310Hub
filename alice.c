#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>

#include "players.h"
#include "utilities.h"


/* Determines the moves of the player if they are leading the round.
 * The chosen card is then outputted to stdout for the hub to view.
 */
void determine_lead_move(struct Game* game) {
    struct Card* cards = malloc(sizeof(struct Card));
    char suitOrder[4] = {'S', 'C', 'D', 'H'};
    
    find_highest(game, cards, suitOrder);
    
    printf("PLAY%c%c\n", cards[0].suit, encode_rank(cards[0].rank));
    fflush(stdout);
    char* playedCard = malloc(sizeof(char) * 3);
    playedCard[0] = cards[0].suit;
    playedCard[1] = encode_rank(cards[0].rank);
    playedCard[2] = '\0'; 
    strcpy(game->currentCard, playedCard);
    free(playedCard);
    remove_card(cards[0], game);
    free(cards);
}


/* Determines a non-lead move of the current bob player. It
 * first checks whether an acceptable number of diamonds have
 * have been player. If not, then the player plays a move in the
 * order of suits specified. The chosen card is then outputted to stdout for
 * the hub to view.
 */
void determine_regular_move(struct Game* game) {
    struct Card* cards = malloc(sizeof(struct Card));
    char suit = game->leadCard.suit;
    char suitOrder[4] = {'D', 'H', 'S', 'C'};
    
    if (find_lowest_suit(game, cards, suit)) {
        printf("PLAY%c%c\n", cards[0].suit, encode_rank(cards[0].rank));
        fflush(stdout);
    } else {
        find_highest(game, cards, suitOrder);
        printf("PLAY%c%c\n", cards[0].suit, encode_rank(cards[0].rank));
        fflush(stdout);
    }

    game->currentCard = malloc(sizeof(char) * 3);
    char* playedCard = malloc(sizeof(char) * 3);
    playedCard[0] = cards[0].suit;
    playedCard[1] = encode_rank(cards[0].rank);
    playedCard[2] = '\0'; 
    strcpy(game->currentCard, playedCard);
    free(playedCard);
    remove_card(cards[0], game);
    free(cards);
}


/* Checks whether the lead player for the round is this current player.
 * If so, makes move based on this, and otherwise makes a non-lead move.
 */
void make_new_move(struct Game* game) {
    if (game->leadPlayer == game->playerId) {
        determine_lead_move(game);
    } else {
        determine_regular_move(game);
    }

    game->hasPlayed = true;
    strcat(game->cardsPlayed, game->currentCard);
    game->numCardsPlayed++;

}

