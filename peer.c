#include "peer.h"

char * getTime()
{
	static char timeStr[20];
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	sprintf(timeStr, "%d:%d:%d", tm.tm_hour, tm.tm_min, tm.tm_sec);
	return timeStr;
}

struct in_addr getMyIpAddress()
{
	int fd;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	 /* I want to get an IPv4 IP address */
	ifr.ifr_addr.sa_family = AF_INET;

	/* I want IP address attached to "eth0" */
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);

	ioctl(fd, SIOCGIFADDR, &ifr);

	close(fd);
	
	return ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
}

void getBcastIpAddress(char *bcast_ip, char *my_ip)
{
	int a, b, c;
	sscanf(my_ip, "%d.%d.%d.%*d", &a, &b, &c);
	sprintf(bcast_ip, "%d.%d.%d.%d", a, b, c, 255);
}

int getNodeIndex()
{
	int index;
	sscanf(my_ip, "%*d.%*d.%*d.%d", &index);
	return index;
}

void initLog()
{
	char filename[100];
	char logPath[100];
	
	filename[0] = '\0';

	struct stat st = {0};
	strcpy(logPath, runPath);
	strcat(logPath, "/log");
	
	//CHECK if log folder does not exist
	if(stat(logPath, &st) == -1)
	{
		mkdir(logPath, 0775);
	} 

	sprintf(filename, "%s/%s.txt", logPath, my_ip);
	flog = (FILE *) fopen(filename, "w");
	if(flog == NULL)
	{
		printf("Log file fails to establish!\n");
		exit(1);
	} else
		printf("Write Log to %s.\n", filename);
}

void writeLog(char *logType, char *logInfo)
{
	if(flog != NULL)
	{
		fprintf(flog, "[%s][%s][%s]\n", getTime(), logType, logInfo);
		fflush(flog);
	}
}

int initSocket()
{	
    //set a block of memory to zero
    bzero(&client_addr, sizeof(client_addr));
    client_addr.sin_family = AF_INET;	//For Internet protocols
    client_addr.sin_addr.s_addr = htons(INADDR_ANY);
    client_addr.sin_port = htons(SERVER_PORT);	//0 means to allocate an available port randomly

    //create a client socket based on UDP
    client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (client_socket < 0) {
    	printf("Create Socket Failed!\n");
    	exit(EXIT_FAILURE);
    } else
	   printf("Socket successfully established!\n");

	broadcast_addr.sin_family = AF_INET;
    //server ip address comes from the argument at the beginning
    if (inet_aton(server_ip, &broadcast_addr.sin_addr) == 0) 
    {
    	printf("Server IP Address Error!\n");
    	exit(EXIT_FAILURE);
    }
    broadcast_addr.sin_port = htons(SERVER_PORT);
	
}

int bindSocket()
{
    //bind the socket with a socket address structure
    if (bind(client_socket, (struct sockaddr *) &client_addr, sizeof(client_addr))) {
    	printf("Client Bind Port Failed!\n");
    	exit(EXIT_FAILURE);
    } else
	   printf("Client Port has been binded!\n");
}

