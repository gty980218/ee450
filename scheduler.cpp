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
#include "scheduler.h"
#include <map>
#include <sstream>
using namespace std;
Scheduler scheduler;
int main(){
    //initailize UDP sock
    int sock_udp;
    if((sock_udp=socket(PF_INET,SOCK_DGRAM,0))<0){
        fprintf(stderr,"Create UDP socket failed!\n");
        exit(1);
    }
    struct sockaddr_in scheaddr;
    memset(&scheaddr, 0, sizeof(scheaddr));
	scheaddr.sin_family = AF_INET;                          //use IPv4
	scheaddr.sin_port = htons(33188);
	scheaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(sock_udp, (struct sockaddr*)& scheaddr, sizeof(scheaddr)) < 0){
        fprintf(stderr,"Binding UDP failed,check the availability of port!\n");
        exit(1);
    }
    cout<<"The Scheduler is up and running."<<endl;
    //initialize sheduler
    init_sche(sock_udp);
    //initialize TCP sock
    int sock_tcp;
    if((sock_tcp=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP))<0){
        fprintf(stderr,"Create TCP socket failed!\n");
        exit(1);
    }
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(34188); 
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(sock_tcp, (struct sockaddr*)& servaddr, sizeof(servaddr)) < 0){
        fprintf(stderr,"Binding TCP failed,check the availability of port!\n");
        exit(1);
    }
    if(listen(sock_tcp,SOMAXCONN)<0){
        fprintf(stderr,"Listen failed!");
        exit(1);
    }
    //connect tcp
    tcp_conn(sock_tcp,sock_udp);

    close(sock_tcp);
    close(sock_udp); 
}

