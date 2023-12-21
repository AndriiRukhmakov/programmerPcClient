#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <jansson.h>

#ifndef JSON_HUMAN
#define JSON_HUMAN (JSON_INDENT(4) | JSON_SORT_KEYS)
#endif

//#define PATH_TO_FW        "/Users/andrii/test.txt"
#define PATH_TO_FW        "/Users/andrii/Flash_data.bin"
#define SERVER_ADDR       "192.168.1.113"
#define SERVER_PORT       8888U
#define NAND_PAGE_SIZE    2048U

/******************************************************************************************
 * Check if FW file exist using provided path
 ******************************************************************************************/
int
checkExistingFirmware(void)
{
    FILE *fp;

    fp = fopen(PATH_TO_FW, "r");

    if (NULL == fp)
    {
        printf("\n\rError: Wrong FW path! %s\n\r", PATH_TO_FW);
        return 1;
    }

//TODO Check file size.

    fclose(fp);

	return 0;
}

/******************************************************************************************
 * Connection to server and obtaining of connection descriptor
 ******************************************************************************************/
int
connectToServer(int *sockfd)
{
    struct sockaddr_in serv_addr;

    if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n\rError: Can't create socket!\n\r");
        return 1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_ADDR, &serv_addr.sin_addr)<=0)
    {
        printf("\n\rError: inet_pton error occured!\n\r");
        return 1;
    }

    if (connect(*sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n\rError: Connection failed!\n\r");
       return 1;
    }

	return 0;
}

/******************************************************************************************
 * Initiation of necessary process on server
 ******************************************************************************************/
int
serverInitiateProcess(const char *name, const char *response)
{
	int     sockfd;
	json_t *obj = json_object();
	int     len = 0;
	char    buff[NAND_PAGE_SIZE];


	connectToServer(&sockfd);

    memset(buff, 0, sizeof(buff));

	json_object_set_new(obj, "status", json_string(name));
	len = json_dumpb(obj, buff, sizeof(buff), 0);

    write(sockfd, buff, len);

	json_decref(obj);

    memset(buff, 0, sizeof(buff));

    len = recv(sockfd, buff, sizeof(buff), 0);

    close(sockfd);

    printf("\n\rResponse - %s\n\r", buff);

    if (-1 == len)
    {
        printf("\n\rError: Unable to get status response from server!\n\r");
        return 1;
    }
    else if (0 != strcmp(response, buff))
    {
        printf("\n\rError: Incorrect response from server!\n\r");
        return 1;
    }

	return 0;
}

/******************************************************************************************
 * Transmittion FW to server
 ******************************************************************************************/
int
flashProcess(void)
{
    FILE *fp;
    int   sockfd;
	int   len = 0;
	char  buff[NAND_PAGE_SIZE];
	char  respBuff[256];

    fp = fopen(PATH_TO_FW, "r");

    if (NULL == fp)
    {
        printf("\n\rError: Wrong FW path! %s\n\r", PATH_TO_FW);
        return 1;
    }

READ_FILE:
    memset(buff, 0, NAND_PAGE_SIZE);
    len = fread(buff, 1, NAND_PAGE_SIZE, fp);

    if (0 == len)
    {
        fclose(fp);
    	return 0;
    }

WRITE_TO_SOCKET:
    connectToServer(&sockfd);
    write(sockfd, buff, len);
    memset(respBuff, 0, sizeof(respBuff));
    len = recv(sockfd, respBuff, sizeof(respBuff), 0);
    close(sockfd);

    if (-1 == len)
    {
        fclose(fp);
    	close(sockfd);
        printf("\n\rError: Unable to get status response from server!\n\r");
        return 1;
    }
    else if (0 == strcmp("ok", respBuff))
    {
         goto READ_FILE;
    }
    else if (0 == strcmp("nandIsBusy", respBuff))
    {
        goto WRITE_TO_SOCKET;
    }
    else
    {
        fclose(fp);
    	close(sockfd);
        printf("\n\rError: Incorrect response from server!\n\r");
        return 1;
    }

    fclose(fp);

	return 0;
}

/******************************************************************************************
 * main function
 ******************************************************************************************/
int main(int argc, char *argv[])
{
    if (checkExistingFirmware())
    {
    	return 1;
    }

    if (serverInitiateProcess("start", "startFlash"))
    {
    	return 1;
    }

    if (flashProcess())
    {
    	return 1;
    }

    if (serverInitiateProcess("stop", "stopFlash"))
    {
    	return 1;
    }

    return 0;
}