//Enable the Broadcast 
int setBroadcastEnable()
{
	int broadcastEnable = 1;
	setsockopt(client_socket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
}

int closeSocket()
{
    close(client_socket);
    printf("Socket has been closed!\n");
}

int sendData(int count)
{
    if (sendto(client_socket, buffer, count, 0, 
    	(struct sockaddr *) &broadcast_addr, sizeof(broadcast_addr)) == -1)
       printf("Send Data To Server %s Failed!\n", server_ip);
}

int recvData()
{
	socklen_t addr_len = sizeof(server_addr);
    socklen_t length = recvfrom(client_socket, buffer, BUFFER_SIZE, MSG_DONTWAIT, 
    	(struct sockaddr *) &server_addr, &addr_len);
	#ifdef DEBUG_RECV
		if(length == -1)
			printf("errno: %d\n", errno);
		printf("recv length: %d\n", (int)length);
	#endif

    return (int)length;
}

int sendString(char *str)
{
    strcpy(buffer, str);
    sendData(strlen(str));
	#ifdef SHOW_SEND
	    printf("[%s] Send Data [%s]!\n", getTime(), buffer);
	#endif
	#ifdef SAVE_LOG
		writeLog("S", buffer);	
	#endif
}

int recvString()
{
    int length = recvData();
	#ifdef SHOW_RECV
		if(length > 0)
		{
			buffer[length] = '\0';
			if(length < 1024)
				printf("[%s] Recv Data [%s]!\n", getTime(), buffer);
			else
				printf("[%s] Recv Data %d bytes\n", getTime(), length);
		}		
	#endif
	#ifdef SAVE_LOG
		if(length > 0)
			writeLog("R", buffer);	
	#endif
    return length;
}

void delay(int second)
{
	int i, j;
	for(; second>=0; second--)
	{
		for(i=0; i<10000; i++)
			for(j=0; j<10000; j++)
				;
	}
}

void broadcastHello()
{
	static int iter = 0;
	char sendBuffer[BLOCK_SIZE];
	char sender_ip[BLOCK_SIZE];
	struct in_addr my_addr;

	my_addr = getMyIpAddress();
	inet_ntop(AF_INET, &my_addr, sender_ip, INET_ADDRSTRLEN);
	sprintf(sendBuffer, "#%s:Hello#", sender_ip);
	sendString(sendBuffer);
}

void recvNeighborHello()
{
	char temp[20];
	int find = 0;
	int a, b, c, d;
	int i;

	recvString();
	if(sscanf(buffer, "#%d.%d.%d.%d:", &a, &b, &c, &d) == 4)
	{
		sprintf(temp, "%d.%d.%d.%d", a, b, c, d);	
		for(i=0; i<neighborCount; i++)
		{
			if(strcmp(ip_neighbor[i], temp) == 0)
			{
				find = 1;
				break;
			}
		}
		// first time recv a neighbor Hello, save it
		if(find == 0)
			strcpy(ip_neighbor[neighborCount++], temp);
	}
}

void printNeighbor()
{
	int i;
	char temp[100];
	for(i=0; i<neighborCount; i++)
	{
		printf("Neighbor #%d: %s\n", i+1, ip_neighbor[i]);
		#ifdef SAVE_LOG
			sprintf(temp, "Neighbor #%d: %s", i+1, ip_neighbor[i]);
			writeLog("N", temp);	
		#endif
	}
}

void discoverNeighbor()
{
	int i;
	for(i=0; i<10; i++)
	{
		delay(1);
		broadcastHello();
		recvNeighborHello();
	}
	printNeighbor();
}

gsl_matrix *getxNeighbor(int a, int b, int c, int d)
{
	char temp[20];
	int find = 0;
	int i;

	sprintf(temp, "%d.%d.%d.%d", a, b, c, d);	
	//find the x vector corresponding to IP=(a.b.c.d)
	for(i=0; i<neighborCount; i++)
	{
		if(strcmp(ip_neighbor[i], temp) == 0)
			return xNeighbor[i];
	}
	return NULL;
}

void printMatrix(gsl_matrix *m, int rows, int cols)
{
	int i, j;
	for(i=0; i<rows; i++)
	{
		for(j=0; j<cols; j++)
		{
			printf("%f ", gsl_matrix_get(m, i, j));
		}
		printf("\n");
	}
}

void readMatrix(char *filename, gsl_matrix *m, int rows, int cols)
{
	int i, j;
	double data;

	FILE *f = fopen(filename, "r");
	for(i=0; i<rows; i++)
	{
		for(j=0; j<cols; j++)
		{
			fscanf(f, "%lf", &data);
			gsl_matrix_set(m, i, j, data);
		}
	}
	fclose(f);
}

void writeMatrix(char *filename, gsl_matrix *m, int rows, int cols)
{
	int i, j;
	double data;

	FILE *f = fopen(filename, "w");
	for(i=0; i<rows; i++)
	{
		for(j=0; j<cols; j++)
		{
			fprintf(f, "%lf ", gsl_matrix_get(m, i, j));
		}
		fprintf(f, "\n");
	}
	fclose(f);
}

void readMatrixFile(gsl_matrix *A, gsl_matrix *b)
{
	char filename[100];
	char dataPath[100];
	
	filename[0] = '\0';

	struct stat st = {0};
	strcpy(dataPath, runPath);
	strcat(dataPath, "/data");
	
	//CHECK if log folder does not exist
	sprintf(filename, "%s/A%d.txt", dataPath, getNodeIndex());
	if(stat(filename, &st) == -1)
	{
		printf("A matrix (%s) is missing!\n", filename);
		exit(1);
	} 
	readMatrix(filename, A, MATRIX_ROWS, MATRIX_COLS);

	sprintf(filename, "%s/b%d.txt", dataPath, getNodeIndex());
	if(stat(filename, &st) == -1)
	{
		printf("b vector (%s) is missing!\n", filename);
		exit(1);
	} 
	readMatrix(filename, b, MATRIX_ROWS, 1);
}

void writeResultFile(gsl_matrix *x)
{
	char filename[100];
	int i, j;
	char resultPath[100];
	
	filename[0] = '\0';

	struct stat st = {0};
	strcpy(resultPath, runPath);
	strcat(resultPath, "/result");
	
	//CHECK if log folder does not exist
	if(stat(resultPath, &st) == -1)
	{
		mkdir(resultPath, 0775);
	} 

	sprintf(filename, "%s/X%d.txt", resultPath, getNodeIndex());

	FILE * fp = fopen(filename, "w");
    for (i = 0; i < MATRIX_COLS; i++)
		for (j = 0; j < 1; j++) {
			//printf("X(%d,%d) = %g\n", i, j, gsl_matrix_get(x, i, j));
	    	fprintf(fp, "%g\n", gsl_matrix_get(x, i, j));
		}
	fclose(fp);
	printf("Save result to %s\n", filename);
}

void multiplyMatrix(gsl_matrix *A, gsl_matrix *B, gsl_matrix *C, int dim1, int dim2, int dim3)
{
	gsl_blas_dgemm (CblasNoTrans, CblasNoTrans, 1.0, A, B, 0.0, C);
}

void computeGradient(gsl_matrix *A, gsl_matrix *x, gsl_matrix *b, gsl_matrix *g)
{
	//calculate A'*b
	gsl_matrix *ATb = gsl_matrix_alloc(MATRIX_COLS, 1);
	gsl_blas_dgemm(CblasTrans, CblasNoTrans, 2, A, b, 0, ATb);

	//gfx=2*A'*A*x0-2*A'*b
	gsl_matrix *ATA = gsl_matrix_alloc(MATRIX_COLS, MATRIX_COLS);
	gsl_blas_dgemm(CblasTrans, CblasNoTrans, 2, A, A, 0, ATA);
	gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1, ATA, x, -2, ATb);
	
	gsl_matrix_memcpy(g, ATb);
	gsl_matrix_free(ATb);
	gsl_matrix_free(ATA);
}

