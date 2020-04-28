// Author: Justin Tromp
// Date: 04/25/2020
// Description: Buildrooms randomly selects 7 out of 10 preset room names and randomly applies values to them such
// as type of room (start, end, or mid) and 3-6 randomly generated connections to other rooms. These values along with
// the name of the room selected are each outputted to a room file in a new directory appended with pid for each run.

#include <stdio.h>
#include <stdlib.h>
#include <zconf.h>
#include <memory.h>
#include <sys/stat.h>
#include <time.h>

// Sets all positions on 7x7 matrix graph to -1.
// Pre-conditions: Receives a 7x7 matrix int graph as parameter.
// Post-conditions: All positions in parameter matrix are set to -1.
void initializeGraph(int roomGraph[7][7]) {

    int x;
    for (x = 0; x < 7; x++) {
        int y;
        for (y = 0; y < 7; y++) {
            roomGraph[x][y] = -1;
        }
    }

}

// Checks if graph passed as parameter is full according to build rooms rules. Each room must have 3 or more
// and less than 7 room connections. This function checks number of connections by analyzing how many positions
// are still set to -1 (unset).
// Pre-conditions: Pass 7x7 int matrix as parameter to analyze values.
// Post-conditions: Returns 1 if graph is full or 0 if not full.
int isGraphFull(int roomGraph[7][7]) {
    int roomConnections[7] = { 0, 0, 0, 0, 0, 0, 0 };
    int x;
    int y;

    // Loop through room matrix and increase counter for room connections array if a room is found
    for (x = 0; x < 7; x++) {
        for (y = 0; y < 7; y++) {
            if (roomGraph[x][y] != -1) {
                roomConnections[x] += 1;
            }
        }
    }

    int idx;
    int graphFull = 1;
    // Loop through room counts to see if any are less than 3, if so then the graph is not full.
    for (idx = 0; idx < 7; idx++) {
        if (roomConnections[idx] < 3) {
            graphFull = 0;
        }
    }

    return graphFull;
}

// Determines if you can still add a connection from the room selected. If so, returns 1, otherwise returns 0.
// Pre-conditions: Pass room matrix (7x7) with current room connections and the room in question as int value room.
// Post-conditions: Returns 1 if room can have another connection, otherwise returns 0.
int canAddConnectionFrom(int rooms[7][7], int room) {
    int canAdd = 0;

    int roomCount = 0;
    int idx;
    // Loop through room and count number of rooms assigned, if less than 6
    for (idx = 0; idx < 7; idx++) {
        if (rooms[room][idx] != -1) {
            roomCount++;
        }
    }

    // Set if less than 6 assignments to allow adding another room
    if (roomCount < 6) {
        canAdd = 1;
    }

    return canAdd;
}

// Checks if the two rooms are the same room by comparing room int values.
// Pre-conditions: Pass int representation of each room (A and B)
// Post-conditions: If roomA and roomB are the same, return 1, else return 0.
int isSameRoom(int roomA, int roomB) {
    int isSame = 0;

    if (roomA == roomB) {
        isSame = 1;
    }

    return isSame;
}

// Check if connection between two rooms already exists and return int value with determined result.
// Pre-conditions: Pass room graph matrix (7x7) of room connections and two int values, one for room A and
// one for roomB.
// Post-conditions: If roomA and roomB are already connected, return 1, else return 0.
int connectionAlreadyExists(int roomGraph[7][7], int roomA, int roomB) {
    int connExists = 0;

    int idxA;
    int idxB;

    // Loop through room matrix and see if room a and room b have a matching connection already. Set connExists to 1
    // if true and exit.
    for (idxA = 0; idxA < 7; idxA++) {
        for (idxB = 0; idxB < 7; idxB++) {
            if (roomGraph[roomA][idxA] == roomB && roomGraph[roomA][idxA] != -1 && roomGraph[roomB][idxB] != -1) {
                if (roomGraph[roomB][idxB] == roomA) {
                    connExists = 1;
                    break;
                }
            }
        }
    }

    return connExists;
}

