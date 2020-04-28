// Author: Justin Tromp
// Date: 04/25/2020
// Description: Adventure allows a user to navigate from a starting room in the newest directory of rooms
// to an end room through command line input. With use of threading, user can input "time" instead of room
// in order to output current time to a file, which is then read into the main program and outputted to
// the terminal screen. After a win condition is reached, user gets a congratulatory message and is informed
// of the number of rooms moved through, as well as the rooms that were moved through by name.
// REFERENCES: https://www.geeksforgeeks.org/mutex-lock-for-linux-thread-synchronization/

#include <stdio.h>
#include <stdlib.h>
#include <zconf.h>
#include <memory.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <pthread.h>

// Struct for room name, room type, and an array of room connections.
struct room {
    char* roomName;
    char* roomType;
    char* roomConnections[6];
};

// Struct for use with thread to pass file pointer and mutex lock.
struct fileThreadLock {
    FILE* filePointer;
    pthread_mutex_t lock;
};

// Opens or creates a new currentTime.txt file and outputs the current time to the file. Will overwrite
// the file if it already exists. This is implemented as a thread with a mutex lock.
// Pre-conditions: Must be passed a fileThreadLock struct pointer.
// Post-conditions: Determines current time and outputs to currentTime.txt file.
void* writeTimeFileThread(void* args) {
    struct fileThreadLock* threadVars = args;

    // Set mutex lock for thread
    if (pthread_mutex_lock(&threadVars->lock) != 0) {
        perror("Error locking mutex");
        exit(1);
    }

    char fileName[] = "currentTime.txt";
    // Open and create file if necessary to write time to. Overwrite if exists.
    threadVars->filePointer = fopen(fileName, "w");

    // Set variables for use in time operations
    char timeString[256];
    time_t t;
    struct tm* tmp;
    t = time(NULL);
    tmp = localtime(&t);

    // If tmp is NULL, there was an error getting local time, exit and output error.
    if (tmp == NULL) {
        perror("Local time error");
        exit(1);
    }

    // Generate time/date string. If there is an error, output to stderr and exit
    if (strftime(timeString, sizeof(timeString), "%l:%M%P, %A, %B %d, %Y\n", tmp) == 0) {
        fprintf(stderr, "strftime error");
        exit(1);
    }

    // Output time/date string to file
    fprintf(threadVars->filePointer, "%s", timeString);
    fflush(threadVars->filePointer);

    // Close time file
    fclose(threadVars->filePointer);

    // Unlock thread so time file can be accessed by main
    if (pthread_mutex_unlock(&threadVars->lock) != 0) {
        perror("Error unlocking mutex");
        exit(1);
    }
}

// Opens currentTime.txt file and reads the time from the file to be outputted to the terminal screen.
// This is implemented as part of the main thread and utilizes the same mutex lock as file writing.
// Pre-conditions: Must be passed a fileThreadLock struct pointer.
// Post-conditions: Reads time from file sand outputs to terminal screen.
void readTimeFile(struct fileThreadLock* threadVars) {
    char fileName[] = "currentTime.txt";
    // Open file to read time from
    threadVars->filePointer = fopen(fileName, "r");

    char lineRead[80];
    memset(lineRead, '\0', 80);

    // Read first line from file and output to terminal
    fseek(threadVars->filePointer, 0, SEEK_SET);
    fgets(lineRead, 80, threadVars->filePointer);
    printf("%s\n", lineRead);

    // Close time file
    fclose(threadVars->filePointer);
}

// Driver function for time processing, which directs the threading process and read/write of time file.
// Pre-conditions: None
// Post-conditions: Results in write of current time to currentTime.txt and output of time to terminal.
void timeProcessing(struct fileThreadLock* threadVars) {

    // Unlock mutex for thread to use
    if (pthread_mutex_unlock(&threadVars->lock) != 0) {
        perror("Error unlocking mutex");
        exit(1);
    }

    pthread_t thread;
    // Create thread to write to file and throw error if unable to create
    int error = pthread_create(&thread, NULL, &writeTimeFileThread, threadVars);
    if (error != 0) {
        perror("Thread was unable to be created.");
        exit(1);
    }

    // Have main thread wait until completion of file write thread
    pthread_join(thread, NULL);

    // Set mutex lock for main
    if (pthread_mutex_lock(&threadVars->lock) != 0) {
        perror("Error locking mutex");
        exit(1);
    }

    // Read time from file and output to screen
    readTimeFile(threadVars);

}

