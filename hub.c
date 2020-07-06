#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>

#include "utilities.h"

#define WRITE_END 1
#define READ_END 0

// A global variable to check whether SIGHUP has been called
int signalOut = 0;


/* Defines all possible exit statuses of the hub program
 * and assigns them their relevant exit value.
 */
enum ExitMessage {
    NORMAL_EXIT = 0,
    ARGUMENT_LENGTH = 1,
    INVALID_THRESHOLD = 2,
    DECK_ERROR = 3,
    SMALL_DECK = 4,
    PLAYER_ERROR = 5,
    PLAYER_EOF = 6,
    INVALID_MESSAGE = 7,
    INVALID_CARD = 8,
    INTERRUPTED = 9,
};


/* The initial game arguments for the game.
 */
struct GameArgs {
    // The name of the deck file
    char* deckFile;
    // The threshold amount parsed to program
    int threshold;
    // The number of players parsed to program
    int playerCount;
    // The player programs to execute
    char** players;
};


/* Stores the relevant information of a card in the game.
 */
struct Card {
    // A char representing the cards suit
    char suit;
    // cards rank converted to decimal from hexadecimal
    int rank;
};


/* Stores information concerning the deck for the game.
 */
struct Deck {
    // The number of cards in the deck
    int count;
    // The cards in the deck
    struct Card* cards;
    // String representation of cards in the deck
    char** decodedCards;
};


/* Stores information regarding each player program.
 */
struct Player {
    // The player's id number
    int playerId;
    // The process id of the child
    pid_t pid;
    // The file to send information to the child
    FILE* toChild;
    // The file to receive information from the child
    FILE* fromChild;
    // The status of the child
    int status;
    // The score of the player
    int score;
    // The number of diamonds the player currently holds
    int diamondCardScore;
    // String representation of cards in hand
    char* hand;
};


/* The main way of tracking the current state of the game. Stores all
 * information concerning the game as well as handling the pipes between
 * programs.
 */
struct Game {
    // The threshold for the game
    int threshold;
    // The total number of players in the game
    int totalPlayers;
    // All player programs
    struct Player* players;
    // The size of the deck
    int deckSize;
    // The size of a hand
    int handSize;
    // The deck containing deckSize cards
    struct Deck deck;
    // The current child program
    int currentChild;
    // The player leading a round
    int leadPlayer;
    // The lead suit of the lead player
    char leadSuit;
    // The rank of the lead player's card
    int leadRank;
    // The number of diamonds in a round
    int numDiamondCards;
    // Current card being played
    struct Card currentCard;
    // Tracks the number of rounds played in the game
    int currentRound;
    // All cards played in a round
    char* currentRoundCards;
};


/* A function that sets the value of signalOut if SIGHUP has
 * been called somewhere in the program. If signalOut is greater
 * than 0, then the program is ended immediately and exited with
 * status 9.
 */
void child_handler(int s) {
    signalOut = s;
}


/* Determines the type of player programs to be executed. If the player type is
 * invalid then the relevant exit code is returned. If successful, all player
 * types are stored to be initialised later. Returns exit status 0 on success.
 */
enum ExitMessage get_player_types(int argc, char* argv[], 
        struct GameArgs* gameArgs) {
    int size = argc - 3;
    gameArgs->players = malloc(sizeof(char*) * size);
    char alice[13] = "./2310alice";
    char bob[11] = "./2310bob";

    for (int i = 0; i < size; i++) {
        if (strstr(argv[i], alice) != NULL) {
            gameArgs->players[i] = malloc(sizeof(char) * 80);
            strcpy(gameArgs->players[i], argv[i]);
        } else if (strstr(argv[i], bob) != NULL) {
            gameArgs->players[i] = malloc(sizeof(char) * 80);
            strcpy(gameArgs->players[i], argv[i]);
        } else {
            free(gameArgs->players);
            return PLAYER_ERROR;
        }
    }

    return NORMAL_EXIT;
}


