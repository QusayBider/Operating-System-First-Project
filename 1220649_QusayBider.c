/*

1)Naive approach

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_WORD_LENGTH 50
#define MAX_WORDS 2000000  // Set a max limit for the number of words to process

int size = 0;  // To track number of unique words

// Define a structure for storing word and its frequency
struct Array {
    char word[MAX_WORD_LENGTH];
    int frequency;
};

// Array to store inserted words
struct Array Insert_Array[MAX_WORDS];

// Function to check if a word exists in the Insert_Array and update its frequency
int checkIfExist(char *word) {
    for (int i = 0; i < size; i++) {
        if (strcmp(Insert_Array[i].word, word) == 0) {
            Insert_Array[i].frequency++;  // Increment frequency if word exists
            return 1;  // Word found
        }
    }
    return 0;  // Word not found
}

// Function to load the data from the file
void loadData(char *filename) {
    FILE *in = fopen(filename, "r");
    if (in == NULL) {
        printf("Error! Can't open the file\n");
        return;
    }

    char word[MAX_WORD_LENGTH];
    while (fscanf(in, "%49s", word) == 1) {  // Read word by word

        // If the word doesn't exist, add it to Insert_Array
        if (!checkIfExist(word)) {
            strncpy(Insert_Array[size].word, word, MAX_WORD_LENGTH - 1);
            Insert_Array[size].word[MAX_WORD_LENGTH - 1] = '\0';  // Null-terminate the word
            Insert_Array[size].frequency = 1;
            size++;
        }
    }

    fclose(in);
    printf("Data loaded successfully\n");
}

// Function to get the top 10 most frequent words
void getTop10() {
    struct Array top10[10] = {{0}};  // Array to store the top 10 frequent words

    // Traverse through all Insert_Array entries and find the top 10 frequent words
    for (int i = 0; i < size; i++) {
        // Insert the word into the top10 list if its frequency is high enough
        for (int j = 0; j < 10; j++) {
            if (top10[j].frequency == 0 || Insert_Array[i].frequency > top10[j].frequency) {
                // Shift lower frequency words down
                for (int k = 9; k > j; k--) {
                    top10[k] = top10[k - 1];
                }
                top10[j] = Insert_Array[i];
                break;
            }
        }
    }

    // Print the top 10 most frequent words
    printf("Top 10 most frequent words:\n");
    for (int i = 0; i < 10; i++) {
        if (top10[i].frequency > 0) {
            printf("%s: %d\n", top10[i].word, top10[i].frequency);
        }
    }
}

int main(void) {
    clock_t start, end;

    // Load data and measure time
    loadData("text8.txt");  // Specify the path to your large file

    // Get Top 10 most frequent words and measure time
    getTop10();

    return 0;
}

*/


