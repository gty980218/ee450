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
using namespace std;
map<int, int> indice_map;                               // the mapping of location # and indice


int main(int argc, char *argv[]){
    if(argc!=4){
        fprintf(stderr,"Missing location,capacity or occupancy parameters!\n");
        exit(1);
    }

    int size=indice_map.size();
    Hospital hospital(atoi(argv[2]),atoi(argv[3]),init(atoi(argv[1])),atoi(argv[1]));
    //create socket 
	int sock;
	if((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0){
        fprintf(stderr,"Create socket failed!\n");
        exit(1);
    }                                                       //SOCK_DGRAM for UDP
	//initialize address
	struct  sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;                          //use IPv4
	servaddr.sin_port = htons(30188);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);           //0.0.0.0 stands for all IPs in local host,listening all NIC's ports 
    //bind
	if(bind(sock, (struct sockaddr*)& servaddr, sizeof(servaddr)) < 0){
        fprintf(stderr,"Binding failed,check the availability of port!\n");
        exit(1);
    }
    //send initialized occpation and capacity
    ready(sock,hospital);
    //waiting for client
    serve(sock,hospital);

    close(sock);
    
    return 0;
}


void ready(int sock,Hospital& hospital){
    //initialize schedular's address
    struct sockaddr_in scheaddr;
    memset(&scheaddr, 0, sizeof(scheaddr));
    scheaddr.sin_family=AF_INET;
    scheaddr.sin_port=htons(33188);
    scheaddr.sin_addr.s_addr=inet_addr("127.0.0.1");
    socklen_t schelen=sizeof(scheaddr);

    //send occupancy and capability
    char buff[8]={0};
    stringstream ss;
    ss<<hospital.capacity<<" "<<hospital.occupancy<<endl;
    string str=ss.str();
    strcpy(buff,str.c_str());
    sendto(sock, buff, sizeof(buff), 0, (struct sockaddr*)&scheaddr, schelen);

    cout<<"Hospital A is up and running using UDP on port 30188"<<endl;
    cout<<"Hospital A has total capacity "<<hospital.capacity<<" and initial occupancy "<<hospital.occupancy<<endl;
    cout<<"------------------------------------------------------------------------------------------------"<<endl;
}

void serve(int sock,Hospital& hospital){
    char buff[64]={0};
    struct sockaddr_in cliaddr;                                                 //client address
    socklen_t clilen;
    int n;
    while(1){
        clilen=sizeof(cliaddr);
        memset(buff,0,sizeof(buff));
        n=recvfrom(sock,buff,sizeof(buff),0,(struct sockaddr*)&cliaddr,&clilen);
        if(n==-1){
            continue;
        }else if(n>0){
            string rev_str=buff;
            if(rev_str.compare("assigned")==0){                                 //assigned response
                hospital.occupancy+=1;
                double avil=1.0*(hospital.capacity-hospital.occupancy)/hospital.capacity;
                cout<<"Hospital A has been assigned to a client, occupancy updated to "<<hospital.occupancy<<", availability is updated to "<<avil<<endl;
            }else{                                                              //availability response
                int loc=atoi(buff);
                cout<<"Hospital A has received input from client at location "<<loc<<endl;
                double dist=-1;
                double avil=0.0;
                double score=0.0;
                if(loc!=hospital.location){
                    map<int, int>::iterator iter;
                    iter=indice_map.find(loc);
                    if(iter!=indice_map.end()){                                     //loc exits
                        dist=hospital.dist_list[iter->second].val;
                        avil=1.0*(hospital.capacity-hospital.occupancy)/hospital.capacity;
                        if(avil<0){
                            avil=-1;
                        }
                        score=1.0/(dist*(1.1-avil)); 
                        cout<<"Hospital A has capacity = "<<hospital.capacity<<" ,occupancy= "<<hospital.occupancy<<" , availability = "<<avil<<endl;
                        cout<<"Hospital A has found the shortest path to client,distance = "<<dist<<endl;
                        cout<<"Hospital A has the score = "<<score<<endl;
                    }else{
                        cout<<"Hospital A does not have the location "<<loc<<" in map"<<endl;
                        cout<<"Hospital A has sent \" location not found\" to the Scheduler"<<endl;
                    }                   
                }
                memset(buff,0,sizeof(buff));
                if(dist==-1 || avil==-1){
                    strcpy(buff,"-1 -1");
                }else{
                    stringstream ss;
                    ss<<score<<" "<<dist;
                    string str=ss.str();
                    strcpy(buff,str.c_str());
                    cout<<"Hospital A has sent score = "<<score<<" and distance= "<<dist<<" to the Scheduler"<<endl;
                }
                sendto(sock, buff, sizeof(buff), 0, (struct sockaddr*)&cliaddr, clilen);
            }    
        }
        cout<<"------------------------------------------------------------------------------------------------"<<endl;
    }
}