// Checks all directories in current location and determines the most recent rooms directory.
// The name of the directory is saved in parameter variable for later use.
// Pre-conditions: Char array parameter to save directory name to.
// Post-conditions: Most recent directory name is saved to dirName char array.
void mostRecentRooms(char dirName[128]) {
    // Declare variables to be used in directory manipulation
    int newestModified = -1;
    char dirPrefix[15] = "trompj.rooms.";
    memset(dirName, '\0', 128);

    // Starting directory
    DIR* dirToExamine = opendir(".");
    // To hold sub directory of starting directory
    struct dirent* subDir;
    // To hold information about sub directory
    struct stat dirAttributes;

    // Check that directory could be opened
    if (dirToExamine > 0) {
        // Loop through directory contents
        while ((subDir = readdir(dirToExamine)) != NULL) {

            // Check encountered entry for prefix
            if (strstr(subDir->d_name, dirPrefix) != NULL) {

                // Get attributes
                stat(subDir->d_name, &dirAttributes);

                // Check if found subDir is newer than last newest directory
                // If so, update time and name of directory
                if ((int) dirAttributes.st_mtime > newestModified) {
                    newestModified = (int) dirAttributes.st_mtime;

                    memset(dirName, '\0', 128);
                    strcpy(dirName, subDir->d_name);
                }
            }
        }
    }
    else {
        perror("Could not open directory");
    }

    closedir(dirToExamine);
}

// Takes a room struct pointer as parameter and sets all values to NULL to initialize.
// Pre-conditions: Room struct pointer passed to be initialized.
// Post-conditions: All aspects of the room struct are set to NULL.
void initializeData(struct room* roomObj) {
    // Initialize struct values before assignment
    roomObj->roomName = NULL;

    roomObj->roomType = NULL;

    unsigned int i = 0;
    for(i; i < 6; i++){
        roomObj->roomConnections[i] = NULL;
    }
}

// Reads a room file and sets values in a room struct, such as name, type, and connections. This room
// struct is then returned.
// Pre-conditions: A FILE pointer is passed as parameter, which will be the file to read room information from.
// Post-conditions: A room struct has all variables set with name, type, and connections for that room and is returned.
struct room readFile(FILE* fPointer) {
    char lineRead[256];
    memset(lineRead, '\0', 256);

    // Declare a room struct and initialize it to be returned at end of function with data
    struct room roomObj;
    initializeData(&roomObj);

    int connNumber = 0;
    // Read line from file
    while (fgets(lineRead, 255, fPointer) != NULL) {
        // Check if line is room name and add to struct
        if (strstr(lineRead, "ROOM NAME:")) {
            int roomIdx = 0;
            int i = 11;
            char* name = malloc(sizeof(char)*11);
            memset(name, '\0', 11);
            // Loop through result and extract data into struct
            for (i; i < strlen(lineRead); i++) {
                if (lineRead[i] != '\n') {
                    name[roomIdx] = lineRead[i];
                    roomIdx++;
                }
            }
            roomObj.roomName = name;

        }
        // Check if line is room type and add to struct
        else if (strstr(lineRead, "ROOM TYPE:")) {
            int roomIdx = 0;
            int i = 11;
            char* type = malloc(sizeof(char)*11);
            memset(type, '\0', 11);
            // Loop through result and extract data into struct
            for (i; i < strlen(lineRead); i++) {
                if (lineRead[i] != '\n') {
                    type[roomIdx] = lineRead[i];
                    roomIdx++;
                }
            }
            roomObj.roomType = type;
        }
        // Check if line is a connection and add to struct
        else if (strstr(lineRead, "CONNECTION")) {
            // Prepare string to accept a room connection name
            char* roomConn = malloc(sizeof(char)*10);
            memset(roomConn, '\0', 10);

            int i = 14;
            int strIdx = 0;
            // Loop through result and extract data into struct
            for (i; i < strlen(lineRead); i++) {
                if (lineRead[i] != '\n') {
                    roomConn[strIdx] = lineRead[i];
                    strIdx++;
                }
            }
            // Set room connection
            roomObj.roomConnections[connNumber] = roomConn;
            connNumber++;
        }
        memset(lineRead, '\0', 256);
    }