// Connect two rooms together. Adds integer value of roomB to the matrix graph for applicable room position of roomA.
// Pre-conditions: Pass valid matrix graph of room connections and an int value for each room.
// Post-conditions: Sets room connection matrix with value indicating a connection from roomA to roomB.
void connectRoom(int roomGraph[7][7], int roomA, int roomB) {
    roomGraph[roomA][roomB] = roomB;
}

// Adds a random connection between two randomly selected rooms and sets the connection if valid in room
// connection matrix.
// Pre-conditions: Pass valid room connection matrix (7x7) to manipulate connections on.
// Post-conditions: Room connection matrix will be filled with each room containing 3-6 randomly determined connections.
void addRandomConnection(int roomGraph[7][7]) {
    int roomA = -1;
    int roomB = -1;

    while (1) {
        roomA = rand() % 7;

        // Check if you can add a room connection to chosen room, if so exit loop
        if (canAddConnectionFrom(roomGraph, roomA) == 1) {
            break;
        }
    }

    // Randomly generate room to link to until it is not the same room as room a, a connection doesn't exist already,
    // and it's possible to add a connection (< 6 connections already).
    do {
        roomB = rand() % 7;
    }
    while(canAddConnectionFrom(roomGraph, roomB) == 0 || isSameRoom(roomA, roomB) == 1
        || connectionAlreadyExists(roomGraph, roomA, roomB) == 1);

    connectRoom(roomGraph, roomA, roomB);
    connectRoom(roomGraph, roomB, roomA);
}

// Randomly selects 7 out of 10 possible room names to be used.
// Pre-conditions: Must be passed a char pointer to array of selected rooms.
// Post-conditions: Selected rooms char* array will be filled with names of 7 randomly selected room names out
// of 10 possible selections.
void selectRooms(char* selectedRooms[]) {
    int chosenRooms[7] = { -1, -1, -1, -1, -1, -1, -1 };

    // Declare list of possible rooms
    char* rooms[10] = {
            "Dungeon",
            "Barracks",
            "Garden",
            "Game",
            "Medical",
            "Corridor",
            "Kitchen",
            "Stairs",
            "Basement",
            "Attic"
    };

    int selectedRoom = 0;
    // Randomly select 7 rooms and add to selected rooms list
    srand(time(NULL));
    int i = 0;
    for (i; i < 7; i++) {
        int uniqueRoom = 0;
        // See if randomly generated value is unique and if not, generate until it is
        while (uniqueRoom == 0) {
            uniqueRoom = 1;
            selectedRoom = rand() % 10;

            int x = 0;
            for (x; x < 7; x++) {
                if (chosenRooms[x] == selectedRoom) {
                    uniqueRoom = 0;
                }
            }
        }

        // Add selected room integer to chosen rooms array
        chosenRooms[i] = selectedRoom;

        // Add name of room to selected rooms array
        selectedRooms[i] = rooms[selectedRoom];
    }

}