void initxNeighbor()
{
	int i;
	for(i=0; i<neighborCount; i++)
	{
		xNeighbor[i] = gsl_matrix_alloc(MATRIX_COLS, 1);
		gsl_matrix_set_zero(xNeighbor[i]);
	}
}

void setNeighborReady(int a, int b, int c, int d)
{
	char temp[100];
	int i;
	
	sprintf(temp, "%d.%d.%d.%d", a, b, c, d);
	for(i=0; i<neighborCount; i++)
	{
		if(strcmp(ip_neighbor[i], temp) == 0)
			isNeighborReady[i] = 1;
	}
}

void sendMatrix(gsl_matrix *m, int rows, int cols, int index)
{
	int i, j;
	char sendBuffer[BLOCK_SIZE];
	char temp[100];

	sendBuffer[0] = '\0';
	strcat(sendBuffer, "#");
	strcat(sendBuffer, my_ip);
	strcat(sendBuffer, ":(");
	sprintf(temp, "%d", index);
	strcat(sendBuffer, temp);
	strcat(sendBuffer, ")[");
	for(i=0; i<rows; i++)
	{
		for(j=0; j<cols; j++)
		{
			sprintf(temp, "%g ", gsl_matrix_get(m, i, j));
			strcat(sendBuffer, temp);
		}
		strcat(sendBuffer, ";");
	}
	strcat(sendBuffer, "]#");
	sendString(sendBuffer);
}

void recvMatrix(gsl_matrix *m, int rows, int cols)
{
	int iterIndex;
	char temp[100];
	int a, b, c, d;//for 4 parts of IP address
	int i, j;
	double item;

	int strIndex = 0;
	if(recvString() > 0)
	{
		//Matrix data, not Hello
		if(sscanf(buffer, "#%d.%d.%d.%d:(%d)", &a, &b, &c, &d, &iterIndex) == 5)
		{
			m = getxNeighbor(a, b, c, d);
			//skip header part
			strIndex += sprintf(temp, "#%d.%d.%d.%d:(%d)[", a, b, c, d, iterIndex);
			for(i=0; i<rows; i++)
			{
				for(j=0; j<cols; j++)
				{
					sscanf(buffer+strIndex, "%lf", &item);
					gsl_matrix_set(m, i, j, item);
					strIndex += sprintf(temp, "%g ", item);
				}
				strIndex++;//skip ;
			}
			printf("sender[%d] at iter[%d]:\n", d, iterIndex);
			#ifdef SHOW_MATRIX		
				printMatrix(m, MATRIX_ROWS, MATRIX_COLS);
			#endif			
			setNeighborReady(a, b, c, d);
		}
	}
}

