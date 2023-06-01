// CSCI 4780 Spring 2021 Project 4
// Jisoo Kim, Will McCarthy, Tyler Scalzo

#include <iostream>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fstream>
#include <pthread.h>

using namespace std;

#define BUFFSIZE 1024 // For char buffs.

struct Node
{
  int key;
  char value[20];
  Node* prev;
  Node* next;
};

struct CircularList
{
  Node* head;
};

int bootId, bootPort;
int rangeLow = 0; // Bootstrap always has 0, keep track of the other range end.
int prevPort, prevID, nxtPort, nxtID; // Info of previous, and next nodes.
CircularList list;

void config(char* fileName); // Parse config.
int initConnection(int port); // Creating server connection.
Node* allocNode();
void initList(CircularList* clist);
void setNode(Node* node);
void insertNode(Node* node);
void print(const CircularList* clist, int key);
Node* lookupKey(const CircularList* clist, int key);
void insertKey(int key, char value[20]);
void deleteKey(int key);
void* bgThread(void* arg); // Thread for connection handling.

int main(int argc, char* argv[])
{
  if (argc != 2) //command line arguments format correction
  {
    cout << "Usage: ./bootstrap [config_file_path]" << endl;
    return EXIT_FAILURE;
  }
  config(argv[1]);

  // Thread for background connections.
  pthread_t backgdThread;
  pthread_create(&backgdThread, NULL, bgThread, NULL);

  while(1){
    printf("Commands available: (1) lookupKey, (2) insertKey, (3) deleteKey: \n");

    int command = 0;
    int key = 0;
    scanf("%d", &command); //get command from the terminal

    if(command == 1){ //lookupKey
      printf("Enter a key to look up: ");
      scanf("%d", &key);
      if (key >= rangeLow || key == 0) // In current node's range.
      {
        printf("key is in range of btsp \n");
        lookupKey(&list, key);
      } else {
        //build message to traverse to next server
        printf("key not in range of btsp \n");
        char message[BUFFSIZE]; // Net, and message buffs.
        memset(message, 0, BUFFSIZE);
        //define the command for the message
        strcat(message, "lkp");

        // Conerting the int to a char*.
        char keyChar[10];
        sprintf(keyChar, "%d", key);

        //putting the key on the message
        strcat(message, " ");
        strcat(message, keyChar);
        strcat(message, " ");

        //append boostrap to end for sequence
        strcat(message, "btsp");

        // Sending message to next nameserver.
        // Need to open connection to next server, and send message to it.
        int cnct = initConnection(nxtPort); // Connecting to server.

        int check = send(cnct, message, strlen(message), 0);
        if (check < 0)
        {
            cout << "Error sending data." << endl;
            exit(EXIT_FAILURE);
        }
      }
    }else if(command == 2){ //insertKey
      char value[20];
      printf("Enter a key value pair to insert (separated by a single whitespace): ");
      scanf("%d %s", &key, value);

      if (key >= rangeLow || key == 0) // In current node's range.
      {
        printf("key is in range of btsp \n");
        insertKey(key, value);
      } else {
        //build message to traverse to next server
        printf("key not in range of btsp \n");
        char message[BUFFSIZE]; // Net, and message buffs.
        memset(message, 0, BUFFSIZE);
        //define the command for the message
        strcat(message, "ins");

        // Conerting the int to a char*.
        char keyChar[10];
        sprintf(keyChar, "%d", key);

        //putting the key on the message
        strcat(message, " ");
        strcat(message, keyChar);
        strcat(message, " ");

        // Putting value in the message.
        strcat(message, value);
        strcat(message, " ");

        //append boostrap to end for sequence
        strcat(message, "btsp");

        // Sending message to next nameserver.
        // Need to open connection to next server, and send message to it.
        int cnct = initConnection(nxtPort); // Connecting to server.

        int check = send(cnct, message, strlen(message), 0);
        if (check < 0)
        {
            cout << "Error sending data." << endl;
            exit(EXIT_FAILURE);
        }
      }
    }else if(command == 3){  //deleteKey
      printf("Enter a key to delete: ");
      scanf("%d", &key);
      if (key >= rangeLow || key == 0) // In current node's range.
      {
        printf("key is in range of btsp delete \n");
        deleteKey(key);
      } else {
        //build message to traverse to next server
        printf("key not in range of btsp: delete \n");
        char message[BUFFSIZE]; // Net, and message buffs.
        memset(message, 0, BUFFSIZE);
        //define the command for the message
        strcat(message, "del");

        // Conerting the int to a char*.
        char keyChar[10];
        sprintf(keyChar, "%d", key);

        //putting the key on the message
        strcat(message, " ");
        strcat(message, keyChar);
        strcat(message, " ");

        //append boostrap to end for sequence
        strcat(message, "btsp");

        // Sending message to next nameserver.
        // Need to open connection to next server, and send message to it.
        int cnct = initConnection(nxtPort); // Connecting to server.

        int check = send(cnct, message, strlen(message), 0);
        if (check < 0)
        {
            cout << "Error sending data." << endl;
            exit(EXIT_FAILURE);
        }//if check
      } //else not in range
    }//if command == 3
    else{
      printf("Command not accepted\n");
    }
  }

  return EXIT_SUCCESS;
}