    return roomObj;
}

// Open directory and read room file contents. Room information is saved to room struct objects and set
// to an array of room structs for later use.
// Pre-conditions: Valid name of directory (dirName) and array of room structs to set values to.
// Post-conditions: Array of room structs has values set for later use in program.
void setRoomArray(char dirName[], struct room roomArr[]) {

    // Open directory
    DIR *dirToOpen;
    dirToOpen = opendir(dirName);
    struct dirent *fileInDir;

    // Check if directory could be opened, if not output error
    if (dirToOpen > 0) {
        int arrIdx = 0;
        // Loop through contents (room files)
        while ((fileInDir = readdir(dirToOpen)) != NULL) {
            // If _room file is found, open it and extract data
            if (strstr(fileInDir->d_name, "_room")) {
                char pathName[256];
                memset(pathName, '\0', 256);

                // Create path of file and open it
                strcat(pathName, dirName);
                strcat(pathName, "/");
                strcat(pathName, fileInDir->d_name);

                FILE *fPointer = fopen(pathName, "r");
                // Check to see if file was opened
                if (fPointer == NULL) {
                    perror("Error opening a file.");
                }
                    // Output room name and room type to struct
                else {
                    // Set values in structs from files
                    roomArr[arrIdx] = readFile(fPointer);
                    arrIdx++;
                }

                fclose(fPointer);
            }
        }

        // Close the directory used to access room files
        closedir(dirToOpen);
    }
        // Directory opening issue, output error
    else {
        perror("Directory could not be opened");
    }

}

