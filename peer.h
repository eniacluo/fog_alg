#include <netinet/in.h>		// for sockaddr_in
#include <sys/types.h>		// for socket
#include <sys/socket.h>		// for socket
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>			// for getcwd
#include <sys/stat.h>		// for folder exists checking

#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <gsl/gsl_math.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>

//#define DEBUG_RECV 1
//#define SHOW_SEND 1
#define SHOW_RECV 1
//#define SHOW_MATRIX 1
#define SAVE_LOG 1

#define SERVER_PORT 6666
#define BUFFER_SIZE 64000
#define BLOCK_SIZE 64000

#define MATRIX_ROWS 256
#define MATRIX_COLS 4096

double alpha = 0.001;
int iterCount = 25;

char buffer[BUFFER_SIZE];
//set a socket address structure [client_addr] containing ip address and port
struct sockaddr_in client_addr;
//define the client socket address structure [server_addr]
struct sockaddr_in server_addr;
struct sockaddr_in broadcast_addr;
//a client socket identification
int client_socket;
char server_ip[20];
char my_ip[20];
//neighbor variables
char ip_neighbor[100][20];
int neighborCount = 0;
int isNeighborReady[200];
gsl_matrix *xNeighbor[200];

char runPath[200];
FILE *flog = NULL;

/**************Tool Function**************/
char * getTime();

void saveRunPath();

struct in_addr getMyIpAddress();

void getBcastIpAddress(char *bcast_ip, char *my_ip);

int getNodeIndex();

void delay(int second);

/**************Log File Function**************/
void initLog();

void writeLog(char *logType, char *logInfo);

/**************Socket Related Function**************/
int initSocket();

int bindSocket();

int setBroadcastEnable();

int closeSocket();

int sendData(int count);

int recvData();

int sendString(char *str);

int recvString();

/**************Manage Neighbor Function**************/
void broadcastHello();

void recvNeighborHello();

void printNeighbor();

void initxNeighbor();

gsl_matrix *getxNeighbor(int a, int b, int c, int d);

void setNeighborReady(int a, int b, int c, int d);

int checkNeighborReady();

int setAllNeighborReady(int isReady);

void discoverNeighbor();

/**************Matrix Function**************/
void printMatrix(gsl_matrix *m, int rows, int cols);

void readMatrix(char *filename, gsl_matrix *m, int rows, int cols);

void writeMatrix(char *filename, gsl_matrix *m, int rows, int cols);

void readMatrixFile(gsl_matrix *A, gsl_matrix *b);

void writeResultFile(gsl_matrix *x);

void multiplyMatrix(gsl_matrix *A, gsl_matrix *B, gsl_matrix *C, int dim1, int dim2, int dim3);

void computeGradient(gsl_matrix *A, gsl_matrix *x, gsl_matrix *b, gsl_matrix *g);

void sendMatrix(gsl_matrix *m, int rows, int cols, int index);

void recvMatrix(gsl_matrix *m, int rows, int cols);

void updateLocalx(gsl_matrix *x);

void computeNeighborAverage(gsl_matrix *x);

double computeNorm(gsl_matrix *x, int rows);

void computeError(gsl_matrix *A, gsl_matrix *b, gsl_matrix *x, int iterIndex);

void computeMatrix();

/**************Main Function**************/
int initArgs(int argc, char **argv);

int main(int argc, char **argv);