/* Checks command line arguments are valid as defined by the spec. Returns 0
 * if the values are within a valid range. If the given values are invalid,
 * the relevant exit status is returned. 
 */
enum ExitMessage check_valid_args(struct GameArgs* gameArgs, int argc, 
        char* argv[]) {
    enum ExitMessage errorMessage = 0;
    if (argc < 5) {
        return ARGUMENT_LENGTH;
    }

    gameArgs->deckFile = malloc(sizeof(char) * (int)strlen(argv[1]));
    gameArgs->deckFile = argv[1];
    gameArgs->threshold = atoi(argv[2]);
    gameArgs->playerCount = argc - 3;
    errorMessage = get_player_types(argc, &argv[3], gameArgs);

    if (errorMessage) {
        gameArgs->playerCount = 0;
    }

    if (gameArgs->threshold < 2) {
        return INVALID_THRESHOLD;
    }

    return NORMAL_EXIT;
}


/* Handles the deck file by parsing each line, and checking each card is
 * valid as well as that the number of cards are a valid number. Also,
 * the number of cards stated on the first line of the file is parsed
 * and the number of cards following it are checked to ensure they match.
 */
enum ExitMessage handle_deck_file(FILE* input, struct Deck* deck) {
    const short bufferSize = (short)log10(INT_MAX) + 3;
    char buffer[bufferSize];

    if (!fgets(buffer, bufferSize - 1, input)) {
        return DECK_ERROR;
    }

    char* error;
    unsigned int deckSize = strtoul(buffer, &error, 10);

    if (*error != '\n') {
        return DECK_ERROR;
    }

    struct Card* cardContents = malloc(sizeof(struct Card) * deckSize);
    char** cardStrings = malloc(sizeof(char*) * deckSize);
    deck->decodedCards = malloc(sizeof(char*) * deckSize);
    deck->count = deckSize;

    for (unsigned i = 0; i < deckSize; ++i) {
        if (!fgets(buffer, bufferSize - 1, input)) {
            free(cardContents);
            return DECK_ERROR;
        }

        if (((buffer[2] != '\n') && (buffer[2] != '\0')) ||
                !valid_card(buffer[0], buffer[1])) {
            free(cardContents);
            return DECK_ERROR;
        }

        deck->decodedCards[i] = malloc(sizeof(char) * 4);
        char* card = malloc(sizeof(char) * 4);
        strcpy(card, buffer);
        card[2] = '\0';
        cardContents[i].suit = buffer[0];
        cardContents[i].rank = decode_rank(buffer[1]);

        cardStrings[i] = malloc(sizeof(char) * 4);
        cardStrings[i] = card;

        strcpy(deck->decodedCards[i], cardStrings[i]);
    }

    free(cardStrings);
    deck->cards = cardContents;
    fclose(input);
    return NORMAL_EXIT;
}


/* Opens a deck file and checks it exists. If it does exist, the deckfile is
 * handled. Returns 0 if a valid file, otherwise returns the relevant 
 * error status.
 */
enum ExitMessage load_deck_file(struct Deck* deck, const char* filename) {
    FILE* deckFile = fopen(filename, "r'");

    if (!deckFile) {
        return DECK_ERROR;
    }

    enum ExitMessage errorMessage = handle_deck_file(deckFile, deck);
    return errorMessage;
}


/* Initialise the pipe for both parent and child, and then stores the relevant
 * file pointer for each program to allow inter-process communication.
 */
enum ExitMessage initialise_pipe(struct Player* player, char* args[]) {
    int pipeIn[2], pipeOut[2];

    player->toChild = NULL;
    player->fromChild = NULL;

    pipe(pipeOut);
    pipe(pipeIn);

    player->pid = fork();