bool inRange(int key){
  int low = list.head->prev->key;
  int high = 0;
  if(key == 0 || key > rangeLow){
    return true;
  } else{
    return false;
  }
}

void config(char* fileName) // Getting data from the configuration file.
{
    ifstream conFile(fileName);
    string line;

    // Bootstrap server Id
    getline(conFile, line);
    bootId = stoi(line);

    // Bootstrap port number
    getline(conFile, line);
    bootPort = stoi(line);

    // Initialize a circular likst
    initList(&list);

    // Read each key, value pair and store it in the linked list
    while(getline(conFile, line)){

      const char* temp = line.c_str();
      char* value = strtok(strdup(temp), " ");
      int key = atoi(value);
      value = strtok(NULL, "\n");

      Node* node = allocNode();
      node->key = key;
      strcpy(node->value, value);

      insertNode(node);
    }

    // Setting initial, empty, server values.
    prevPort = bootPort;
    prevID = bootId;
    nxtPort = bootPort;
    nxtID = bootId;
} // config()

int initConnection(int port) // Creating server connection. Return fd.
{
    // Conerting the int to a char*.
    char portChar[10];
    sprintf(portChar, "%d", port);

    const char* srvHost = "127.0.0.1"; // All localhost for this project.

    int sockfd = 0, check = 0;
    struct addrinfo cltAddr, *conInfo;

    // Setting up the host's IP information.
    memset(&cltAddr, 0, sizeof cltAddr);
    cltAddr.ai_family = AF_INET;
    cltAddr.ai_socktype = SOCK_STREAM;
    cltAddr.ai_flags = AI_PASSIVE;

    // Getting connection information.
    getaddrinfo(srvHost, portChar, &cltAddr, &conInfo);

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // Getting socket fd.
    if (sockfd < 0)
    {
            cout << "Error creating Socket." << endl;
            exit(EXIT_FAILURE);
    }

    // Connecting to server.
    check = connect(sockfd, conInfo->ai_addr, conInfo->ai_addrlen);
    if (check < 0)
    {
            cout << "Error connecting to server." << endl;
            exit(EXIT_FAILURE);
    }

    return sockfd;
} // initConnection()


Node* allocNode(){
  return (Node*)calloc(1, sizeof(Node));
}

void initList(CircularList* clist){ // Initialize bootstrap node of id 0
  Node* bootNode = allocNode();
  bootNode->key = 0;
  clist->head = bootNode;
  bootNode->prev = bootNode->next = bootNode;
}

void insertNode(Node* node){ // Insert a new node into the circular linked list
  Node* ptr = list.head->next;
  while(ptr != list.head){
    if(node->key > ptr->key){
      ptr = ptr->next;
    }
    else{
      break;
    }
  }
  // Set predecessor and successor for newly added node
  ptr->prev->next = node;
  node->prev = ptr->prev;
  node->next = ptr;
  ptr->prev = node;
}

