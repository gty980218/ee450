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
#include "hospital.h"
#include <map>
#include <sstream>

#define PORT_BASE 30188;
using namespace std;

struct Hspt_info{
    sockaddr_in addr; 
    int cpblt;
    int ocpt;
    double score;
    double dist;
};

class Scheduler{
    public:
        Hspt_info hspt_info[3];

    void init_info(int i,int cpblt,int ocp,sockaddr_in addr);
    void add_info(int i,double score,double dist);
};

void Scheduler::init_info(int i,int cpblt,int ocpt,sockaddr_in addr){
    this->hspt_info[i].cpblt=cpblt;
    this->hspt_info[i].ocpt=ocpt;
    this->hspt_info[i].addr=addr;
    this->hspt_info[i].score=-1;
}
void Scheduler::add_info(int i,double score,double dist){
    this->hspt_info[i].score=score;
    this->hspt_info[i].dist=dist;
}
void init_sche(int sock);

void tcp_conn(int sock_tcp,int sock_udp);

void serve(int conn,int sock_udp);
void reinit();