    if (player->pid) {
        close(pipeOut[READ_END]);
        close(pipeIn[WRITE_END]);

        player->toChild = fdopen(pipeOut[WRITE_END], "w");
        player->fromChild = fdopen(pipeIn[READ_END], "r");

        return NORMAL_EXIT;
    } else {
        close(pipeOut[WRITE_END]);
        close(pipeIn[READ_END]);
        // suppresses the stderr from player
        close(STDERR_FILENO);

        dup2(pipeOut[READ_END], STDIN_FILENO);
        dup2(pipeIn[WRITE_END], STDOUT_FILENO);

        execvp(args[0], args);

        return PLAYER_ERROR;
    }

    return NORMAL_EXIT;
}


/* Creates and executes the specified child programs as players, and also
 * opens the communication channel with players. If successful, the hub is
 * able to communicate with player programs, otherwise a player error status
 * is returned.
 */
enum ExitMessage initialise_game_players(struct Game* game, 
        struct GameArgs gameArgs) {
    char numPlayers[2];
    char playerId[2];
    char threshold[2];
    char handSize[2];
    enum ExitMessage errorMessage = 0;

    game->handSize = game->deck.count / gameArgs.playerCount;
    game->currentRound = 0;
    game->players = malloc(sizeof(struct Player) * gameArgs.playerCount);

    sprintf(numPlayers, "%d", gameArgs.playerCount);
    sprintf(threshold, "%d", gameArgs.threshold);
    sprintf(handSize, "%d", game->handSize);

    char* args[] = {NULL, numPlayers, playerId, threshold, 
            handSize, NULL};

    for (int i = 0; i < gameArgs.playerCount; i++) {

        game->players[i].playerId = i;
        game->currentChild = i;
        game->players[i].score = 0;
        game->players[i].diamondCardScore = 0;
        
        args[0] = gameArgs.players[i];
        sprintf(playerId, "%d", i);

        errorMessage = initialise_pipe(&game->players[i], args);

        if (errorMessage) {
            return errorMessage;
        }
    }

    return NORMAL_EXIT;
}


/* Kills all children in the event of a game over or
 * SIGHUP interruption. 
 */
void kill_children(struct Game* game) {
    int status = 0;

    // handle SIGHUP if occurred.
    if (signalOut) {
        for (int i = 0; i < game->totalPlayers; i++) {
            while(game->players[i].pid = waitpid(-1, 0, WNOHANG),
                    game->players[i].pid > 0) {
                kill(game->players[1].pid, SIGKILL); 
            }
        }

        fprintf(stderr, "Ended due to signal\n");
        exit(9);
    }

    for (int i = 0; i < game->totalPlayers; i++) {
        fprintf(game->players[i].toChild, "GAMEOVER\n");
        fflush(game->players[i].toChild);

        // brute force
        if (!waitpid(game->players[i].pid, &status, WNOHANG)) {
            kill(game->players[1].pid, SIGKILL);
            continue;
        }
    }
}


/* Resizes the games deck after each player's hand is initialised.
 */
void resize_deck(struct Game* game) {
    game->deck.count = game->deck.count - game->handSize;

    for (int i = 0; i < game->deck.count; i++) {
        game->deck.decodedCards[i] = 
                game->deck.decodedCards[i + game->handSize];
    }

    game->deck.decodedCards = realloc(game->deck.decodedCards, 
            (sizeof(char*) * game->deck.count));
}


/* Sends a player their initial hand, by removing the first (hand size)
 * number of cards from the deck.
 */