Node* lookupKey(const CircularList* clist, int key){ // Search a key handled by bootstrap server

  Node* ptr = clist->head->next;
  bool found = false;
  char* value;
  while(ptr != clist->head){
    if(ptr->key == key){
      value = ptr->value;
      found = true;
      break;
    }
    ptr = ptr->next;
  }
  if(found){
    printf("Located in: btsp \n");
    printf("Value: %s \n", value);
    printf("sequence: btsp \n");
    //At this point, a key is found from the list storing all key, value pairs.

    return ptr;
  }else{
    printf("Key not found\n");
    return NULL;
  }
}

void insertKey(int key, char value[20]){ // Insert a key, value pair into a list storing all key, value pairs
  Node* node = allocNode();
  node->key = key;
  strcpy(node->value, value);
  insertNode(node);
  printf("Insterted in btsp \n");
  printf("Sequence: btsp \n");
  //At this point, a new key, value pair is stored in the list storing all key, value pairs
  //Node struct has key and value

}

void deleteKey(int key){ // Delete a key, value pair of the given key from a list of all key, value pairs
  Node* ptr = lookupKey(&list, key);
  if(ptr != NULL){ // Key to be deleted exists

    ptr->prev->next = ptr->next;
    ptr->next->prev = ptr->prev;

    //At this point, a key to be deleted is found from the list storing all key, value pairs

    free(ptr);
    printf("Successful deletion\n");
  }
}

// Print list of of key and value pairs handled by bootstrap server
void print(const CircularList* clist, int key){
  Node* ptr = clist->head->next;
  while(ptr != clist->head){
    if(key < ptr->key){
      break;
    }
    printf("%d\n", ptr->key);
    ptr = ptr->next;
  }
}