// Sets up and creates room files with randomly generated room connections, randomly generated types, and the
// name of the room in each file.
// Pre-conditions: Pass valid matrix graph of room connections to generate, name of directory created, and room names
// that are randomly selected to be generated.
// Post-conditions: Creates a file with applicable name, room connections, and room type for each room in selectedRooms
// array in the provided directory name location.
void setupRoomFiles(char dirName[], char* selectedRooms[], int roomGraph[7][7]) {

    int roomTypes[7] = { -1, -1, -1, -1, -1, -1, -1 };
    char roomType[25];
    memset(roomType, '\0', sizeof(roomType));

    int fileNum = 0;
    // Setup room files with name and type
    for (fileNum; fileNum < 7; fileNum++) {
        char pathName[50];
        memset(pathName, 0, 50);

        // Create path name for file creation
        strcat(pathName, dirName);
        strcat(pathName, "/");
        strcat(pathName, selectedRooms[fileNum]);
        strcat(pathName, "_room");

        FILE * fPointer = fopen(pathName, "w");

        // Check to see if file was opened
        if (fPointer == NULL) {
            perror("Error opening a file.");
        }
            // Output room name and room type to file
        else {
            char fileOutput[25];
            memset(fileOutput, '\0', sizeof(fileOutput));

            // Output name of room to file
            strcat(fileOutput, "ROOM NAME: ");
            strcat(fileOutput, selectedRooms[fileNum]);
            strcat(fileOutput, "\n");
            fprintf(fPointer, fileOutput);

            memset(roomType, 0, 25);

            // Randomly select one of the room types. Only 0 can be START and 6 is END
            int uniqueRoom = 0;
            // See if randomly generated value is unique and generate until it is
            while (uniqueRoom == 0) {
                uniqueRoom = 1;
                int selectedRoom = rand() % 7;

                int x = 0;

                for (x; x < 7; x++) {
                    if (roomTypes[x] == selectedRoom) {
                        uniqueRoom = 0;
                    }
                }
                roomTypes[fileNum] = selectedRoom;
            }

            int connection = 0;
            int connCounter = 0;
            char connectionChar[2];
            // Output room connection strings to file
            for (connection; connection < 7; connection++) {
                if (roomGraph[fileNum][connection] != -1) {
                    connCounter++;

                    memset(fileOutput, '\0', sizeof(fileOutput));
                    strcat(fileOutput, "CONNECTION ");

                    // Set connection number after string conversion
                    sprintf(connectionChar, "%d", connCounter);
                    strcat(fileOutput, connectionChar);

                    strcat(fileOutput, ": ");

                    // Output name of room to string based on room matrix
                    strcat(fileOutput, selectedRooms[roomGraph[fileNum][connection]]);
                    strcat(fileOutput, "\n");

                    fprintf(fPointer, fileOutput);
                }
            }

            // Build room type string to add to file. Randomly select whether it is START, END, or MID.
            // There can only be one of each value, which equals 1 start, 1 end, and 5 mid rooms.
            strcat(roomType, "ROOM TYPE: ");
            if (roomTypes[fileNum] == 0) {
                strcat(roomType, "START_ROOM");
            }
            else if (roomTypes[fileNum] == 6) {
                strcat(roomType, "END_ROOM");
            }
            else {
                strcat(roomType, "MID_ROOM");
            }
            strcat(roomType, "\n");


        }

        // Print to file and close file
        fprintf(fPointer, roomType);
        fclose(fPointer);
    }


}

// Main function creates/opens directory and creates/opens a file for each room. Each room file will be filled with
// applicable information about the room, such as its name, 3-6 randomly generated room connections, and a randomly
// generated room type of either start, mid, or end.
int main() {
    int roomGraph[7][7];
    // Initialize matrix of room connections with -1 values
    initializeGraph(roomGraph);

    // Generate all room connections in graph randomly
    while (isGraphFull(roomGraph) == 0) {
        addRandomConnection(roomGraph);
    }

    char dir[32] = "trompj.rooms.";
    char dirName[256];
    memset(dirName, '\0', sizeof(dirName));

    char pid[10];
    sprintf(pid, "%d", getpid());

    // Create directory name to be created
    strcat(dirName, dir);
    strcat(dirName, pid);

    // Create directory with process ID
    int result = mkdir(dirName, 0755);
    if (result != 0) {
        perror("Error creating directory.");
    }

    char* selectedRooms[7] = { "", "", "", "", "", "", "" };
    // Fill selectedRooms and chosenRooms arrays by randomly selecting rooms to build out of list of 10 options
    selectRooms(selectedRooms);

    // Generate files with randomly selected room connections, type, and the name of the room
    setupRoomFiles(dirName, selectedRooms, roomGraph);

    return 0;
}