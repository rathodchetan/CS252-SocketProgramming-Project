#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <dirent.h>
#include <algorithm>
#include <bits/stdc++.h>
#include <thread>
#include <arpa/inet.h>
//#include <unistd.h>


using std::cout; using std::cin;
using std::endl; using std::vector;

#define BACKLOG 10
using namespace std;
void client(int numNeig, int* neigPORT){
    // for(int i=0;i<1000;i++){
    //     cout<<"-";
    // }
    int neig_i_fd[numNeig];
	struct sockaddr_in dest_addr; 
	socklen_t sin_size;
   	bool connectedNeig[numNeig];

    for(int i=0;i<numNeig;i++){
    	connectedNeig[i] = false;
    }

    for(int i=0;i<numNeig;i++){
		if ((neig_i_fd[i] = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			perror("socket");
			exit(1);
		}
	}
	
   	dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(dest_addr.sin_zero), '\0', 8);

    int idArrayIndex=0;
    char msg[5000];
    int neigUniqueIDReceived[numNeig]={0};
    int neigIDReceived[numNeig]={0};

    while(true){
        for(int i=0;i<numNeig;i++){
            if(!connectedNeig[i]){
                dest_addr.sin_port = htons(neigPORT[i]);
                if(connect(neig_i_fd[i], (struct sockaddr *)&dest_addr, sizeof(struct sockaddr))!=-1){
                    connectedNeig[i] = true;
                    
                    recv(neig_i_fd[i],&msg,sizeof(msg),0);
                    bool gotDollar=false;
                    string s;
                    int j=0;
                    while(true){
                        if(gotDollar){
                            if(msg[j]!='#')
                                s += msg[j];
                            else{
                                neigUniqueIDReceived[idArrayIndex]=stoi(s);
                                break;
                            }
                            j++;
                        }
                        else{
                            if(msg[j]!='$')
                                s+=msg[j];
                            else{
                                gotDollar = true;
                                neigIDReceived[idArrayIndex]=stoi(s);
                                s.clear();
                            }			
                            j++;
                        }
                    }
                    idArrayIndex++;
                } 	
            }
        }
        if(idArrayIndex==numNeig)break;
    }
    for (int i = 0; i < numNeig-1; i++){    
        for (int j = 0; j < numNeig-i-1; j++){
            if (neigIDReceived[j] > neigIDReceived[j+1]){
                int temp = neigIDReceived[j];
                neigIDReceived[j]=neigIDReceived[j+1];
                neigIDReceived[j+1]=temp;
                temp = neigUniqueIDReceived[j];
                neigUniqueIDReceived[j] = neigUniqueIDReceived[j+1];
                neigUniqueIDReceived[j+1] = temp;
            }
        }
    }
    
    for(int i=0;i<numNeig;i++){
        cout<<"Connected to "<<neigIDReceived[i]<<" with unique-ID "<<neigUniqueIDReceived[i]<<" on port "<<neigPORT[i]<<endl;
    }
}

void server(int myid,int myuid,int MYPORT,int numNeig){
    // for(int i=0;i<1000;i++){
    //     cout<<"*";
    // }
    int listener;
    int newfd;
    int fdmax;
    int addrlen;
    int neigSocketNumber[numNeig]={-1};
    struct sockaddr_in my_addr; 
    struct sockaddr_in remoteaddr;
    socklen_t sin_size;
    fd_set master; 
 	fd_set read_fds;
    int yes=1;
   	
   	FD_ZERO(&master); 
	FD_ZERO(&read_fds);
   	
	if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }
    
    //Assigning address and Port number of the clientX to myaddr
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(MYPORT);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(my_addr.sin_zero), '\0', 8);

    if((bind(listener, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)))<0){
        cout<<"Binding Failed!:(";
    };
    
    if(listen(listener, numNeig+2)<0){
       cout<<"listen Failed!:(";   		
    };
    
    FD_SET(listener, &master);
    fdmax = listener;
    int sockNoArrayIndex=0;
    while(true){
        if(sockNoArrayIndex==numNeig)break;
        read_fds = master;
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(1);
        }
        for(int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == listener) {
                    addrlen = sizeof(remoteaddr);
                    if ((newfd = accept(listener, (struct sockaddr *)&remoteaddr, (socklen_t *)&addrlen)) == -1) {
                        perror("accept");
                    } 
                    else{
                        neigSocketNumber[sockNoArrayIndex]=newfd;
                        char msg1[5000]=" ";		
                        string cc=to_string(myid);
                        int tt=cc.size();
                        for(int i=0;i<tt;i++) msg1[i]=(char)(cc[i]);
                        msg1[tt]='$';
                        string cc1=to_string(myuid);
                        int tt1=cc1.size();
                        for(int i=0;i<tt1;i++) msg1[i+tt+1]=(char)(cc1[i]);
                        msg1[tt1+tt+1]='#';
                        send(newfd,msg1,sizeof(msg1),0);
                        sockNoArrayIndex++;
                    }
                } 
                else {
                    
                }
            }
        }
    }
}

int main(int argc, char** argv){
	// FileReading
	string FILENAME = argv[1];
    string MYFILEPATH = argv[2];	
    ifstream fin(FILENAME);
    int myid,myuid,MYPORT;
    int numNeig;
    
    fin>>myid>>MYPORT>>myuid>>numNeig;
    int neigID[numNeig],neigPORT[numNeig]; //change here 

    for(int i=0;i<numNeig;i++){
    	fin>>neigID[i]>>neigPORT[i];
    }
    
    int numFILES;
    fin>>numFILES;
    string fileNAME[numFILES];

    for(int i=0;i<numFILES;i++){
    	fin>>fileNAME[i];
    }

    //Getting File names of the client present in its directory
      
    DIR *dir; struct dirent *diread;
    vector<string> files;
	
    if ((dir = opendir(argv[2])) != nullptr) {
        while ((diread = readdir(dir)) != nullptr) {
            files.push_back(diread->d_name);
        }
        closedir (dir);
        
		sort(files.begin(),files.end());
		files.erase(files.begin());
		files.erase(files.begin());



    }else {
        perror ("opendir");
        return EXIT_FAILURE;
    }
    for (auto file : files) cout << file << "\n";
    if(numNeig==0)return 0;
    int* a;
    a = neigPORT;
    thread t1(server, myid,myuid,MYPORT,numNeig);
    thread t2(client, numNeig,a);
    t1.join();
    t2.join();
}
