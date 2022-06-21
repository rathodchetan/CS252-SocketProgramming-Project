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
#include <mutex>
//#include <unistd.h>
std:: mutex mtx;
bool client_acquired_lock = false;
using std::cout; using std::cin;
using std::endl; using std::vector;

#define BACKLOG 10
using namespace std;
void client(int numNeig, int* neigPORT,int myuid, string* myFiles,int numMyFiles){
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
        if (idArrayIndex == numNeig)    break;
        {
            /* code */
        }
        for(int i=0;i<numNeig;i++){
            if(!connectedNeig[i]){
                dest_addr.sin_port = htons(neigPORT[i]);
                if(connect(neig_i_fd[i], (struct sockaddr *)&dest_addr, sizeof(struct sockaddr))!=-1){
                    connectedNeig[i] = true;

                    recv(neig_i_fd[i],&msg,sizeof(msg),0);
                    //cout<<"Received message:- "<<msg<<endl;

                    bool gotDollar=false;
                    string s;
                    int j=0;
                    while(true){
                        if(gotDollar){
                            if(msg[j]!='#')
                                s += msg[j];
                            else{
                                neigUniqueIDReceived[idArrayIndex]=stoi(s);
                                s.clear();
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
                    j++;
                    int numNeigFilesReq;
                    while(true){
                    	if(msg[j]!='$'){
                    		s +=msg[j];
                    		j++;
                    	}
                    	else{
                    		numNeigFilesReq=stoi(s);
                    		s.clear();
                    		break;
                    	}
                    }
                   	j++;
                    string namefilesReq[numNeigFilesReq];
                    //cout<<numNeigFilesReq<<"-----------"<<endl;
                    int tt4=0;
                    while(true){
                        if(msg[j]=='&'){
                            namefilesReq[tt4]=s;
                            s.clear();
                            break;
                     	}
                        else if(msg[j]=='$'){
                            namefilesReq[tt4]=s;
                            tt4++;
                            j++;
                            s.clear();
                     	}
                        else{
                            s+=msg[j];
                            j++;
                        }
                    }
                    bool doIHaveAny=false;
                    bool yesIHave[numNeigFilesReq]={false};
                    for (int i = 0; i < numNeigFilesReq; i++)
                    {
                        /* code */
                        for (int j = 0; j < numMyFiles; j++)
                        {
                            /* code */
                            if(myFiles[j] == namefilesReq[i]){
                                yesIHave[i]=true;
                                doIHaveAny=true;
                                break;
                            }
                        }
                        
                    }
                    char msgReply[5000]=" ";
                    string cc=to_string(myuid);
                    int tt=cc.size();
                    int index=0;
                    for(int i=0;i<tt;i++) msgReply[index++]=(char)(cc[i]);
                    msgReply[index++]='$';
                    if(doIHaveAny){
                        //changes
                        
                        //changes done
                        for (int i = 0; i < numNeigFilesReq; i++)
                        {
                            if (yesIHave[i])
                            {
                                for (int j = 0; j < namefilesReq[i].size(); j++)
                                {
                                    msgReply[index++]=(char)namefilesReq[i][j];
                                }
                                msgReply[index++]='$';
                            }

                        }
                        index--;
                        msgReply[index]='#';
                        //cout<<msgReply<<"---------------"<<endl;    
                    }
                    else{
                        msgReply[index++]='*';
                        msgReply[index]='#';
                    }
                    //cout<<msgReply<<"-------"<<endl;
                    send(neig_i_fd[i],&msgReply,sizeof(msgReply),0);
                    //changes
                    // cout<<"beforesend"<<endl;
                    
                    // cout<<"aftersend"<<endl;
                    //changes done
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
    mtx.lock();
    client_acquired_lock = true;
    for(int i=0;i<numNeig;i++){
        cout<<"Connected to "<<neigIDReceived[i]<<" with unique-ID "<<neigUniqueIDReceived[i]<<" on port "<<neigPORT[i]<<endl;
    }
    mtx.unlock();
}

void server(int myid,int myuid,int MYPORT,int numNeig,string* fileName,int numFiles){
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
    map<string,int>file_uid_map;
    for(int i=0;i<numFiles;i++){
        file_uid_map.insert({fileName[i],0});
    }
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
    int ReplyReceived=0;
    while(true){
        if(sockNoArrayIndex == numNeig && ReplyReceived == numNeig) break;
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
                        int index= tt1 + tt + 2;

                        string cc2=to_string(numFiles);
                        int tt2=cc2.size();
                        for(int i=0;i<tt2;i++) msg1[index++]=(char)(cc2[i]);
                        msg1[index++]='$';
                        for(int i = 0;i<numFiles;i++){
                        	int tt3 = fileName[i].size();
                        	for(int j=0;j<tt3;j++){
                        		msg1[index++] = (char)(fileName[i][j]);
                        	}
                        	if(i != numFiles - 1)
                        		msg1[index++] = '$';
                        	else
                        		msg1[index++] = '&';
                        }
                        //cout<<"Sending mesage " << msg1 << endl;
                        send(newfd,msg1,sizeof(msg1),0);
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) { // keep track of the maximum
                            fdmax = newfd;
                        }
                        sockNoArrayIndex++;
                    }
                }
                else {
                    char msg1[5000]=" ";
                    int nbytes;
                    if ((nbytes = recv(i, &msg1, sizeof(msg1), 0)) <= 0) {
                        if (nbytes == 0) {
                        //printf("selectserver: socket %d hung up\n", i);
                        }
                        else {
                            perror("recv");
                        }
                        //close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    }
                    else {
                        //cout<<"got msg from client"<<msg1<<endl;
                        ReplyReceived++;
                        string s;
                        int i=0;
                        int j=0;
                        while(true){
                            if(msg1[i]=='$'){s=s+' '; i++;j++;}
                            else {s=s+msg1[i];i++;}
                            if(msg1[i]=='#')break;
                        }
                        string s1[j+1];
                        int l=0;
                        for(int k=0;k<j+1;k++){
                            while(true){
                                if(s[l]=='\0')break;
                                if(s[l]==' '){l++;break;}
                                s1[k] = s1[k]+s[l];
                                l++;
                            }
                        }
                        int uid = stoi(s1[0]);
                        // cout<<"got following files from "<<s1[0]<<": ";
                        if(s1[1] != "*"){
                            for(int k=1;k<j+1;k++){
                                if(file_uid_map[s1[k]] != 0){
                                    file_uid_map[s1[k]] = min(uid,file_uid_map[s1[k]]);
                                }
                                else{
                                    file_uid_map[s1[k]] = uid;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    while(true){
        if(client_acquired_lock)break;
    }
    mtx.lock();
    for (auto itr = file_uid_map.begin(); itr != file_uid_map.end(); ++itr) {
        cout<<"Found "<<itr->first<<" at "<<itr->second<<" with MD5 0 at depth ";
        if(itr->second == 0)cout<<"0";
        else cout<<"1";
        cout<<endl;
    }
    mtx.unlock();
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
    sort(fileNAME,fileNAME+numFILES);
    if(numNeig==0){
    	for(auto file: fileNAME)cout<<"Found "<<file<<" at "<<"0"<<" with MD5 0 at depth 0"<<endl;
    	return 0;
    }
    string myFiles[files.size()];
   	for(int i=0;i<files.size();i++){
   		myFiles[i]=files[i];
   		//cout<<myFiles[i]<<endl;
   	}

    int* a;
    a = neigPORT;
	string* b;
	b = fileNAME;
	string* c;
	c = myFiles;
    thread t1(server, myid,myuid,MYPORT,numNeig,b,numFILES);
    thread t2(client, numNeig,a,myuid,c,files.size());
    t1.join();
    t2.join();
}