void tcp_conn(int sock_tcp,int sock_udp){
    struct sockaddr_in peeraddr;
    socklen_t peerlen=sizeof(peeraddr);
    int conn;
    pid_t pid;

    while(1){
        if((conn=accept(sock_tcp,(struct sockaddr*)&peeraddr,&peerlen))<0){
            fprintf(stderr,"Connetc failed!");
            exit(1);
        }
        reinit();
        pid=fork();                                                     //fork() will copy everything from its parents' process
        if(pid==-1){
            fprintf(stderr,"Child process error!");
        }else if(pid==0){
            close(sock_tcp);
            serve(conn,sock_udp);
            exit(1);
        }else{
            close(conn);
        }
    }
}
//used for reload the occupancy that have changed
void reinit(){
    ifstream infile; 
    infile.open("info.txt");  //"info.txt" used for storing changed occupancy info
    assert(infile.is_open());

    string str;
    int i=0;
    while(getline(infile,str)){
        int ocpt=atoi(str.c_str());
        scheduler.hspt_info[i].ocpt=ocpt;
        i++;
    }
}
// get client requests and forward requests
void serve(int conn,int sock_udp){
    char buf_tcp[64]={0};
    char buf_udp[64]={0};
    memset( buf_tcp,0,sizeof( buf_tcp));
    int ret=read(conn, buf_tcp,sizeof( buf_tcp));
    if(ret==-1){
        fprintf(stderr,"Runtime read error");
        exit(1);
    }
    cout<<"The Scheduler has received client at location "<<buf_tcp<<" from the client using TCP over port 34188"<<endl;
    for(int i=0;i<3;i++){
        if(scheduler.hspt_info[i].cpblt>scheduler.hspt_info[i].ocpt){    //cability>occupancy do while
            memset( buf_udp,0,sizeof( buf_udp));
            sendto(sock_udp,  buf_tcp, strlen( buf_tcp), 0, (struct sockaddr*)&scheduler.hspt_info[i].addr, sizeof(scheduler.hspt_info[i].addr));//send location to hospital
            recvfrom(sock_udp, buf_udp, sizeof(buf_udp), 0, NULL, NULL);//get score and dist from hospital
            char* pch;
            pch=strtok(buf_udp," ");
            double score=atof(pch);
            pch=strtok(NULL," ");
            double dist=atof(pch);
            scheduler.add_info(i,score,dist); 
            if(i==0){
                cout<<"The Scheduler has sent client location to Hospital A using UDP over port 33188."<<endl;
                if(dist!=-1){
                    cout<<"The Scheduler has received map information from Hospital A, the score = "<<score<<" and the distance = "<<dist<<endl;
                }else{
                    cout<<"The Scheduler has received map information from Hospital A, the score = none and the distance = none"<<endl;
                }
            }else if(i==1){
                cout<<"The Scheduler has sent client location to Hospital B using UDP over port 33188."<<endl;
                if(dist!=-1){
                    cout<<"The Scheduler has received map information from Hospital B, the score = "<<score<<" and the distance = "<<dist<<endl;
                }else{
                    cout<<"The Scheduler has received map information from Hospital B, the score = none and the distance = none"<<endl;
                }
            }else{
                cout<<"The Scheduler has sent client location to Hospital C using UDP over port 33188."<<endl;
                if(dist!=-1){
                    cout<<"The Scheduler has received map information from Hospital C, the score = "<<score<<" and the distance = "<<dist<<endl;
                }else{
                    cout<<"The Scheduler has received map information from Hospital C, the score = none and the distance = none"<<endl;
                }
            }

        }
        
    }
    int assigned=-1;//-1:not find;-2:is hospital;
    if(scheduler.hspt_info[0].dist!=-1 && scheduler.hspt_info[1].dist!=-1 && scheduler.hspt_info[2].dist!=-1 ){     //whether client is in map and is not hospital
        if(scheduler.hspt_info[0].score!=-1 || scheduler.hspt_info[1].score!=-1 || scheduler.hspt_info[2].score!=-1){
            double max_sc=0;//find the max score and min distance
            double min_dist;
            assigned=0;
            for(int i=0;i<3;i++){
                if(scheduler.hspt_info[i].score!=-1){
                    if(max_sc<scheduler.hspt_info[i].score){
                        max_sc=scheduler.hspt_info[i].score;
                        min_dist=scheduler.hspt_info[i].dist;
                        assigned=i;// decide which hospital will be assigned
                    }else if(max_sc==scheduler.hspt_info[i].score){
                        if(min_dist<scheduler.hspt_info[i].dist){
                            min_dist=scheduler.hspt_info[i].dist;
                            assigned=i;
                        }
                    }
                }
            }
        }
    }else if(scheduler.hspt_info[0].dist!=-1 || scheduler.hspt_info[1].dist!=-1 || scheduler.hspt_info[2].dist!=-1){//is hospital
        assigned=-2;
    }
    if(assigned>=0){
        scheduler.hspt_info[assigned].ocpt+=1;
        ofstream out_file("info.txt");              //write the changed occupancy to info.txt
        for(int i=0;i<3;i++){
            out_file<<scheduler.hspt_info[i].ocpt<<endl;
        }
        out_file.close();

        memset( buf_udp,0,sizeof( buf_udp));
        memset( buf_tcp,0,sizeof( buf_tcp));
        strcpy(buf_udp,"assigned");
        sendto(sock_udp,  buf_udp,sizeof(buf_udp), 0, (struct sockaddr*)&scheduler.hspt_info[assigned].addr, sizeof(scheduler.hspt_info[assigned].addr));
        string str_asn;
        if(assigned==0){
            str_asn="A";
            cout<<"The Scheduler has assigned Hospital A to the client"<<endl;
            cout<<"The Scheduler has sent the result to Hospital A using UDP over port 33188"<<endl;

        }else if(assigned==1){
            str_asn="B";
            cout<<"The Scheduler has assigned Hospital B to the client"<<endl;
            cout<<"The Scheduler has sent the result to Hospital B using UDP over port 33188"<<endl;

        }else{
            str_asn="C";
            cout<<"The Scheduler has assigned Hospital C to the client"<<endl;
            cout<<"The Scheduler has sent the result to Hospital C using UDP over port 33188"<<endl;

        }
        strcpy(buf_tcp,str_asn.c_str());
        write(conn,buf_tcp,sizeof(buf_tcp));
    }else if(assigned==-1){// not find
        string str_err="NF";
        if(scheduler.hspt_info[0].cpblt<=scheduler.hspt_info[0].ocpt && scheduler.hspt_info[1].cpblt<=scheduler.hspt_info[1].ocpt && scheduler.hspt_info[2].cpblt<=scheduler.hspt_info[2].ocpt){
            str_err="OL";  //overload
        }
        strcpy(buf_tcp,str_err.c_str());
        write(conn,buf_tcp,sizeof(buf_tcp));
    }else{
        string str_err="N";// client is in hospital illegal input
        strcpy(buf_tcp,str_err.c_str());
        write(conn,buf_tcp,sizeof(buf_tcp));
    }
    cout<<"The Scheduler has sent the result to client using TCP over port 34188 "<<endl;
    cout<<"------------------------------------------------------------------------------------------------"<<endl;
    
    
}
void init_sche(int sock){
    ofstream out_file("info.txt",ios::out);                         //when start scheduler,clean the info.txt
    out_file.close();

    char buff[64]={0};
    struct sockaddr_in hosaddr;
    socklen_t hoslen;
    int n;
    int init_times=0;
    while(1){                                                       //waiting for hospital A/B/C to send their capability and occupancy
        hoslen=sizeof(hosaddr);
        memset(buff,0,sizeof(buff));
        n=recvfrom(sock,buff,sizeof(buff),0,(struct sockaddr*)&hosaddr,&hoslen); //get initialized info from hospitals and get their IP and port #
        if(n==-1){
           continue;
        }else if(n>0){
            char *pch;
            pch=strtok(buff," ");
            int cpblt=atoi(pch);
            pch=strtok(NULL," ");
            int ocpt=atoi(pch);
            int port;
            if(ntohs(hosaddr.sin_port)==30188){
                port=0;
                cout<<"The Scheduler has received information from Hospital A:total capacity is "<<cpblt<<" and initial occupancy is "<<ocpt<<endl;
            }else if(ntohs(hosaddr.sin_port)==31188){
                port=1;
                cout<<"The Scheduler has received information from Hospital B:total capacity is "<<cpblt<<" and initial occupancy is "<<ocpt<<endl;
            }else{
                port=2;
                cout<<"The Scheduler has received information from Hospital C:total capacity is "<<cpblt<<" and initial occupancy is "<<ocpt<<endl;
            }
            cout<<"------------------------------------------------------------------------------------------------"<<endl;
            scheduler.init_info(port,cpblt,ocpt,hosaddr);
            init_times+=1;
            if(init_times==3){
                break;
            }
        }
    }

}