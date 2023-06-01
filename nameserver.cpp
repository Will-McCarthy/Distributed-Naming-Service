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

struct CircularList{
  Node* head;
};

int nameId, namePort, bootPort, bootId;
int prevPort, prevID, nxtPort, nxtID; // Info of previous, and next nodes.
int rangeLow, rangeHigh; // Range of values this nameserver is responsible for.
char* bootIp;
CircularList list; // Store keys.

void config(char* fileName); // Parse config.
int initConnection(int port); // Creating server connection.
Node* allocNode();
void initList(CircularList* clist);
void insertNode(Node* node);
Node* lookupKey(const CircularList* clist, int key);
void insertKey(int key, char value[20]);
bool deleteKey(int key);
void print(const CircularList* clist);
void* bgThread(void* arg); // Thread for connection handling.

int main(int argc, char* argv[]){
  if (argc != 2){
    cout << "Usage: ./nameserver [config_file_path]" << endl;
    return EXIT_FAILURE;
  }

  // Initialize a circular list to store name servers
  initList(&list);
  config(argv[1]);

  // Thread for background connections.
  pthread_t backgdThread;
  pthread_create(&backgdThread, NULL, bgThread, NULL);

  char netBuffer[BUFFSIZE], messageBuffer[BUFFSIZE]; // Net, and message buffs.
  char* com, seq[32]; // For commands, and traversal sequence.

  int run = 1;
  while(run == 1)
  {
    // Resetting buffers.
    memset(netBuffer, 0, BUFFSIZE);
    memset(messageBuffer, 0, BUFFSIZE);

    printf("Commands available: (1) enter, (2) exit: \n");

    int command = 0;
    int key = 0;
    scanf("%d", &command);

    if(command == 1)  //enter
    {
      // Building the initial message for bootstrap.
      strcpy(messageBuffer, "en ");

      char tempThing[10];
      sprintf(tempThing, "%d", nameId);
      strcat(messageBuffer, tempThing);
      strcat(messageBuffer, " ");

      memset(tempThing, 0, 10);
      sprintf(tempThing, "%d", namePort);
      strcat(messageBuffer, tempThing);

      // Connect to bootstrap, and send message.
      // ID, and port of enter server. Bootstrap at first.
      int enterID = bootId, enterPort = bootPort;
      int cnct = initConnection(enterPort);

      int check = send(cnct, messageBuffer, strlen(messageBuffer), 0);
      if (check < 0)
      {
          cout << "Error sending data." << endl;
          exit(EXIT_FAILURE);
      }

      // Recieving message
      check = recv(cnct, netBuffer, BUFFSIZE, 0);
      if (check < 0)
      {
        cout << "Error receiving data." << endl;
        exit(EXIT_FAILURE);
      }
      cout << "first: " << netBuffer << endl;

      // Extracting command.
      com = strtok(netBuffer, " ");

      // Connect to next nameserver, and try again.
      while ((strcmp(com, "ok") != 0))
      {
        com = strtok(NULL, " "); // ID of next server.
        enterID = stoi(com); // Converting to int.

        com = strtok(NULL, " "); // Port of next server.
        enterPort = stoi(com); // Converting to int.

        com = strtok(NULL, " ");
        memset(seq, 0, 32);
        strcpy(seq, com);
        cout << "seq: " << seq << endl;

        close(cnct); // Closing connection.

        memset(messageBuffer, 0, BUFFSIZE);
        strcpy(messageBuffer, "en ");
        sprintf(tempThing, "%d", nameId);
        strcat(messageBuffer, tempThing);
        strcat(messageBuffer, " ");

        memset(tempThing, 0, 10);
        sprintf(tempThing, "%d", namePort);
        strcat(messageBuffer, tempThing);
        strcat(messageBuffer, " ");
        strcat(messageBuffer, seq);
        cout << "messageBuff: " << messageBuffer << endl;

        cnct = initConnection(enterPort);

        int check = send(cnct, messageBuffer, strlen(messageBuffer), 0);
        if (check < 0)
        {
            cout << "Error sending data." << endl;
            exit(EXIT_FAILURE);
        }

        // Recieving message
        check = recv(cnct, netBuffer, BUFFSIZE, 0);
        if (check < 0)
        {
          cout << "Error receiving data." << endl;
          exit(EXIT_FAILURE);
        }
        cout << "SecondL: " << netBuffer << endl;

        // Extracting command.
        com = strtok(netBuffer, " ");
      }

      // Start updating next/prev info, and accept key values.
      nxtID = enterID;
      nxtPort = enterPort;

      com = strtok(NULL, " "); // ID of prev server.
      enterID = stoi(com); // Converting to int.

      com = strtok(NULL, " "); // Port of prev server.
      enterPort = stoi(com); // Converting to int.

      prevID = enterID; // Reusing enter* vars instead of making new ones.
      prevPort = enterPort;

      com = strtok(NULL, " ");
      memset(seq, 0, 32);
      strcpy(seq, com);

      // Getting the keys.
      int tempKey;
      char tempValue[20];
      do
      {
        int check = send(cnct, "ok", 2, 0); // Sync message.
        if (check < 0)
        {
            cout << "Error sending data." << endl;
            exit(EXIT_FAILURE);
        }

        memset(netBuffer, 0, BUFFSIZE);
        // Recieving message
        check = recv(cnct, netBuffer, BUFFSIZE, 0);
        if (check < 0)
        {
          cout << "Error receiving data." << endl;
          exit(EXIT_FAILURE);
        }

        com = strtok(netBuffer, " ");

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

      rangeLow = (prevID + 1);
      rangeHigh = nameId;

      close(cnct);

      cout << "Successful entry" << endl;
      cout << "Next Node ID: " << nxtID << ", previous Node ID: "
        << prevID << endl;
      cout << "Traversal: " << seq << endl;
    }
    else if(command == 2)
    {
      memset(messageBuffer, 0, BUFFSIZE);
      char anotherTemp[20];

      strcpy(messageBuffer, "ex ");

      memset(anotherTemp, 0, 20); // For int conversions.
      sprintf(anotherTemp, "%d", prevID); // Previous ID.
      strcat(messageBuffer, anotherTemp);
      strcat(messageBuffer, " ");

      memset(anotherTemp, 0, 20); // For int conversions.
      sprintf(anotherTemp, "%d", prevPort); // Previous port.
      strcat(messageBuffer, anotherTemp);

      int cnct = initConnection(nxtPort);

      int check = send(cnct, messageBuffer, strlen(messageBuffer), 0);
      if (check < 0)
      {
          cout << "Error sending data." << endl;
          exit(EXIT_FAILURE);
      }

      memset(netBuffer, 0, BUFFSIZE);
      // Recieving sunchronization message.
      check = recv(cnct, netBuffer, BUFFSIZE, 0);
      if (check < 0)
      {
        cout << "Error receiving data." << endl;
        exit(EXIT_FAILURE);
      }

      // Sending key values.
      int tempTraverse = rangeLow;
      while (tempTraverse <= rangeHigh)
      {
        Node* tempNode = lookupKey(&list, tempTraverse);

          if (tempNode != nullptr)
          {
            memset(messageBuffer, 0, BUFFSIZE);
            // Conerting the int to a char*.
            memset(anotherTemp, 0, 20);
            sprintf(anotherTemp, "%d", tempNode->key);
            strcat(messageBuffer, anotherTemp);
            strcat(messageBuffer, " ");

            strcat(messageBuffer, tempNode->value);

            // Send message to node.
            check = send(cnct, messageBuffer, strlen(messageBuffer), 0);
            if (check < 0)
            {
              cout << "Error sending data." << endl;
              exit(EXIT_FAILURE);
            }

            // Deleting key from this server.
            deleteKey(tempTraverse);
            memset(messageBuffer, 0, BUFFSIZE);

            // Recieving synchronization message.
            check = recv(cnct, netBuffer, BUFFSIZE, 0);
            if (check < 0)
            {
              cout << "Error receiving data." << endl;
              exit(EXIT_FAILURE);
            }
          }

          tempTraverse++;
      }

      // Send a finished command to the new server.
      check = send(cnct, "0000", 4, 0);
      if (check < 0)
      {
          cout << "Error sending data." << endl;
          exit(EXIT_FAILURE);
      }

      close(cnct);
      sleep(0.2);
      cnct = initConnection(prevPort);

      // Now send chgS message to prev node.
      memset(messageBuffer, 0, BUFFSIZE);

      strcpy(messageBuffer, "chgS ");
      
      memset(anotherTemp, 0, 20);
      sprintf(anotherTemp, "%d", nxtID);
      strcat(messageBuffer, anotherTemp);
      strcat(messageBuffer, " ");

      memset(anotherTemp, 0, 20);
      sprintf(anotherTemp, "%d", nxtPort);
      strcat(messageBuffer, anotherTemp);

      check = send(cnct, messageBuffer, strlen(messageBuffer), 0);
      if (check < 0)
      {
          cout << "Error sending data." << endl;
          exit(EXIT_FAILURE);
      }

      close(cnct);

      // Printing out some info.
      cout << "Successful exit" << endl;
      cout << "Successor ID: " << nxtID << endl;
      cout << "Key range handed over: " << rangeLow << "-" << rangeHigh
        << endl;

      nxtID = -1;
      nxtPort = -1;
      prevID = -1;
      prevPort = -1;
    }
    else{
      printf("Command not accepted\n");
    }
  }

  return EXIT_SUCCESS;
}

void config(char* fileName){ // Getting data from the configuration file.

    ifstream conFile(fileName);
    string line;

    // Name server Id
    getline(conFile, line);
    nameId = stoi(line);

    // Name server port number
    getline(conFile, line);
    namePort = stoi(line);

    // IP address and port number of bootstrap server
    getline(conFile, line);
    const char* temp = line.c_str();
    char* value = strdup(temp);
    bootIp = strtok(value, " ");
    bootPort = atoi(strtok(NULL, " "));
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
  Node* initNode = allocNode();
  initNode->key = -1;
  memset(&initNode->value, 0, 20);
  clist->head = initNode;
  initNode->prev = initNode->next = initNode;
}

void insertNode(Node* node){ // Insert a new node into the circular linked list

  printf("Successful entry\n");

  // Traverse the list of name servers until a name server corresponding to the request is retrieved
  Node* ptr = list.head->next;
  while(ptr != list.head){
    if(node->key > ptr->key){
      ptr = ptr->next;
    }
    else{
      break;
    }
  }

  // Set predecessor and successor for newly added name server
  ptr->prev->next = node;
  node->prev = ptr->prev;
  node->next = ptr;
  ptr->prev = node;
}

Node* lookupKey(const CircularList* clist, int key){ // Search a key from a list storing all key, value pairs

  Node* ptr = clist->head->next;
  bool found = false;
  char* value;
  while(ptr != clist->head){
    if(ptr->key == key){
      found = true;
      break;
    }
    ptr = ptr->next;
  }
  if(found){
    //printf("Corresponding value is : %s\n", value);

    //At this point, a key is found from the list storing all key, value pairs.
    //Have to send this key to each available naming server to check which naming server handles it
    return ptr;
  }else{
    printf("Key not found\n");
    return NULL;
  }
}

void insertKey(int key, char value[20]){ // Insert a new key, value pair to the list storing all key value pairs
  Node* node = allocNode();
  node->key = key;
  strcpy(node->value, value);
  insertNode(node);
}

bool deleteKey(int key){ // Delete a key, value pair of the given key from a list of all key, value pairs
  Node* ptr = lookupKey(&list, key);
  if(ptr != NULL){ // Key to be deleted exists
    ptr->prev->next = ptr->next;
    ptr->next->prev = ptr->prev;

    free(ptr);
    return true;
  }
  else
  {
    return false;
  }
}


void* bgThread(void* arg) // Handles connections, and their commands.
{
  // Converting the int to a char*.
  char portChar[10];
  sprintf(portChar, "%d", namePort);

  struct addrinfo srvAddr, *conInfo;

  //create the socket
  int sockFD = socket(AF_INET, SOCK_STREAM, 0); // Getting socket fd.
  if (sockFD < 0)
  {
    cout << "Error creating Socket." << endl;
    exit(EXIT_FAILURE);
  }

  //Getting stuff setup for socket binding
  memset(&srvAddr, 0, sizeof(srvAddr));
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
  check = listen(sockFD, 5);
  if (check < 0)
  {
    cout << "Error listening." << endl;
    exit(EXIT_FAILURE);
  }

  // Client connection stuff.
  socklen_t cltAddrSize = 0;
  struct sockaddr cltAddr;

  char netBuff[BUFFSIZE], message[BUFFSIZE]; // Net, and message buffs.
  char* bgCom;
  int accFD;

  // Always listen for connections, and handle them.
  while (true)
  {
    // Resetting buffers.
    memset(netBuff, 0, BUFFSIZE);
    memset(message, 0, BUFFSIZE);

    char anotherTemp[20];

    // Waiting for connections.
    accFD = accept(sockFD, &cltAddr, &cltAddrSize);
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

    bgCom = strtok(netBuff, " "); // Getting command.
    printf("Command is: ");
    printf("%s \n", bgCom);

    if ((strcmp(bgCom, "lkp") == 0)) // Lookup.
    {
      // Msg struct: com keyVal sequence

      memset(anotherTemp, 0, 20); // For int conversions.
      bgCom = strtok(NULL, " "); // Getting keyVal
      int key = stoi(bgCom); // Converting to int.

      if (key >= rangeLow && key <= rangeHigh) // In current node's range.
      {
        // Check for key.
        // If found, let bootstrap know.
        // If not found, send bootstrap a nf message.

        Node* tempNode = lookupKey(&list, key);

        if (tempNode != nullptr)
        {
          strcpy(message, "lkp ");

          // locID
          memset(anotherTemp, 0, 20);
          sprintf(anotherTemp, "%d", nameId);
          strcat(message, anotherTemp);
          strcat(message, " ");

          // keyVal
          strcat(message, tempNode->value);
          strcat(message, " ");
        }
        else // If not in the list.
        {
          strcpy(message, "nf ");
        }

        // Add current nameserver to sequence list.
        bgCom = strtok(NULL, " "); // Getting sequence
        strcat(message, bgCom);
        memset(anotherTemp, 0, 20);
        sprintf(anotherTemp, "%d", nameId);
        strcat(message, "->");
        strcat(message, anotherTemp);

        // Still need to alert bootsrtrap.
        int cnct = initConnection(bootPort);

        check = send(cnct, message, strlen(message), 0);
        if (check < 0)
        {
            cout << "Error sending data." << endl;
            exit(EXIT_FAILURE);
        }

        close(cnct); // Only need to send the message, nothing else.
      }
      else // Not in current Node's range, move on to next node.
      {
        strcat(message, "lkp ");

        // Converting the int to a char*.
        memset(anotherTemp, 0, 20);
        sprintf(anotherTemp, "%d", key);
        strcat(message, anotherTemp);
        strcat(message, " ");

        bgCom = strtok(NULL, " "); // Getting sequence
        strcat(message, bgCom);
        memset(anotherTemp, 0, 20);
        sprintf(anotherTemp, "%d", nameId);
        strcat(message, "->");
        strcat(message, anotherTemp);

        // Sending message to next nameserver.
        // Need to open connection to next server, and send message to it.
        int cnct = initConnection(nxtPort); // Connecting to server.

        check = send(cnct, message, strlen(message), 0);
        if (check < 0)
        {
            cout << "Error sending data." << endl;
            exit(EXIT_FAILURE);
        }

        close(cnct); // Only need to send the message, nothing else.
      }
    }
    else if ((strcmp(bgCom, "ins") == 0)) // Insert.
    {
      // Msg struct: com keyVal value seq

      bgCom = strtok(NULL, " "); // Getting keyVal
      int key = stoi(bgCom); // Converting to int.

      if (key >= rangeLow && key <= rangeHigh) // In current node's range.
      {
        // Getting value.
        memset(anotherTemp, 0, 20);
        bgCom = strtok(NULL, " "); // Getting value.
        strcpy(anotherTemp, bgCom);

        insertKey(key, anotherTemp);

        // Alerting bootstrap.
        memset(message, 0, BUFFSIZE);
        strcpy(message, "ins ");

        memset(anotherTemp, 0, 20);
        sprintf(anotherTemp, "%d", nameId);
        strcat(message, anotherTemp);
        strcat(message, " ");

        bgCom = strtok(NULL, " "); // Getting sequence.
        strcat(message, bgCom);
        strcat(message, "->");
        strcat(message, anotherTemp);

        // Sending infor to bootstrap.
        int cnct = initConnection(bootPort); // Connecting to server.

        check = send(cnct, message, strlen(message), 0);
        if (check < 0)
        {
            cout << "Error sending data." << endl;
            exit(EXIT_FAILURE);
        }

        close(cnct); // Only need to send the message, nothing else.
      }
      else
      {
        strcat(message, "ins ");

        // Converting the int to a char*.
        memset(anotherTemp, 0, 20);
        sprintf(anotherTemp, "%d", key);
        strcat(message, anotherTemp);
        strcat(message, " ");

        bgCom = strtok(NULL, " "); // Getting value.
        strcat(message, bgCom);
        strcat(message, " ");

        bgCom = strtok(NULL, " "); // Getting sequence.
        strcat(message, bgCom);
        memset(anotherTemp, 0, 20);
        sprintf(anotherTemp, "%d", nameId);
        strcat(message, "->");
        strcat(message, anotherTemp);

        // Sending message to next nameserver.
        // Need to open connection to next server, and send message to it.
        int cnct = initConnection(nxtPort); // Connecting to server.

        check = send(cnct, message, strlen(message), 0);
        if (check < 0)
        {
            cout << "Error sending data." << endl;
            exit(EXIT_FAILURE);
        }

        close(cnct); // Only need to send the message, nothing else.
      }
    }
    else if ((strcmp(bgCom, "del") == 0)) // Delete.
    {
      // Msg struct: com keyVal sequence

      memset(anotherTemp, 0, 20); // For int conversions.
      bgCom = strtok(NULL, " "); // Getting keyVal
      int key = stoi(bgCom); // Converting to int.

      if (key >= rangeLow && key <= rangeHigh) // In current node's range.
      {
        if (deleteKey(key)) // If deleted.
        {
          strcpy(message, "del ");
        }
        else // If key does not exist.
        {
          strcpy(message, "nf ");
        }

        // Add current nameserver to sequence list.
        bgCom = strtok(NULL, " "); // Getting sequence
        strcat(message, bgCom);
        memset(anotherTemp, 0, 20);
        sprintf(anotherTemp, "%d", nameId);
        strcat(message, "->");
        strcat(message, anotherTemp);

        // Still need to alert bootsrtrap.
        int cnct = initConnection(bootPort);

        check = send(cnct, message, strlen(message), 0);
        if (check < 0)
        {
            cout << "Error sending data." << endl;
            exit(EXIT_FAILURE);
        }

        close(cnct); // Only need to send the message, nothing else.
      }
      else
      {
        strcat(message, "del ");

        // Converting the int to a char*.
        memset(anotherTemp, 0, 20);
        sprintf(anotherTemp, "%d", key);
        strcat(message, anotherTemp);
        strcat(message, " ");

        bgCom = strtok(NULL, " "); // Getting sequence
        strcat(message, bgCom);
        memset(anotherTemp, 0, 20);
        sprintf(anotherTemp, "%d", nameId);
        strcat(message, "->");
        strcat(message, anotherTemp);

        // Sending message to next nameserver.
        // Need to open connection to next server, and send message to it.
        int cnct = initConnection(nxtPort); // Connecting to server.

        check = send(cnct, message, strlen(message), 0);
        if (check < 0)
        {
            cout << "Error sending data." << endl;
            exit(EXIT_FAILURE);
        }

        close(cnct); // Only need to send the message, nothing else.
      }
    }
    else if ((strcmp(bgCom, "en") == 0)) // Enter.
    {
      // Msg struct: com nameID namePort sequence
      bgCom = strtok(NULL, " "); // Getting nameID.
      int tempID = stoi(bgCom); // Converting to int.

      bgCom = strtok(NULL, " "); // Getting namePort.
      int tempPort = stoi(bgCom); // Converting to int.

      bgCom = strtok(NULL, " "); // Getting sequence.

      if (tempID > rangeLow && tempID < rangeHigh) // If this is to be the succesor.
      {
        // Alert server that this will be it's succesor, and update prev srv.
        memset(message, 0, BUFFSIZE);
        strcpy(message, "ok ");

        memset(anotherTemp, 0, 20);
        sprintf(anotherTemp, "%d", prevID);
        strcat(message, anotherTemp);
        strcat(message, " ");

        memset(anotherTemp, 0, 20);
        sprintf(anotherTemp, "%d", prevPort);
        strcat(message, anotherTemp);
        strcat(message, " ");

        // Add current nameserver to sequence list.
        strcat(message, bgCom);
        memset(anotherTemp, 0, 20);
        sprintf(anotherTemp, "%d", nameId);
        strcat(message, "->");
        strcat(message, anotherTemp);

        cout << "message: " << message << endl;

        // Send message to node.
        check = send(accFD, message, strlen(message), 0);
        if (check < 0)
        {
            cout << "Error sending data." << endl;
            exit(EXIT_FAILURE);
        }

        // Update prev, and next node info, and prev server.
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
            memset(anotherTemp, 0, 20);
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

        memset(anotherTemp, 0, 20);
        sprintf(anotherTemp, "%d", nxtID);
        strcat(message, anotherTemp);
        strcat(message, " ");

        memset(anotherTemp, 0, 20);
        sprintf(anotherTemp, "%d", nxtPort);
        strcat(message, anotherTemp);
        strcat(message, " ");


        // Add current nameserver to sequence list.
        strcat(message, bgCom);
        memset(anotherTemp, 0, 20);
        sprintf(anotherTemp, "%d", nameId);
        strcat(message, "->");
        strcat(message, anotherTemp);


        // Send message to node.
        check = send(accFD, message, strlen(message), 0);
        if (check < 0)
        {
            cout << "Error sending data." << endl;
            exit(EXIT_FAILURE);
        }
      }
    }
    else if ((strcmp(bgCom, "ex") == 0)) // Exit.
    {
      // Msg struct: com prevID prevPort
      char anotherTemp[20];

      memset(anotherTemp, 0, 20); // For int conversions.
      bgCom = strtok(NULL, " "); // Getting prevID.
      int tempID = stoi(bgCom); // Converting to int.

      memset(anotherTemp, 0, 20); // For int conversions.
      bgCom = strtok(NULL, " "); // Getting prevPort.
      int tempPort = stoi(bgCom); // Converting to int.

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
        //cout << "enter netBuff: " << netBuff << endl;

        bgCom = strtok(netBuff, " ");

        if ((strcmp(bgCom, "0000") != 0))
        {
          // First would be the key. Convert to int.
          tempKey = stoi(bgCom);

          bgCom = strtok(NULL, " "); // Key value.
          memset(tempValue, 0, 20);
          strcpy(tempValue, bgCom);

          insertKey(tempKey, tempValue);
        }

      } while ((strcmp(bgCom, "0000") != 0));

      // New rangeLow.
      rangeLow = (prevID + 1);
    }
    else if ((strcmp(bgCom, "chgS") == 0)) // Change successor in a enter case.
    {
      // Msg struct: com nodeID nodePort

      bgCom = strtok(NULL, " "); // Getting nameID.
      int tempID = stoi(bgCom); // Converting to int.

      bgCom = strtok(NULL, " "); // Getting namePort.
      int tempPort = stoi(bgCom); // Converting to int.

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