void send_initial_hand(struct Game* game) {
    char* nextCard = malloc(sizeof(char) * 3);
    char* buffer = malloc(sizeof(char) * (3 * game->handSize + 8));
    char playerHand[80];
    int skip;

    for (int i = 0; i < game->totalPlayers; i++) {
        strcpy(buffer, "");
        sprintf(buffer, "HAND%d,", game->handSize);
        skip = i * game->handSize;
        game->players[i].hand = malloc(sizeof(char) * 
                (game->handSize * 2) + 2);

        for (int j = 0; j < game->handSize - 1; j++) {
            strcpy(nextCard, game->deck.decodedCards[j + skip]);
            nextCard[2] = '\0';

            if (j == 0) {
                strcpy(playerHand, nextCard);
            } else {
                strcat(playerHand, nextCard);
            }

            strcat(buffer, nextCard);
            strcat(buffer, ",");
        }

        strcpy(nextCard, game->deck.decodedCards[game->handSize - 1 + skip]);
        strcat(buffer, nextCard);
        strcat(buffer, "\n");

        strcat(playerHand, nextCard);
        strcpy(game->players[i].hand, playerHand);

        fputs(buffer, game->players[i].toChild);
        fflush(game->players[i].toChild);  
    }

    free(buffer);
    free(nextCard);  
}


/* Initialises a new game by checking all inputs are valid, and assigns their
 * values to the relevant structs. If the game is succeessfully, it is exited
 * normally with exit status 0, otherwise the relevant error status is 
 * returned.
 */
enum ExitMessage initialise_new_game(struct Game* game, 
        struct GameArgs gameArgs) {
    enum ExitMessage errorMessage;

    game->threshold = gameArgs.threshold;
    game->totalPlayers = gameArgs.playerCount;

    errorMessage = load_deck_file(&game->deck, gameArgs.deckFile);

    if (game->deck.count < gameArgs.playerCount) {
        return DECK_ERROR;
    }

    if (errorMessage) {
        return errorMessage;
    }

    if (game->deck.count < game->totalPlayers) {
        return SMALL_DECK;
    }

    errorMessage = initialise_game_players(game, gameArgs);

    if (errorMessage) {
        kill_children(game);
    }

    send_initial_hand(game);
    return NORMAL_EXIT;
}


/* Sends each player a message informing them of a new round and
 * the lead player for that round.
 */
void new_round(struct Game* game) {
    char buffer[20];
    int leader = game->leadPlayer;

    printf("Lead player=%d\n", game->leadPlayer);

    for (int i = 0; i < game->totalPlayers; i++) {
        sprintf(buffer, "NEWROUND%d\n", leader);
        fputs(buffer, game->players[i].toChild);
        fflush(game->players[i].toChild);
    }
}


/* Checks whether the current game is over by determining whether the
 * player's hands are emtpy.
 */
int is_game_over(const struct Game* game) {
    if (game->handSize == 0) {
        return 1;
    }
    return 0;
}


/* Checks whether a card being played by a player is valid.
 */
enum ExitMessage check_valid_card(struct Game* game, int player, char suit, 
        int rank) {
    int counter;
    int isValid = 0;
    char buffer[80];

    for (int i = 0; i < game->handSize; i++) {
        if (game->players[player].hand[2 * i] == suit && 
                game->players[player].hand[2 * i + 1] == encode_rank(rank)) {
            counter = i;
            isValid = 1;
        }
    }

    if (!isValid) {
        return INVALID_CARD;
    }

    for (int i = 0; i < counter; i++) {
        buffer[2 * i] = game->players[player].hand[2 * i];
        buffer[2 * i + 1] = game->players[player].hand[2 * i + 1];
    }

    if (counter == game->handSize - 1) {
        buffer[2 * counter] = game->players[player].hand[2 * counter + 2];
        buffer[2 * counter + 1] = game->players[player].hand[2 * counter + 3];
    } else {
        for (int i = counter; i < game->handSize - 1; i++) {
            buffer[2 * i] = game->players[player].hand[2 * i + 2];
            buffer[2 * i + 1] = game->players[player].hand[2 * i + 3];

        }   
    }

    buffer[game->handSize * 2 - 2] = '\0';
    game->players[player].hand = realloc(game->players[player].hand, 
            sizeof(buffer));
    strcpy(game->players[player].hand, buffer);
    return NORMAL_EXIT;
}


/* Handles a message sent from the player to the hub, and keeps track 
 * of the leading player & card for the round.
 */