/*

2)Multiprocessing approach

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <limits.h>

#define MAX_WORD_LENGTH 50
#define MAX_WORDS 2000000  // Set a max limit for the number of words to process

int cword;
int numofchildren;  // Number of child processes

// Define a structure for storing word and its frequency
struct Array {
    char word[MAX_WORD_LENGTH];
    int frequency;
};

// Array to store inserted words in shared memory
struct Array* Insert_Array;

// Semaphore for synchronization
int sem_id;

// Shared size variable to track the number of unique words
int* shared_size;

// Function to convert word to uppercase
char *strupr(char *str) {
    char *p = str;
    while (*p) {
        *p = toupper((unsigned char)*p);  // Convert each character to uppercase
        p++;
    }
    return str;
}

// Function to check if a word exists in the Insert_Array and update its frequency
int checkIfExist(char *word) {
    for (int i = 0; i < *shared_size; i++) {
        if (strcmp(Insert_Array[i].word, word) == 0) {
            Insert_Array[i].frequency++;  // Increment frequency if word exists
            return 1;  // Word found
        }
    }
    return 0;  // Word not found
}

// Function to load the data from the file
void loadData(char *filename, int start, int end) {
    FILE *in = fopen(filename, "r");
    if (in == NULL) {
        printf("Error! Can't open the file\n");
        return;
    }

    char word[MAX_WORD_LENGTH];
    int word_count = 0;

    // Skip the first 'start' words
    while (word_count < start && fscanf(in, "%49s", word) == 1) {
        word_count++;
    }

    // Read the words between start and end
    while (word_count < end && fscanf(in, "%49s", word) == 1) {
        strupr(word);  // Convert word to uppercase
        // If the word doesn't exist, add it to Insert_Array
        if (!checkIfExist(word)) {
            strncpy(Insert_Array[*shared_size].word, word, MAX_WORD_LENGTH - 1);
            Insert_Array[*shared_size].word[MAX_WORD_LENGTH - 1] = '\0';  // Null-terminate the word
            Insert_Array[*shared_size].frequency = 1;
            (*shared_size)++;  // Increment the shared size variable
        }
        word_count++;
    }

    fclose(in);
}

// Function to get the top 10 most frequent words
void getTop10() {
    struct Array top10[10] = {{0}};  // Array to store the top 10 frequent words

    // Traverse through all Insert_Array entries and find the top 10 frequent words
    for (int i = 0; i < *shared_size; i++) {
        // Insert the word into the top10 list if its frequency is high enough
        for (int j = 0; j < 10; j++) {
            if (top10[j].frequency == 0 || Insert_Array[i].frequency > top10[j].frequency) {
                // Shift lower frequency words down
                for (int k = 9; k > j; k--) {
                    top10[k] = top10[k - 1];
                }
                top10[j] = Insert_Array[i];
                break;
            }
        }
    }

    // Print the top 10 most frequent words
    printf("Top 10 most frequent words:\n");
    for (int i = 0; i < 10; i++) {
        if (top10[i].frequency > 0) {
            printf("%s: %d\n", top10[i].word, top10[i].frequency);
        }
    }
}

// Function to count the total words in the file
int countwords(char *filename) {
    FILE *fh = fopen(filename, "r");
    if (fh == NULL) {
        printf("The file does not exist\n");
        return 0;
    } else {
        int count = 0;
        char temp[100];
        while (fscanf(fh, "%s", temp) != EOF) {
            count++;
        }
        fclose(fh);
        return count;
    }
}

// Semaphore wait (P) operation
void sem_wait() {
    struct sembuf sop;
    sop.sem_num = 0;
    sop.sem_op = -1;  // Decrement the semaphore
    sop.sem_flg = 0;
    semop(sem_id, &sop, 1);
}

// Semaphore signal (V) operation
void sem_signal() {
    struct sembuf sop;
    sop.sem_num = 0;
    sop.sem_op = 1;  // Increment the semaphore
    sop.sem_flg = 0;
    semop(sem_id, &sop, 1);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <filename> <num_children>\n", argv[0]);
        return 1;
    }

    char* filename = argv[1];
    numofchildren = atoi(argv[2]);
    cword = countwords(filename);

    // Create shared memory
    int shm_id = shmget(IPC_PRIVATE, sizeof(struct Array) * MAX_WORDS, IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("shmget failed");
        return 1;
    }

    Insert_Array = (struct Array*)shmat(shm_id, NULL, 0);
    if (Insert_Array == (void*)-1) {
        perror("shmat failed");
        return 1;
    }

    // Create shared memory for size
    int size_shm_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (size_shm_id < 0) {
        perror("shmget failed for shared size");
        return 1;
    }

    shared_size = (int*)shmat(size_shm_id, NULL, 0);
    if (shared_size == (void*)-1) {
        perror("shmat failed for shared size");
        return 1;
    }

    *shared_size = 0;  // Initialize shared size

    // Create a semaphore for synchronization
    sem_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (sem_id < 0) {
        perror("semget failed");
        return 1;
    }

    // Initialize the semaphore to 1
    semctl(sem_id, 0, SETVAL, 1);


    int words_per_child = cword / numofchildren;
    for (int i = 0; i < numofchildren; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            int start = i * words_per_child;
            int end = (i == numofchildren - 1) ? cword : (i + 1) * words_per_child;
            loadData(filename, start, end);
            exit(0);
        }
    }

    // Wait for all child processes to finish
    for (int i = 0; i < numofchildren; i++) {
        wait(NULL);
    }

    // Get and display the top 10 most frequent words
    getTop10();

    // Detach and remove shared memory and semaphore
    shmdt(Insert_Array);
    shmctl(shm_id, IPC_RMID, NULL);
    shmdt(shared_size);
    shmctl(size_shm_id, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);

    return 0;
}


*/

