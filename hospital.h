#include <iostream>
#include <fstream>
#include <cassert>
#include <string.h>
#include <stdio.h>
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
using namespace std;

struct Dist_list{
    bool vstd;
    double val;
};

class Hospital{
    public:
        int capacity;
        int occupancy;
        int location;
        Dist_list *dist_list;

    Hospital(int capacity,int occupancy,Dist_list *dist_list,int location);
};

void ready(int sock,Hospital& Hospital);
void serve(int sock,Hospital& hospital);
double** readTxt(string file);
Dist_list* init(int loc);