// Runs game until user reaches an end room, after which the game driver will exit with all win conditions printed
// for user to see.
// Pre-conditions: Must have valid room struct array with room information filled.
// Post-conditions: Driver function runs until the user reaches the end room. Win conditions are outputted for user.
runGameDriver(struct room roomArr[]) {

    struct room* currentRoom;
    int roomNum = 0;
    // Find starting location and set current rom pointer to starting room
    for (roomNum; roomNum < 7; roomNum++) {
        if (strcmp(roomArr[roomNum].roomType, "START_ROOM") == 0) {
            currentRoom = &roomArr[roomNum];
        }
    }

    int stepCount = 0;
    char userInputRoom[10];
    memset(userInputRoom, '\0', 10);
    char **visitedRooms = NULL;

    // Set thread struct in thread struct pointer for use throughout program for time processing.
    // Allows threads to be managed by same mutex lock and pass file pointers as needed.
    struct fileThreadLock* threadVars;
    struct fileThreadLock threadInfo;
    threadVars = &threadInfo;
    // Initialize mutex and throw error if unable to initialize
    if (pthread_mutex_init(&threadInfo.lock, NULL) != 0) {
        perror("Mutex initialization has failed!\n");

        exit(1);
    }

    // Set mutex lock for main
    if (pthread_mutex_lock(&threadVars->lock) != 0) {
        perror("Error locking mutex");
        exit(1);
    }

    // Loop until end room is reached and track number of steps and names of rooms visited
    while (strcmp(currentRoom->roomType, "END_ROOM") != 0) {

        if (strcmp(userInputRoom, "time") != 0) {
            // Output current location name
            printf("CURRENT LOCATION: %s\n", currentRoom->roomName);

            // Output possible connections from current location
            printf("POSSIBLE CONNECTIONS:");
            int roomConnIdx = 0;
            for (roomConnIdx;
                 roomConnIdx < sizeof(currentRoom->roomConnections) / sizeof(*currentRoom->roomConnections);
                 roomConnIdx++) {
                // Check if room name encountered is null, if so then exit loop
                if (currentRoom->roomConnections[roomConnIdx] == NULL) {
                    break;
                }

                // Output room name to terminal
                printf(" %s", currentRoom->roomConnections[roomConnIdx]);

                // Output expected punctuation based on whether next room connection is NULL or not
                if (roomConnIdx == 5) {
                    printf(".");
                }
                else if (currentRoom->roomConnections[roomConnIdx+1] != NULL) {
                    printf(",");
                }
                else {
                    printf(".");
                }
            }
            printf("\n");
        }

        char buffer[256];
        memset(buffer, '\0', 256);
        // Request and accept user input
        printf("WHERE TO? >");
        fgets(buffer, sizeof(buffer), stdin);

        // Get string without \n from user input for comparison
        memset(userInputRoom, '\0', 10);
        strcpy(userInputRoom, buffer);
        userInputRoom[(strlen(userInputRoom)-1)] = '\0';

        int roomConnIdx = 0;
        int roomFound = 0;
        // Loop through possible room connections to check against user input for match
        for (roomConnIdx; roomConnIdx < sizeof(currentRoom->roomConnections)/sizeof(*currentRoom->roomConnections);
             roomConnIdx++) {
            // Check if room name encountered is null, if so then exit loop
            if (currentRoom->roomConnections[roomConnIdx] == NULL) {
                break;
            }

            // Check if room matches a connection and break/change current room to new room connection if so
            if (strcmp(currentRoom->roomConnections[roomConnIdx], userInputRoom) == 0) {
                int roomNum = 0;
                // Find matching room from array and set currentRoom as new room moved to
                for (roomNum; roomNum < 7; roomNum++) {
                    if (strcmp(roomArr[roomNum].roomName, userInputRoom) == 0) {
                        currentRoom = &roomArr[roomNum];
                        roomFound = 1;

                        stepCount++;
                        char** newVisitedRooms = malloc(stepCount * sizeof(char*));
                        int i = 0;
                        // Loop through array and add
                        for (i; i < stepCount; i++) {
                            newVisitedRooms[i] = malloc(sizeof(char)*10);
                        }

                        i = 0;
                        // Loop through and copy values to new dynamically allocated array. Free memory after copy.
                        for (i; i < (stepCount-1); i++) {
                            free(newVisitedRooms[i]);
                            newVisitedRooms[i] = visitedRooms[i];
                        }
                        free(visitedRooms);

                        // Add newly visted room to visited rooms array
                        strcpy(newVisitedRooms[(stepCount-1)], userInputRoom);
                        visitedRooms = newVisitedRooms;
                        newVisitedRooms = NULL;
                    }
                }

                break;
            }
        }

        printf("\n");

        // User requests time
        if (roomFound == 0 && strcmp(userInputRoom, "time") == 0) {
            timeProcessing(threadVars);
        }
        // If room was not found, output message indicating room not found
        else if (roomFound == 0) {
            printf("HUH? I DON’T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
        }
        // Check if room is END_ROOM and output win message/exit adventure if found
        else if (strcmp(currentRoom->roomType, "END_ROOM") == 0) {
            printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
            printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", stepCount);

            int roomIdx = 0;
            // Loop through path and output rooms visited
            for (roomIdx; roomIdx < stepCount; roomIdx++) {
                printf("%s\n", visitedRooms[roomIdx]);
            }

            break;
        }

    }

    int i = 0;
    // Free visited rooms character arrays
    for (i; i < stepCount; i++) {
        free(visitedRooms[i]);
    }
    free(visitedRooms);

    // Destroy mutex
    pthread_mutex_destroy(&threadVars->lock);
}

// Main function to link all pieces of adventure process
int main() {
    // Determine which directory has the most recent rooms
    char dirName[128];
    memset(dirName, '\0', 128);
    mostRecentRooms(dirName);

    struct room roomArr[7];
    int i = 0;
    // Loop through array and initialize values in structs
    for (i; i < sizeof(roomArr) / sizeof(struct room); i++) {
        initializeData(&roomArr[i]);
    }

    // Set room array with applicable information from room files found in newest directory
    setRoomArray(dirName, roomArr);

    // Runs adventure until user reached end condition by reaching the end room
    runGameDriver(roomArr);

    i = 0;
    // Free all memory allocations in room struct objects for room connections
    for (i; i < 7; i++) {
        int x = 0;
        // Loop through all room connections and free allocated memory
        for (x; x < 6; x++) {
            if (roomArr[i].roomConnections[x] == NULL) {
                break;
            }
            else {
                free(roomArr[i].roomConnections[x]);
            }
        }

        // Free allocated memory for room name and room type
        free(roomArr[i].roomName);
        free(roomArr[i].roomType);
    }

    return 0;
}