void updataLocalx(gsl_matrix *x)
{
	int a, b, c, d;
	int i, j;

	sscanf(my_ip, "%d.%d.%d.%d", &a, &b, &c, &d);
	gsl_matrix * m = getxNeighbor(a, b, c, d);
	for(i=0; i<MATRIX_COLS; i++)
	{
		for(j=0; j<1; j++)
		{
			gsl_matrix_set(m, i, j, gsl_matrix_get(x, i, j));
		}
	}
}

int checkNeighborReady()
{
	int allReady = 1;
	int i;

	for(i=0; i<neighborCount; i++)
	{
		if(isNeighborReady[i] == 0)
			allReady *= 0;
	}
	return allReady;
}

int setAllNeighborReady(int isReady)
{
	int i;

	for(i=0; i<neighborCount; i++)
	{
		isNeighborReady[i] = isReady;
	}
}

void computeNeighborAverage(gsl_matrix *x)
{
	int i, j;
	gsl_matrix_set_zero(x);
	for(i=0; i<neighborCount; i++)
	{
		gsl_matrix_add(x, xNeighbor[i]);
	}
	
	gsl_matrix_scale(x, 1.0/neighborCount); // average
}

double computeNorm(gsl_matrix *x, int rows)
{
	int i;
	double norm = 0;
	for(i=0; i<rows; i++)
	{
		norm += gsl_matrix_get(x, i, 0)*gsl_matrix_get(x, i, 0);
	}
	return norm;
}

void computeError(gsl_matrix *A, gsl_matrix *b, gsl_matrix *x, int iterIndex)
{
	double norm;
	char temp[200];
	gsl_matrix *Ax = gsl_matrix_alloc(MATRIX_ROWS, 1);

	gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1, A, x, 0, Ax);
	gsl_matrix_sub(Ax, b);
	norm = computeNorm(Ax, MATRIX_ROWS);
	printf("iter(%d)error: %lf\n", iterIndex, norm);
	#ifdef SAVE_LOG
		sprintf(temp, "iter(%d)error: %lf", iterIndex, norm);
		writeLog("I", temp);	
	#endif
}

void computeMatrix()
{
	int iterIndex = 0;
	int i, j;
	double k;
	int timeout = 0;

	gsl_matrix *A = gsl_matrix_alloc(MATRIX_ROWS, MATRIX_COLS);
	gsl_matrix *b = gsl_matrix_alloc(MATRIX_ROWS, 1);
	gsl_matrix *x = gsl_matrix_alloc(MATRIX_COLS, 1);
	gsl_matrix *g = gsl_matrix_alloc(MATRIX_COLS, 1);

	readMatrixFile(A, b);

	setAllNeighborReady(1);
	gsl_matrix_set_zero(x);
	initxNeighbor();
	while(iterIndex <= iterCount)
	{
		recvMatrix(NULL, MATRIX_COLS, 1);
		if(checkNeighborReady())
		{
			setAllNeighborReady(0);
			//x_k+0.5=1/n*(x1+x2+...+xn)
			updataLocalx(x);
			computeNeighborAverage(x);
			computeGradient(A, x, b, g);
			//alpha*(1/k)*gradient
			gsl_matrix_scale(g, alpha/(iterIndex+1));
			//x_k+1=x_k+0.5-alpha*(1/k)*gradient
			gsl_matrix_sub(x, g);
			computeError(A, b, x, iterIndex);
			sendMatrix(x, MATRIX_COLS, 1, iterIndex++);
		}
		
		else
		{
			timeout++;
			if(timeout > 10)
			{
				timeout = 0;
				setAllNeighborReady(1);
			}
		}
		
	}
	writeResultFile(x);
}

int initArgs(int argc, char **argv)
{
	strcpy(runPath, argv[0]);
	//save the runPath, remove the executable file name
	char *r = strrchr(runPath, '/');
	*r = '\0';
	//save my ip of eth0
	strcpy(my_ip, inet_ntoa(getMyIpAddress()));
	//save the broadcast ip address of eth0
	getBcastIpAddress(server_ip, my_ip);
    if (argc >= 2) {
    	sscanf(argv[1], "%lf", &alpha);
    	if(argc >= 3)
			sscanf(argv[2], "%d", &iterCount);
    }
}

int main(int argc, char **argv)
{
    initArgs(argc, argv);
	#ifdef SAVE_LOG
	initLog();
	#endif

    initSocket();
    bindSocket();
    setBroadcastEnable();
	discoverNeighbor();
	
	computeMatrix();

    closeSocket();

    return 0;
}
