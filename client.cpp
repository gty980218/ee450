#include <iostream>
#include <fstream>
#include <cassert>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sstream>
using namespace std;

int main(int argc,char* argv[]) {
	int sock;
	if((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        fprintf(stderr,"Create TCP socket failed!\n");
        exit(1);
    }

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(34188); 
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	if(connect(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        fprintf(stderr,"Connetc failed!");
        exit(1);
    }
	
	cout<<"The client is up and running"<<endl;

	char sendbuf[1024] = {0};
	char recvbuf[1024] = {0};
	string str=argv[1];
	strcpy(sendbuf,str.c_str());
	write(sock, sendbuf, strlen(sendbuf));
	cout<<"The client has sent query to Scheduler using TCP: client location "<<str<<endl;

	read(sock, recvbuf, sizeof(recvbuf));
	string response=recvbuf;
	if(response.compare("NF")==0){
		cout<<"Location "<<str<<" not found"<<endl;
	}else if(response.compare("N")==0){
		cout<<"Score = None, No assignment"<<endl;
	}else if(response.compare("OL")==0){
		cout<<"All hospitals are unavailable due to overload"<<endl;
	}else{
		cout<<"The client has received results from the Scheduler: assigned to Hospital "<<recvbuf<<endl;
	}
	close(sock);
	
	return 0;
} 