/*

3)Multithreading approach

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define MAX_WORD_LENGTH 50
#define MAX_WORDS 2000000  // Set a max limit for the number of words to process

int size = 0;  // To track number of unique words
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex to protect shared data structure

// Define a structure for storing word and its frequency
struct Array {
    char word[MAX_WORD_LENGTH];
    int frequency;
};

// Array to store inserted words
struct Array Insert_Array[MAX_WORDS];

// Structure to hold information for each thread (filename and offset)
struct ThreadData {
    char *filename;
    long offset;
    long chunk_size;
};

// Function to check if a word exists in the Insert_Array and update its frequency
int checkIfExist(char *word) {
    for (int i = 0; i < size; i++) {
        if (strcmp(Insert_Array[i].word, word) == 0) {
            Insert_Array[i].frequency++;  // Increment frequency if word exists
            return 1;  // Word found
        }
    }
    return 0;  // Word not found
}

// Thread function to load data from a specific file segment
void *loadDataThread(void *arg) {
    struct ThreadData *data = (struct ThreadData *)arg;
    FILE *in = fopen(data->filename, "r");
    if (in == NULL) {
        printf("Error! Can't open the file: %s\n", data->filename);
        pthread_exit(NULL);
    }

    // Seek to the portion of the file that this thread will process
    fseek(in, data->offset, SEEK_SET);

    char word[MAX_WORD_LENGTH];
    long bytes_read = 0;

    while (fscanf(in, "%49s", word) == 1) {
        bytes_read += strlen(word) + 1;  // Update the bytes read (including space or newline)

        // Stop if the thread has read its chunk size
        if (bytes_read >= data->chunk_size) {
            break;
        }


        // If the word doesn't exist, add it to Insert_Array
        if (!checkIfExist(word)) {
            strncpy(Insert_Array[size].word, word, MAX_WORD_LENGTH - 1);
            Insert_Array[size].word[MAX_WORD_LENGTH - 1] = '\0';  // Null-terminate the word
            Insert_Array[size].frequency = 1;
            size++;
        }
    }

    fclose(in);
    pthread_exit(NULL);  // Properly exit the thread
}

// Function to get the top 10 most frequent words
void getTop10() {
    struct Array top10[10] = {{0}};  // Array to store the top 10 frequent words

    // Traverse through all Insert_Array entries and find the top 10 frequent words
    for (int i = 0; i < size; i++) {
        // Insert the word into the top10 list if its frequency is high enough
        for (int j = 0; j < 10; j++) {
            if (top10[j].frequency == 0 || Insert_Array[i].frequency > top10[j].frequency) {
                // Shift lower frequency words down
                for (int k = 9; k > j; k--) {
                    top10[k] = top10[k - 1];
                }
                top10[j] = Insert_Array[i];
                break;
            }
        }
    }

    // Print the top 10 most frequent words
    printf("Top 10 most frequent words:\n");
    for (int i = 0; i < 10; i++) {
        if (top10[i].frequency > 0) {
            printf("%s: %d\n", top10[i].word, top10[i].frequency);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <filename> <num_threads>\n", argv[0]);
        return 1;
    }

    char *filename = argv[1];         // First argument is the file name
    int num_threads = atoi(argv[2]);  // Second argument is the number of threads

    if (num_threads <= 0) {
        printf("Error: The number of threads should be greater than 0.\n");
        return 1;
    }

    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);  // Array of thread IDs
    if (threads == NULL) {
        printf("Error: Unable to allocate memory for threads.\n");
        return 1;
    }

    long file_size;
    FILE *in = fopen(filename, "r");

    if (in == NULL) {
        printf("Error! Can't open the file: %s\n", filename);
        return 1;
    }

    // Get the size of the file
    fseek(in, 0, SEEK_END);
    file_size = ftell(in);
    fclose(in);


    // Calculate the chunk size each thread will process
    long chunk_size = file_size / num_threads;

    // Create the threads and assign each thread a portion of the file
    for (int i = 0; i < num_threads; i++) {
        struct ThreadData *data = malloc(sizeof(struct ThreadData));
        data->filename = filename;
        data->offset = i * chunk_size;
        data->chunk_size = (i == num_threads - 1) ? (file_size - data->offset) : chunk_size; // last thread gets the remainder

        pthread_create(&threads[i], NULL, loadDataThread, data);
    }

    // Wait for all threads to finish
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Get Top 10 most frequent words and measure time
    getTop10();

    // Free dynamically allocated memory for threads
    free(threads);

    return 0;
}
*/