enum ExitMessage handle_player_message(struct Game* game, char* input, 
        int player) {
    char* playedHand;
    char suit;
    int rank;
    enum ExitMessage errorMessage = 0;
    char card[3];

    if (game->currentRound == 0) {
        playedHand = &input[5];
    } else {
        playedHand = &input[4];
    }

    if (!strstr(input, "PLAY")) {
        return INVALID_MESSAGE;
    }

    suit = playedHand[0];
    rank = decode_rank(playedHand[1]);

    errorMessage = check_valid_card(game, player, suit, rank);

    if (suit == 'D') {
        game->numDiamondCards++;
    }

    card[0] = playedHand[0];
    card[1] = playedHand[1];
    card[2] = '\0';
    strcat(game->currentRoundCards, card);

    if (errorMessage) {
        return errorMessage;
    }

    if (player == game->leadPlayer) {
        game->leadRank = rank;
        game->leadSuit = suit;
    } else {
        if (suit == game->leadSuit && rank > game->leadRank) {
            game->leadPlayer = player;
        }
    }
    game->currentCard.suit = suit;
    game->currentCard.rank = rank;

    return NORMAL_EXIT;
}


/* Handles the scoring for a player at the end of each round.
 * Does so by adding the number of diamonds played in that round to
 * the lead player, as well as adding to their score. Also handles the
 * output of the score for each player to stdout at the end of a round.
 */
void handle_round_score(struct Game* game) {
    int leader = game->leadPlayer;
    char buffer[80];
    char card[5];

    game->players[leader].score++;
    game->players[leader].diamondCardScore += game->numDiamondCards;

    game->numDiamondCards = 0;

    sprintf(buffer, "Cards=");
    
    for (int i = 0; i < game->totalPlayers - 1; i++) {
        card[0] = game->currentRoundCards[2 * i];
        card[1] = '.';
        card[2] = game->currentRoundCards[2 * i + 1];
        card[3] = ' ';
        card[4] = '\0';

        strcat(buffer, card);
    }

    card[0] = game->currentRoundCards[2 * (game->totalPlayers - 1)];
    card[1] = '.';
    card[2] = game->currentRoundCards[2 * (game->totalPlayers - 1) + 1];
    card[3] = '\0';

    strcat(buffer, card);
    fprintf(stdout, "%s\n", buffer);
    fflush(stdout);
}


/* Sends a player's move to all other players in the game so that they
 * can determine what move to make.
 */
void send_player_from_hub(struct Game* game, int currentPlayer) {
    char message[20];
    char rank = encode_rank(game->currentCard.rank);
    char suit = game->currentCard.suit;
    FILE* toChild;

    sprintf(message, "PLAYED%d,%c%c\n", currentPlayer, suit, rank);

    for (int i = 0; i < game->totalPlayers; i++) {
        if (i != currentPlayer) {
            toChild = game->players[i].toChild;
            fputs(message, toChild);
            fflush(toChild);
        }
    }
}


/* Handles a player's move by checking the message sent to the hub
 * is valid, and then outputting it to all other player programs.
 */
enum ExitMessage handle_player_moves(struct Game* game, int startIndex, 
        int endIndex) {
    enum ExitMessage errorMessage = 0;
    char* input;
    input = malloc(sizeof(char) * 80);

    for (int i = startIndex; i < endIndex; i++) {

        input = get_line(game->players[i].fromChild);

        if (input == NULL) {
            if (feof(game->players[i].fromChild)) {
                return PLAYER_EOF;
            } else {
                return INVALID_MESSAGE;
            }
        }

        errorMessage = handle_player_message(game, input, i);

        if (errorMessage) {
            return errorMessage;
        }

        send_player_from_hub(game, i);
    }

    return errorMessage;
}


/* Calculates the final score for each player. dependent on diamonds won.
 * If the number of diamonds are above the threshold, then they are added to
 * the score, otherwise they are subtracted.
 */