Hospital::Hospital(int capacity,int occupancy,Dist_list *dist_list,int location){
    this->capacity=capacity;
    this->occupancy=occupancy;
    this->dist_list=dist_list;
    this->location=location;
}

Dist_list* init(int loc){                               // input the location #
    double** dist_map=readTxt("map.txt");                  //load the map.txt and create indice mapping and distance graph

    map<int, int>::iterator iter;
    iter=indice_map.find(loc);                          //find the indice mapped to location #
    if(iter!=indice_map.end()){                         //determine if the location exists
        int indice=iter->second;
        int size=indice_map.size();
        Dist_list *dist_list;
        dist_list=new Dist_list[size];                  //to store the shortest path from loc

        for(int i=0;i<size;i++){                         //initialize the dist_list
            if(dist_map[indice][i]==0){
                dist_list[i].val=-1;
            }else{
                dist_list[i].val=dist_map[indice][i];
            }
            dist_list[i].vstd=false;
        }
        dist_list[indice].vstd=true;
        
        while(true){                           
            double min=-1;
            int num=-1;
            for(int i=0;i<size;i++){                    //find the shortest path in each round
                if(!dist_list[i].vstd && dist_list[i].val!=-1){
                    if(min==-1){
                        min=dist_list[i].val;
                        num=i;
                    }else{
                        if(dist_list[i].val<min){
                            min=dist_list[i].val;
                            num=i;
                        }
                    }
                }
            }
            if(num==-1){                                 //done, time to quit the loop
                break;
            }
            dist_list[num].vstd=true;
            for(int i=0;i<size;i++){                     // according to the candidate points, update the dist_list
                if(dist_map[num][i]!=0 && !dist_list[i].vstd){
                    if(dist_list[i].val==-1 || (min+dist_map[num][i])<dist_list[i].val){
                        dist_list[i].val=min+dist_map[num][i];
                    }
                }
            }
        }
        dist_list[indice].val=0;
        return dist_list;
    }else{
        fprintf(stderr,"Location not found !\n");
        exit(1);
    }
}

double** readTxt(string file)
{
    ifstream infile; 
    infile.open(file.data());   //将文件流对象与文件连接起来 
    assert(infile.is_open());   //若失败,则输出错误消息,并终止程序运行 

    string s;
    int count=0;
    map<int, int>::iterator iter;   //store the result of find()
    while(getline(infile,s))
    {
        char str[s.length()+1];
        for(int i=0;i<s.length();i++){
            str[i]=s[i];
        }
        str[s.length()]='\0';
        char *pch;
        pch=strtok(str," ");        //segementation 
        int row=atoi(pch);
        iter=indice_map.find(row);     //get the result mapping from location # to indice
        if(iter==indice_map.end()){     //if no mapping the iter is equal to end()
            indice_map[row]=count++;
        }
        pch=strtok(NULL," ");
        int col=atoi(pch);
        iter=indice_map.find(col);
        if(iter==indice_map.end()){
            indice_map[col]=count++;
        }
    }
    infile.close();             //关闭文件输入流 


    int size=indice_map.size();
    double** dist_map; 
    dist_map=(double **)malloc(sizeof(double)*size*size);     //allocate space
    for(int i=0;i<size;i++){
        dist_map[i]=(double *)malloc(sizeof(double)*size);
    }
    for(int i=0;i<size;i++){               //initialize the dist_map to 0s
        for(int j=0;j<size;j++){
            dist_map[i][j]=0;
        }
        
    }
    infile.open(file.data());   //将文件流对象与文件连接起来 
    assert(infile.is_open());   //若失败,则输出错误消息,并终止程序运行 
    while(getline(infile,s))
    {
        char str[s.length()+1];
        for(int i=0;i<s.length();i++){
            str[i]=s[i];
        }
        str[s.length()]='\0';
        char *pch;
        pch=strtok(str," ");
        iter=indice_map.find(atoi(pch));
        int row=iter->second;       //get the indice from iter by ->second.
        pch=strtok(NULL," ");
        iter=indice_map.find(atoi(pch));
        int col=iter->second;
        pch=strtok(NULL," ");
        double dist=atof(pch);
        dist_map[row][col]=dist;
        dist_map[col][row]=dist;
    }
    infile.close(); 
    return dist_map;
}