void* bgThread(void* arg) // Handles connections, and their commands.
{
  // Conerting the int to a char*.
  char portChar[10];
  sprintf(portChar, "%d", bootPort); //what is this meant to do?

  //get socket ready to listen to a new connections:

  struct addrinfo srvAddr, *conInfo; //serverAddress and connectionInfo

  //create the socket
  int sockFD = socket(AF_INET, SOCK_STREAM, 0); // Getting socket fd.
  if (sockFD < 0)
  {
    cout << "Error creating Socket." << endl;
    exit(EXIT_FAILURE);

  }

  //Getting stuff setup for socket binding
  memset(&srvAddr, 0, sizeof srvAddr);
  srvAddr.ai_family = AF_INET;
  srvAddr.ai_socktype = SOCK_STREAM;
  srvAddr.ai_flags = AI_PASSIVE;

  // Getting connection information.
  getaddrinfo(NULL, portChar, &srvAddr, &conInfo);
  // Binding to the port.
  int check = bind(sockFD, conInfo->ai_addr, conInfo->ai_addrlen);
  if (check < 0)
  {
    cout << "Error binding port." << endl;
    exit(EXIT_FAILURE);
  }

  // Listening for connections.
  check = listen(sockFD, 1);
  if (check < 0)
  {
    cout << "Error listening." << endl;
    exit(EXIT_FAILURE);
  }

  // Client connection stuff.
  socklen_t cltAddrSize = 0;
  struct sockaddr cltAddr;

  char netBuff[BUFFSIZE], message[BUFFSIZE]; // Net, and message buffs.
  char* com;

  // Always listen for connections, and handle them.
  while (true) //is each connection a one time use?
  {
    // Resetting buffers.
    memset(netBuff, 0, BUFFSIZE);
    memset(message, 0, BUFFSIZE);

    // Waiting for connections.
    int accFD = accept(sockFD, &cltAddr, &cltAddrSize);
    if (accFD < 0)
    {
      cout << "Error accepting." << endl;
      exit(EXIT_FAILURE);
    }

    // Recieving message.
    check = recv(accFD, netBuff, BUFFSIZE, 0);
    if (check < 0)
    {
      cout << "Error receiving data." << endl;
      exit(EXIT_FAILURE);
    }

    com = strtok(netBuff, " "); // Getting command.

    //WE NEED TO MOVE CONTENTS OF IF/ELSE INTO METHODS
    //THIS METHOD IS TOO LONG

    if ((strcmp(com, "lkp") == 0)) // Lookup.
    {

      // Msg struct: com locID keyVal sequence

      com = strtok(NULL, " "); // Getting server ID of location.

      cout << "Located in: " << com << endl;

      com = strtok(NULL, " "); // Getting key value.

      cout << "Value: " << com << endl;

      com = strtok(NULL, " "); // Getting nameserver sequence.

      cout << "Sequence: " << com << endl;
    }
    else if ((strcmp(com, "ins") == 0)) // Insert.
    {
      // Msg struct: com locID sequence

      com = strtok(NULL, " "); // Getting server ID of inserted location.

      cout << "Inserted in: " << com << endl;

      com = strtok(NULL, " "); // Getting nameserver sequence.

      cout << "Sequence: " << com << endl;
    }
    else if ((strcmp(com, "del") == 0)) // Delete.
    {
      // Msg struct: com sequence

      com = strtok(NULL, " "); // Getting nameserver sequence.

      cout << "Successful deletion" << endl;
      cout << "Sequence: " << com << endl;
    }
    else if ((strcmp(com, "nf") == 0)) // Not found.
    {
      // Msg struct: com sequence

      com = strtok(NULL, " "); // Getting nameserver sequence.

      cout << "Key not found" << endl;
      cout << "Sequence: " << com << endl;
    }
    else if ((strcmp(com, "en") == 0)) // Enter. // PUT ENTER INTO ITS OWN METHOD
    {
      // Msg struct: com nameID namePort

      com = strtok(NULL, " "); // Getting nameID.
      int tempID = stoi(com); // Converting to int.

      com = strtok(NULL, " "); // Getting namePort.
      int tempPort = stoi(com); // Converting to int.

      if (tempID > rangeLow) // If this is to be the succesor.
      {
        // Alert server that this will be it's succesor, and give it prev info.
        strcpy(message, "ok ");

        char anotherTemp[20];
        sprintf(anotherTemp, "%d", prevID);
        strcat(message, anotherTemp);
        strcat(message, " ");

        memset(anotherTemp, 0, 20);
        sprintf(anotherTemp, "%d", prevPort);
        strcat(message, anotherTemp);
        strcat(message, " btsp");

        // Send message to node.
        check = send(accFD, message, strlen(message), 0);
        if (check < 0)
        {
            cout << "Error sending data." << endl;
            exit(EXIT_FAILURE);
        }

        // Update prev node, and prev node info.
        if (prevID != 0) // Don't connect to anything if no previous node.
        {
          int tempCNCT = initConnection(prevPort);
          memset(message, 0, BUFFSIZE);
          strcpy(message, "chgS ");

          memset(anotherTemp, 0, 20);
          sprintf(anotherTemp, "%d", tempID);
          strcat(message, anotherTemp);
          strcat(message, " ");

          memset(anotherTemp, 0, 20);
          sprintf(anotherTemp, "%d", tempPort);
          strcat(message, anotherTemp);

          // Sending information of new server to prev node. Prev node's new nxt.
          check = send(tempCNCT, message, strlen(message), 0);
          if (check < 0)
          {
              cout << "Error sending data." << endl;
              exit(EXIT_FAILURE);
          }

          close(tempCNCT);
        }

        if (nxtID == 0)
        {
          nxtID = tempID;
          nxtPort = tempPort;
        }

        prevID = tempID;
        prevPort = tempPort;

        memset(netBuff, 0, BUFFSIZE);
        // Recieving sunchronization message.
        check = recv(accFD, netBuff, BUFFSIZE, 0);
        if (check < 0)
        {
          cout << "Error receiving data." << endl;
          exit(EXIT_FAILURE);
        }

        // Send key values in new nodes range, and delete from bootstrap.
        int tempTraverse = rangeLow;
        while (tempTraverse <= tempID)
        {
          Node* tempNode = lookupKey(&list, tempTraverse);

          if (tempNode != nullptr)
          {
            memset(message, 0, BUFFSIZE);
            // Conerting the int to a char*.
            char anotherTemp[20];
            sprintf(anotherTemp, "%d", tempNode->key);
            strcat(message, anotherTemp);
            strcat(message, " ");

            strcat(message, tempNode->value);

            // Send message to node.
            check = send(accFD, message, strlen(message), 0);
            if (check < 0)
            {
              cout << "Error sending data." << endl;
              exit(EXIT_FAILURE);
            }

            // Deleting key from this server.
            deleteKey(tempTraverse);
            memset(message, 0, BUFFSIZE);

            // Recieving sunchronization message.
            check = recv(accFD, netBuff, BUFFSIZE, 0);
            if (check < 0)
            {
              cout << "Error receiving data." << endl;
              exit(EXIT_FAILURE);
            }
          }

          tempTraverse++;
        }

        // Send a finished command to the new server.
        check = send(accFD, "0000", 4, 0);
        if (check < 0)
        {
            cout << "Error sending data." << endl;
            exit(EXIT_FAILURE);
        }

        rangeLow = (tempID + 1); // Setting new rangelow.
      }
      else // Not the succesor.
      {
        // Send back information for next nameserver.
        memset(message, 0, BUFFSIZE);
        strcat(message, "nxt ");

        char anotherTemp[20];
        sprintf(anotherTemp, "%d", nxtID);
        strcat(message, anotherTemp);
        strcat(message, " ");

        memset(anotherTemp, 0, 20);
        sprintf(anotherTemp, "%d", nxtPort);
        strcat(message, anotherTemp);
        strcat(message, " btsp"); // Initial sequence.

        // Send message to node.
        check = send(accFD, message, strlen(message), 0);
        if (check < 0)
        {
            cout << "Error sending data." << endl;
            exit(EXIT_FAILURE);
        }
      }
    }
    else if ((strcmp(com, "ex") == 0)) // Exit.
    {
      // Msg struct: com prevID prevPort
      char anotherTemp[20];

      memset(anotherTemp, 0, 20); // For int conversions.
      com = strtok(NULL, " "); // Getting prevID.
      int tempID = stoi(com); // Converting to int.

      memset(anotherTemp, 0, 20); // For int conversions.
      com = strtok(NULL, " "); // Getting prevPort.
      int tempPort = stoi(com); // Converting to int.

      // Updating prev node information with new values.
      prevID = tempID;
      prevPort = tempPort;

      // Getting the keys.
      int tempKey;
      char tempValue[20];
      do
      {
        int check = send(accFD, "ok", 2, 0); // Sync message.
        if (check < 0)
        {
            cout << "Error sending data." << endl;
            exit(EXIT_FAILURE);
        }

        memset(netBuff, 0, BUFFSIZE);
        // Recieving message
        check = recv(accFD, netBuff, BUFFSIZE, 0);
        if (check < 0)
        {
          cout << "Error receiving data." << endl;
          exit(EXIT_FAILURE);
        }
        //cout << "enter netBuffer: " << netBuffer << endl;

        com = strtok(netBuff, " ");

        if ((strcmp(com, "0000") != 0))
        {
          // First would be the key. Convert to int.
          tempKey = stoi(com);

          com = strtok(NULL, " "); // Key value.
          memset(tempValue, 0, 20);
          strcpy(tempValue, com);

          insertKey(tempKey, tempValue);
        }

      } while ((strcmp(com, "0000") != 0));

      // New rangeLow.
      rangeLow = (prevID + 1);
    }
    else if ((strcmp(com, "chgS") == 0)) // Change successor in a enter case.
    {
      // Msg struct: com nodeID nodePort

      com = strtok(NULL, " "); // Getting nameID.
      int tempID = stoi(com); // Converting to int.

      com = strtok(NULL, " "); // Getting namePort.
      int tempPort = stoi(com); // Converting to int.

      // Update successor info.
      nxtID = tempID;
      nxtPort = tempPort;
    }
    else
    {
      cout << "You theoretically shoulnd't be able to get here." << endl;
    }

    close(accFD);
  }
  close(sockFD);
  return NULL;
} // bgThread()