void handle_final_score(struct Game* game) {
    for (int i = 0; i < game->totalPlayers; i++) {
        if (game->players[i].diamondCardScore < game->threshold) {
            game->players[i].score -= game->players[i].diamondCardScore;
        } else {
            game->players[i].score += game->players[i].diamondCardScore;
        }
    }
}


/* Outputs the score of each player to stdout at the end of a complete
 * and successful game. Scores are outputted in order of player id.
 */
void output_final_score(struct Game* game) {
    char buffer[10];
    int size = game->totalPlayers - 1;
    char* output = malloc(sizeof(char) * (game->totalPlayers * 4));
    strcpy(output, "");

    handle_final_score(game);

    for (int i = 0; i < size; i++) {
        sprintf(buffer, "%d:%d ", i, game->players[i].score);
        strcat(output, buffer);
    }

    sprintf(buffer, "%d:%d", size, game->players[size].score);
    strcat(output, buffer);
    printf("%s\n", output);
    fflush(stdout);
    free(output);
}


/* Handles the duration of the game once it has successfully been
 * initialised. Each round is played until the game is complete
 * or an error message is returned. 
 */
enum ExitMessage play_game(struct Game* game) {
    enum ExitMessage errorMessage = 0;
    game->leadPlayer = 0;
    game->numDiamondCards = 0;
    game->currentRoundCards = malloc(sizeof(char) * 
            (2 * game->totalPlayers) + 1);

    strcpy(game->currentRoundCards, "");

    while (!is_game_over(game)) {
        int leader = game->leadPlayer;
        new_round(game);

        handle_player_moves(game, leader, game->totalPlayers);
        handle_player_moves(game, 0, leader);

        handle_round_score(game);
        game->handSize--;
        strcpy(game->currentRoundCards, "");

        game->currentRound++;
    }

    output_final_score(game);
    free(game->currentRoundCards);
    return errorMessage;
}


/* Exits the program and handles the relevant error by printing
 * an error message to stderr.
 */
void handle_game_over(enum ExitMessage errorMessage) {
    switch (errorMessage) {
        case NORMAL_EXIT:
            break;
        case ARGUMENT_LENGTH:
            fprintf(stderr, 
                    "Usage: 2310hub deck threshold player0 {player1}\n");
            break;
        case INVALID_THRESHOLD:
            fprintf(stderr, "Invalid threshold\n");
            break;
        case DECK_ERROR:
            fprintf(stderr, "Deck error\n");
            break;
        case SMALL_DECK:
            fprintf(stderr, "Not enough cards\n");
            break;
        case PLAYER_ERROR:
            fprintf(stderr, "Player error\n");
            break;
        case PLAYER_EOF:
            fprintf(stderr, "Player EOF\n");
            break;
        case INVALID_MESSAGE:
            fprintf(stderr, "Invalid message\n");
            break;
        case INVALID_CARD:
            fprintf(stderr, "Invalid card choice\n");
            break;
        case INTERRUPTED:
            fprintf(stderr, "Ended due to signal\n");
            break;
    }

    exit(errorMessage);
}


int main(int argc, char** argv) {
    enum ExitMessage errorMessage;
    struct GameArgs gameArgs;
    struct Game game;

    // set up sigaction to handle SIGHUP
    struct sigaction sig;
    memset(&sig, 0, sizeof(sig));
    sig.sa_handler = child_handler;
    sig.sa_flags = SA_NOCLDSTOP;
    sigaction(SIGHUP, &sig, NULL);

    errorMessage = check_valid_args(&gameArgs, argc, argv);
    if (errorMessage) {
        handle_game_over(errorMessage);
    }

    errorMessage = initialise_new_game(&game, gameArgs);
    if (errorMessage) {
        handle_game_over(errorMessage);
    }

    errorMessage = play_game(&game);

    kill_children(&game);

    return NORMAL_EXIT;
}

