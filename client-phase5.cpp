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
#include <sys/stat.h>
#include <sys/types.h>
#include <openssl/md5.h> 
#include <filesystem>
std:: mutex mtx;
bool client_acquired_lock = false;
int numberOfFilesFrom2Hop=0;
bool ReadyToSendData=false;

using std::cout; using std::cin;
using std::endl; using std::vector;
#define BACKLOG 10
#define BUFFSIZE 16384 
using namespace std;
map<string,int> file_uid_map;
map<string,int> file_hop_map;
string s5;

std::string get_md5hash( const std::string& fname) 
{ 
 
  char buffer[BUFFSIZE]; 
  unsigned char digest[MD5_DIGEST_LENGTH]; 
 
  std::stringstream ss; 
  std::string md5string; 
   
  std::ifstream ifs(fname, std::ifstream::binary); 
 
  MD5_CTX md5Context; 
 
  MD5_Init(&md5Context); 
 
 
   while (ifs.good())  
    { 
 
       ifs.read(buffer, BUFFSIZE); 
 
       MD5_Update(&md5Context, buffer, ifs.gcount());  
    } 
 
   ifs.close(); 
 
   int res = MD5_Final(digest, &md5Context); 
    
    if( res == 0 ) // hash failed 
      return {};   // or raise an exception 
 
  // set up stringstream format 
  ss << std::hex << std::uppercase << std::setfill('0'); 
 
 
 for(unsigned char uc: digest) 
    ss << std::setw(2) << (int)uc; 
 
 
  md5string = ss.str(); 
  
 return md5string; 
}

int isSubstring(string s1, string s2)
{
    int M = s1.length();
    int N = s2.length();
 
    /* A loop to slide pat[] one by one */
    for (int i = 0; i <= N - M; i++) {
        int j;
 
        /* For current index i, check for
 pattern match */
        for (j = 0; j < M; j++)
            if (s2[i + j] != s1[j])
                break;
 
        if (j == M)
            return i;
    }
 
    return -1;
}

void send_file(int sock, string filename){

    // cout << filename << endl;
    int n = 0;
    int file_size = 0;
    FILE *my_file;
    char buf[50];

    // cout << "Getting image size" << endl;
    my_file = fopen(filename.c_str(), "r"); 
    fseek(my_file, 0, SEEK_END);
    file_size = ftell(my_file);
    // cout << file_size << endl; // Output 880

    // cout << "Sending picture size to the server" << endl;
    sprintf(buf, "%d", file_size);
    if((n = send(sock, buf, sizeof(buf), 0)) < 0)
    {
        perror("send_size()");
        exit(errno);
    }

    char Sbuf[file_size];
    // cout << "Sending the picture as byte array" << endl;
    fseek(my_file, 0, SEEK_END);
    file_size = ftell(my_file);
    fseek(my_file, 0, SEEK_SET); 

    while(!feof(my_file)){
    n = fread(Sbuf, sizeof(char), file_size, my_file);
    if (n > 0) { 
        if((n = send(sock, Sbuf, file_size, 0)) < 0)
        {
            perror("send_data()");
            exit(errno);
        }
    }
    
    }
}

void recv_file(int sock,string filename){

    // cout << filename << endl;

    int n = 0;
    // cout << "Reading image size" << endl;
    char buf[50];
    int file_size = 0;
    if ((n = recv(sock, buf, sizeof(buf), 0) <0)){
        perror("recv_size()");
        exit(errno);
    }
    file_size = atoi(buf);
    // cout << file_size << endl; // 880 output
    char Rbuffer[file_size];
    // cout << "Reading image byte array" << endl;
    n = 0;
    
    FILE *file;
    file = fopen(filename.c_str(), "w");
    while(file_size!=0){
        if ((n = recv(sock, Rbuffer, file_size, 0)) < 0){
            perror("recv_size()");
            exit(errno);
        }
        fwrite(Rbuffer, sizeof(char), n, file);
        file_size-=n;
        // cout<<"file size to be read"<<file_size<<endl;
    }


    // cout << "Converting byte array to image" << endl;
    fclose(file);
}

void client(int numNeig,int* neigID, int* neigPORT,int myuid, string* myFiles,int numMyFiles,string myFilePath,int *portArray2HopFiles,string *name2HopFiles,string MYFILEPATH){
    int neig_i_fd[numNeig];
    struct sockaddr_in dest_addr;
    socklen_t sin_size;
    bool connectedNeig[numNeig];
    map<int,int>uid_neigfd_map;
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

    fd_set master1;
    fd_set read_fds1;
    int fdmax1=-1;
    FD_ZERO(&master1);
    FD_ZERO(&read_fds1);

//---------------------------------------------------------------------

    while(true){
        if (idArrayIndex == numNeig){
            char msg2[5000]=" ";
            int numfilesend=0;
            int totalFilesToSend=0;
            int totalNumbermsgReceived=0;
            while(true){
                read_fds1 = master1;
                //sleep(1);
                if(totalNumbermsgReceived==numNeig && totalFilesToSend == numfilesend)  break;
                if (select(fdmax1+1, &read_fds1, NULL, NULL, NULL) == -1) {
                    perror("select");
                    exit(1);
                }
                // mtx.lock();
                for(int i = 0; i <= fdmax1; i++) {
                    int n;
                    char s[5000] = "file"; //new
                    if(FD_ISSET(i,&read_fds1)){
                        char msg3[5000]=" ";
                        if((n=recv(i,&msg3,sizeof(msg3),0))<=0){}
                        else{
                            mtx.lock();
                            // cout<<n<<" "<<i<<" "<<msg2<<" recieved in if statement "<<" ---------line 67-------"<<endl;
                            
                            mtx.unlock();
                            //cout<<msg3<<"---------202----------"<<endl;
                            if(!(msg3[0]=='%' && msg3[1]=='N')){ //new
                                numfilesend++;
                                string s="";
                                // s = mydir;
                                int j=5;
                                while(msg3[j] != '\0'){
                                    s = s+msg3[j];
                                    j++;
                                }
                                //cout<<s<<"-----------line 160-------------";
                                s=myFilePath+s;
                                //sleep(1);
                                send_file(i,s);
                                // send(i,&s,sizeof(s),0); //new
                            } //new
                            else{
                                string s = msg3;
                                int num=0;
                                s.erase(0,2);
                                //cout<<s<<"-----221------"<<endl;
                                totalFilesToSend+=stoi(s);
                                totalNumbermsgReceived++;
                            }
                        }
                    }
                }
                // mtx.unlock();
            }
            mtx.lock();
            for(int i=0;i<numNeig;i++){
                // cout<<"neig fd "<<neig_i_fd[i]<<" ---------line 73-------"<<endl;
            }
            mtx.unlock();
            for(auto element1: uid_neigfd_map){ 
                char s[5000] = "done";
                mtx.lock();
                send(uid_neigfd_map[element1.first],&s,sizeof(s),0);
                mtx.unlock();
            }
            break;
        }
        else{
            for(int i=0;i<numNeig;i++){
                if(!connectedNeig[i]){
                    dest_addr.sin_port = htons(neigPORT[i]);
                    if(connect(neig_i_fd[i], (struct sockaddr *)&dest_addr, sizeof(struct sockaddr))!=-1){
                        connectedNeig[i] = true;

                        recv(neig_i_fd[i],&msg,sizeof(msg),0);

                        FD_SET(neig_i_fd[i],&master1);
                        if(neig_i_fd[i]>fdmax1){
                            fdmax1=neig_i_fd[i];
                        }

                        bool gotDollar=false;
                        string s;
                        int j=0;
                        while(true){
                            if(gotDollar){
                                if(msg[j]!='#')
                                    s += msg[j];
                                else{
                                    neigUniqueIDReceived[idArrayIndex]=stoi(s);
                                    uid_neigfd_map.insert({neigUniqueIDReceived[idArrayIndex],neig_i_fd[i]});
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
                
                        }
                        else{
                            msgReply[index++]='*';
                            msgReply[index]='#';
                        }
                
                        send(neig_i_fd[i],&msgReply,sizeof(msgReply),0);
                        //changes
        
                        //changes done
                        idArrayIndex++;
                    }
                }
            }
        } 
    }
    map<int,int> uid_id_map;
    for(int i=0;i<numNeig;i++){
        uid_id_map.insert({neigUniqueIDReceived[i],neigIDReceived[i]});
    }
    map<int,int> id_port_map;
    for(int i=0;i<numNeig;i++){
        id_port_map.insert({neigID[i],neigPORT[i]});
    }

    int didIrecv=0;
    map<int,string> uid_myDownFiles_map;
    bool allDataReceived=false;
     while(true){
        if(didIrecv==numNeig){
            // for (auto element: uid_myDownFiles_map)
            // {
            //     cout<<element.first<<" "<<element.second<<endl;
            // }
            for(auto element:uid_myDownFiles_map){
                int recUid = element.first;
                string t = element.second;
                string dataToSend = to_string(recUid);
                int index=0;
                
                    string s;
                    for (int i = 0; i < t.size(); ++i)
                    {
                        if(i==0 && t[i]=='$'){
                            continue;
                        }
                        else if(i==0 && (t[i]=='&'||t[i]=='#'))  break;
                        else if(t[i]=='&'){
                            //cout<<s<<endl;
                            int minUid=2147483647;
                            for(auto element1: uid_myDownFiles_map){
                                string str=element1.second;
                                string substr1 ='&' + s;
                                if(isSubstring(substr1,str)!=-1){
                                    minUid=min(minUid,element1.first);
                                }
                            }
                            if(minUid==2147483647)  minUid=0;
                            dataToSend=dataToSend+'$'+to_string(minUid)+'#' + s + '&'+to_string(id_port_map[uid_id_map[minUid]]);
                            s.clear();
                            break;

                        }
                        else if(t[i]=='$'){
                            int minUid=2147483647;
                            for(auto element1: uid_myDownFiles_map){
                                string str=element1.second;
                                string substr1 ='&' + s;
                                if(isSubstring(substr1,str)!=-1){
                                    minUid=min(minUid,element1.first);
                                }
                            }
                            if(minUid==2147483647)  minUid=0;
                            dataToSend=dataToSend+'$'+to_string(minUid)+'#' + s + '&' + to_string(id_port_map[uid_id_map[minUid]]) ;
                            //cout<<s<<endl;
                            s.clear();
                            //continue;
                        }
                        else{
                            s+=t[i];
                        }
                  
                }
                dataToSend+='%';
                char msg2[5000]=" ";

                for (int k = 0; k < dataToSend.size(); ++k)
                {
                    msg2[k]=dataToSend[k];
                }
                //cout<<"Printing message:- "<<msg2<<endl;
                send(uid_neigfd_map[recUid],&msg2,sizeof(msg2),0);
                // cout<<"?????????????????"<<dataToSend<<endl;

            }
            break;
        };
        // if(didIrecv==numNeig)   break;
         //recv usign select
         read_fds1 = master1;
         //cout<<"--------------------395--------------";
         if (select(fdmax1+1, &read_fds1, NULL, NULL, NULL) == -1) {
             perror("select");
             exit(1);
         }
         //cout<<"-----------400----------";
         // mtx.lock();
         for(int i = 0; i <= fdmax1; i++) {
             if(FD_ISSET(i,&read_fds1)){
                 char msg2[5000]=" ";
                 int n;
                 if((n=recv(i,&msg2,sizeof(msg2),0))<=0){}
                 else{
                     if(msg2[0]=='D'&&msg2[1]=='2'){
                         //mtx.lock();
                         //cout<<"Dept 2 message received "<<msg2<<endl;
                         //mtx.unlock();
                         didIrecv++;
                         int uidRec=0;
                         string s;
                         int index=2;
                         //string s1=msg2;
                         //char msg3[256]=" ";
                         // while(true){
                         //     if(msg2[index]!='#'){
                         //         msg3[index]=msg2[index];
                         //         index++;
                         //     }
                         //     else{
                         //         msg3[index]=msg2[index];
                        
                         //         break;
                         //     }
                            


                         // }
                         //index=2;
                         while(true){
                             //cout<<":-"<<msg2[index]<<"---"<<endl;
                             if(msg2[index] =='$' || msg2[index]=='#'){
                                 uidRec = stoi(s);
                                 s.clear();
                                 break;}
                             else{
                                 s+=msg2[index];
                                 index++;
                             }
                         }
                         s.clear();
                         s=msg2;
                         while(true){
                             if(s[0]=='$'||s[0]=='&' || s[0]=='#'){
                                 break;        
                             }
                             else{
                                 s.erase(0,1);
                             }
                         }
                         //cout<<s<<endl;
                         uid_myDownFiles_map.insert({uidRec,s});
                         

                         //cout<<uidRec<<"------------"<<endl;
                         //msg2[1]='1';
                
                         //int tt=s1.size();

                         // for (int i = 0; i < tt; ++i)
                         // {
                         //     msg3[i]=(char)s1[i];
                         // }

                         //cout<<"Sending message to 2 hop away neig!!!!!!!!!!!! "<<msg2<<endl;
                         // for(auto element: uid_neigfd_map){
                         //     if(element.first != uidRec){
                         //         //cout<<"Will send to "<<element.first<<endl;
                                
                         //         //send(element.second,msg2,sizeof(msg2),0);
                         //         //send(element.second,msg3,sizeof(msg3),0);
                         //         //recv(element.second,);
                         //     } 
                                
                         // }
                            
                     }


                 }
    
             }
         }
         if(allDataReceived){
            // for(auto element:uid_myDownFiles_map){
            //     int recUid = element.first;
            //     string t = element.second;
            //     string dataToSend = to_string(recUid);
            //     int index=0;
            //     while(true){
            //         string s;
            //         for (int i = 0; i < t.size(); ++i)
            //         {
            //             if(i==0 && t[i]=='$'){
            //                 continue;
            //             }
            //             else if(i==0 && (t[i]=='&'||t[i]=='#'))  break;
            //             else if(t[i]=='&'){
            //                 //cout<<s<<endl;
            //                 int minUid=2147483647;
            //                 for(auto element1: uid_myDownFiles_map){
            //                     string str=element1.second;
            //                     string substr1 ='&' + s;
            //                     if(isSubstring(substr1,str)!=-1){
            //                         minUid=min(minUid,element1.first);
            //                     }
            //                 }
            //                 if(minUid==2147483647)  minUid=0;
            //                 dataToSend=dataToSend+'$'+to_string(minUid)+'#' + s;
            //                 s.clear();
            //                 break;

            //             }
            //             else if(t[i]=='$'){
            //                 int minUid=2147483647;
            //                 for(auto element1: uid_myDownFiles_map){
            //                     string str=element1.second;
            //                     string substr1 ='&' + s;
            //                     if(isSubstring(substr1,str)!=-1){
            //                         minUid=min(minUid,element1.first);
            //                     }
            //                 }
            //                 if(minUid==2147483647)  minUid=0;
            //                 dataToSend=dataToSend+'$'+to_string(minUid)+'#' + s;
            //                 //cout<<s<<endl;
            //                 s.clear();
            //                 //continue;
            //             }
            //             else{
            //                 s+=t[i];
            //             }
            //         }
            //     }
            //     cout<<dataToSend<<endl;

            // }
            // break;
         }
         //when msg received then send to neigbours other than receved neighbour
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
    int socket2Hop;
    bool DataHop2Send[numNeig]={false};
    int filesReceived=0;
    while(true){
        //cout<<numberOfFilesFrom2Hop<<"--------663-------"<<endl;
        if(ReadyToSendData && filesReceived==numberOfFilesFrom2Hop)    break;
        if(ReadyToSendData && numberOfFilesFrom2Hop > 0){
            // for (int i = 0; i < numberOfFilesFrom2Hop; ++i)
            // {
            //     cout<<portArray2HopFiles[i]<<" "<<name2HopFiles[i]<<" "<<endl;
            // }
            //cout<<numberOfFilesFrom2Hop;
            //break;
            //cout<<numberOfFilesFrom2Hop<<"???????????"<<endl;
            for(int i=0;i<numberOfFilesFrom2Hop;i++){
                //cout<<name2HopFiles[i]<<" "<<i<<"!!!!!!!!!!"<<endl;
            }
            for (int i = 0; i < numberOfFilesFrom2Hop; ++i)
            {   
                if(!DataHop2Send[i]){


                    if(socket2Hop=socket(AF_INET, SOCK_STREAM, 0)==-1){
                        cout<<"Socket cant Be created"<<endl;
                    }
                    dest_addr.sin_port = htons(portArray2HopFiles[i]);
                    if(connect(socket2Hop, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr))!=-1){
                        char s[5000]=" ";
                        for (int j = 0; j < name2HopFiles[i].size(); ++j)
                        {
                            s[j]=(char)name2HopFiles[i][j];
                        }
                        //name2HopFiles;
                        //cout<<"Sending "<< s <<"-------687----------"<<endl;
                        send(socket2Hop,s,sizeof(s),0);
                        DataHop2Send[i]=true;
                        string s1 = myFilePath+"Downloaded/"+name2HopFiles[i];
                        //cout<<s1<<endl;
                        //cout<<"Receiving "<<s1<<"---------692----------"<<endl;

                        recv_file(socket2Hop,s1);
                        //sleep(1);
                        filesReceived++;
                        file_hop_map[name2HopFiles[i]]=2;
                        //cout<<"Received File"<<s1<<"---------697---------"<<endl;
                    }
                    close(socket2Hop);
                }    
            }
        }
    }
sleep(1);
    mtx.lock();
    int index=0;
    for (auto itr = file_hop_map.begin(); itr != file_hop_map.end(); ++itr) {
      if(itr->second==0){
          cout<<"Found "<<itr->first<<" at "<<file_uid_map[itr->first]<<" with MD5 0 at depth 0";
          
      }  
      else{
            string s2=itr->first;
            s2.erase(remove(s2.begin(), s2.end(), ' '), s2.end());
            string s1 ;
            char tt[5000]=" ";
            for (int i = 0; i < s2.size(); i++)
            {
                tt[i]=(char)s2[i];
            }
            s2.clear();
            s2=tt;
            //cout<<s2<<" Value of S2";
            s1 = MYFILEPATH + s2;
            //cout<<MYFILEPATH<< " "<<s2<<" MYFILEPATH and s2 Value" <<endl;
            //if(s2 == "foo.png")s1="files/client4/Downloaded/foo.png";

            string sl=get_md5hash(s1);
            transform(sl.begin(), sl.end(), sl.begin(), ::tolower);
            cout<<"Found "<<s2<<" at "<<file_uid_map[itr->first]<<" with MD5 "<< sl  <<" at depth "<<itr->second;
            index++; 
      }
      cout<<endl;
    }    

        //cout<<"Found "<<itr->first<<" at "<< itr->second <<" with MD5 0 at depth "<<file_hop_map[itr->first];
        
      
    mtx.unlock();   
}

void server(int myid,int myuid,int MYPORT,int numNeig,string* fileName,int numFiles,string MYFILEPATH,string* myFiles,int numMyFiles,int *portArray2HopFiles,string *name2HopFiles,string presentFiles){
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
    //map<string,int>file_uid_map;
    map<int,string>uid_file_map;
    map<int,int>uid_newfd_map;
    //map<string,int> file_hop_map;
    for(int i=0;i<numFiles;i++){
        file_uid_map.insert({fileName[i],0});
        file_hop_map.insert({fileName[i],0});
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
    bool uid_file_mapset = false;
    bool didNumberSend = false;
    int arrayNewfd[numNeig]={0};
    while(true){
        if(sockNoArrayIndex == numNeig && ReplyReceived == numNeig){
            if(!uid_file_mapset){
                for(auto element: file_uid_map){
                    uid_file_map.insert({element.second,element.first});
                    uid_file_mapset = true;
                }
            }

            if(!didNumberSend){
                map<int,int> uid_num_map;
                for(auto element: uid_newfd_map){
                    uid_num_map.insert({element.first,0});
                }
                for(auto element: file_uid_map){
                    uid_num_map[element.second]++;

                }
                for(auto element: uid_num_map){
                    if(element.first==0)    continue;
                    //cout<<element.first<<" "<<element.second<<"-----451--------"<<endl;
                    char msg[5000]="%N";
                    string s=to_string(element.second);
                    for (int i = 0; i < s.size(); ++i)
                    {
                        msg[i+2]=(char)s[i];
                    }
                    //cout<<msg<<"-------458-------"<<endl;
                    send(uid_newfd_map[element.first],&msg,sizeof(msg),0);
                }
                didNumberSend=true;

            }

            // for(auto element1: uid_newfd_map){
            //     if(uid_file_map.find(element1.first) == uid_file_map.end()){
            //         char s[250] = "no";
            //         mtx.lock();
            //         // cout<<"beforesend "<<send(uid_newfd_map[element1.first],&s,sizeof(s),0)<<" aftersend "<<s<<" "<<sizeof(s)<<" "<<uid_newfd_map[element1.first]<<" ---------line 295-------"<<endl;
            //         send(uid_newfd_map[element1.first],&s,sizeof(s),0);
            //         mtx.unlock();
            //     }
            // }
            for(auto element: file_uid_map){
                char s[5000] = "send ";
                char msg2[5000]=" "; //new
                if(element.second==0)continue;
                else{
                    for(int i=0;i<element.first.size();i++){
                        s[i+5] = element.first[i];
                    }
                    //mtx.lock();
                    //cout<<s<<"--------456------"<<endl;
                    //sleep(1);
                    // cout<<"beforesend "<<send(uid_newfd_map[element.second],&s,sizeof(s),0)<<" aftersend "<<s<<" "<<sizeof(s)<<" "<<uid_newfd_map[element.second]<<" ---------line 295-------"<<endl;
                    send(uid_newfd_map[element.second],s,sizeof(s),0);
                    //mtx.unlock();
                    string filePathSend = MYFILEPATH +  element.first;
                    recv_file(uid_newfd_map[element.second],filePathSend);
                    file_hop_map[element.first]=1;
                    // recv(uid_newfd_map[element.second],&msg2,sizeof(msg2),0); //new
                }
            }
            break;
        }
        else{
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
                            send(newfd,msg1,sizeof(msg1),0);
                            FD_SET(newfd, &master); // add to master set
                            if (newfd > fdmax) { // keep track of the maximum
                                fdmax = newfd;
                            }
                            // cout<<"--------------"<<newfd<<endl;
                            sockNoArrayIndex++;
                        }
                    }
                    else {
                        char msg1[5000]=" ";
                        int nbytes;
                        if ((nbytes = recv(i, &msg1, sizeof(msg1), 0)) <= 0) {
                            if (nbytes == 0) {
                            }
                            else {
                                //perror("recv");
                            }
                            FD_CLR(i, &master); // remove from master set
                        }
                        else {
                            ReplyReceived++;
                            string s;
                            int k=0;
                            int j=0;
                            while(true){
                                if(msg1[k]=='$'){s=s+' '; k++;j++;}
                                else {s=s+msg1[k];k++;}
                                if(msg1[k]=='#')break;
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
                            uid_newfd_map.insert({uid,i});
                            // cout<<"after insert "<<uid<<" "<<i<<" ---------line 466-------"<<endl;
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
    }

    int m=0;
    for(auto  element: uid_newfd_map){
        if(m<numNeig){
        arrayNewfd[m]=element.second;
            m++;
        }
    }

    int ackreceived=0;
    while (ackreceived<numNeig)
    {
        read_fds = master;
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(1);
        }
        for(int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                char msg1[5000]=" ";
                recv(i, &msg1, sizeof(msg1), 0);
                //cout<<msg1<<endl;
                ackreceived++;
            }
        }
    }
 
    mtx.unlock();
    map<int,int> twoHopuid_port_map;
    int p4Message2hop=0;
    int hop2replyRec=0;
    bool dataSendP2[numNeig]={false};
     while(true){
         //if(p4Message2hop==numNeig) break;
         //data send D2uid$files#
        if(p4Message2hop<numNeig){
            char msg1[5000]=" ";
             msg1[0]='D';
             msg1[1]='2';
             string s = to_string(myuid);
             int index =2;
             for (int i = 0; i < s.size(); ++i)
             {
                 msg1[index++]=(char)s[i];
             }
             s.clear();
             msg1[index++]='$';

             for (auto element: file_uid_map){
                 if(element.second == 0){
                     s=element.first;
                     for (int i = 0; i < s.size(); ++i)
                     {
                         msg1[index++]=(char)s[i];
                     }
                     msg1[index++]='$';
                 }
             }
             msg1[index-1]='&';
            

             for (int i=0;i<numMyFiles;i++){
            
                     s=myFiles[i];
                     for (int i = 0; i < s.size(); ++i)
                     {
                         msg1[index++]=(char)s[i];
                     }
                     msg1[index++]='&';
                
             }
             msg1[index-1]='#';
    //         mtx.lock();
             //cout<<msg1<<"------------line 627------------"<<endl;
    //         mtx.unlock();
             for (int i = 0; i < numNeig; ++i)
             {
                 if(!dataSendP2[i]){
                     //cout<<i<<endl;
                     if(send(arrayNewfd[i],&msg1,sizeof(msg1),0)>=0){
                         dataSendP2[i]=true;
                         p4Message2hop++;
                     }
                 }
             }           
        }
        if(hop2replyRec==numNeig){
            //mtx.lock();
            //cout<<"++++++++++++++++++++++++++++";
            // for(auto element: file_uid_map){
            //     cout<<element.first<<" "<<element.second<<endl;
            // }
            //mtx.unlock();
            break;
        }

         //break;
        read_fds = master;
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(1);
        }
        for(int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                char msg4[5000]=" ";
                int nbytes;
                if ((nbytes = recv(i, &msg4, sizeof(msg4), 0)) <= 0) {
                    if (nbytes == 0) {
                    }
                    else {
                        //perror("recv");
                    }
                    FD_CLR(i, &master); // remove from master set
                }
                else {
                    //if(msg1[0]=='D'&&msg1[1]=='1'){
                    //cout<<msg4<<"---------944----------"<<endl;
                    hop2replyRec++;
                    int index=0;
                    bool gotDollar=false;
                    string s;
                    int l=0;
                    int hop2Uid=0;
                    string hop2File;
                    int hop2Port;
                    bool nothingMsg=true;
                    while(true){
                        //cout<<index;
                        //cout<<msg4<<" Printing msg4 claue!!!!!!"<<endl;
                        if(gotDollar){
                            if(msg4[index]!='$' && msg4[index]!='#' && msg4[index]!='&' && msg4[index]!='%'){
                                s+=(char)msg4[index];
                                index++;
                            }
                            else if(msg4[index]=='$'){
                                hop2Port=stoi(s);
                                
                                twoHopuid_port_map.insert({hop2Uid,hop2Port});
                                
                                index++;
                                s.clear();
                                //cout<<hop2Port<<endl;
                            }
                            else if(msg4[index]=='#'){
                                hop2Uid= stoi(s);
                                index++;
                                s.clear();
                                //cout<<hop2Uid<<" ";
                            }
                            else if(msg4[index]=='&'){
                                hop2File=s;
                                int temp=file_uid_map[s];
                                if(temp==0 && hop2Uid>0){ 
                                    file_uid_map[s]=hop2Uid;
                                    file_hop_map[s]=2;
                                    numberOfFilesFrom2Hop++;
                                }
                                else if(hop2Uid>0){    
                                    file_uid_map[s]=min(file_uid_map[s],hop2Uid);
                                }
                                index++;
                                s.clear();
                                //cout<<hop2File<<" ";
                            }
                            else if(msg4[index]=='%'){
                                hop2Port=stoi(s);
                                twoHopuid_port_map.insert({hop2Uid,hop2Port});
                                index++;
                                s.clear();
                                //cout<<hop2Port<<endl;
                                // for(auto element: twoHopuid_port_map){
                                //     cout<<element.first<<" "<<element.second<<"---------1136---------"<<endl;
                                // }
                                // for(auto element: file_uid_map){
                                //     cout<<element.first<<" "<<element.second<<"--------1139-----------"<<endl;
                                // }
                                // cout<<numberOfFilesFrom2Hop<<"-------1141--------"<<endl;
                                break;
                            }
                        }
                        else{
                            if(msg4[index]=='%'){    
                                nothingMsg=false;
                                break;
                            }
                            else if(msg4[index]=='$'){
                                gotDollar=true;
                                index++;
                            }
                            else{index++;}
                        }
                    }
                    //}
                }
            }
        }


        //get data from tha
     }

    // for(auto element: file_hop_map){
    //     cout<<element.first<<" "<<element.second<<"----------1153---------"<<endl;
    // }
    while(true){
        if(client_acquired_lock)break;
    }
    // mtx.lock();
    // for (auto itr = file_uid_map.begin(); itr != file_uid_map.end(); ++itr) {
        
    //     // if(itr->second == 0)    cout<<"Found "<<itr->first<<" at "<<itr->second<<" with MD5 0 at depth 0";
    //     // else{    
    //     //  string s1= MYFILEPATH + itr->first;
    //     //     string sl = get_md5hash(s1);
    //     //     transform(sl.begin(), sl.end(), sl.begin(), ::tolower);
    //     //     cout<<"Found "<<itr->first<<" at "<<itr->second<<" with MD5 "<< sl <<" at depth 1";

    //     // }
    //     cout<<"Found "<<itr->first<<" at "<< itr->second <<" with MD5 0 at depth "<<file_hop_map[itr->first];
    //     cout<<endl;
    // }   
    // mtx.unlock();
    int index=0;
    for(auto element:file_hop_map){
        if(element.second==2){
            //cout<<file_uid_map[element.first]<<"--------1190--------"<<endl;
            //int i = file_uid_map[element.first];
            //cout<<twoHopuid_port_map[i]<<"--------1192---------"<<endl;
            //cout<<twoHopuid_port_map[file_uid_map[element.first]]<<"--------1193-------"<<endl;
            portArray2HopFiles[index]=twoHopuid_port_map[file_uid_map[element.first]];
            name2HopFiles[index]=element.first;
            //cout<<name2HopFiles[index]<<" "<<portArray2HopFiles[index]<<"------1196------"<<endl;
            index++;
        }
    }
    ReadyToSendData=true;
    while(true){
        read_fds=master;
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
                        char msg1[5000];
                        //cout<<"Received Files"<<msg1<<"--------1198-----";
                        recv(newfd,&msg1,sizeof(msg1),0);
                        string s1=msg1;
                        //cout<<msg1<<"?????????????"<<endl;
                        s1=presentFiles+s1;
                        //cout<<s1<<" sendfie"<<endl;
                        //cout<<"Sending "<<s1<<"---------1204----------"<<endl;

                        send_file(newfd,s1);
                        //sleep(1);
                        //cout<<"Sended"<<endl;
                    }   
                }
            }
        }
    }
}  


int main(int argc, char** argv){
    // FileReading
    string mydir;
    string FILENAME = argv[1];
    string MYFILEPATH = argv[2];
    ifstream fin(FILENAME);
    int myid,myuid,MYPORT;
    int numNeig;
    mydir = argv[2];
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
    string fileDownloadDir = "Downloaded";
    fileDownloadDir = MYFILEPATH + fileDownloadDir;
    if(numFILES!=0){
        if (mkdir(fileDownloadDir.c_str(), 0777) == -1);
        // cerr << "Error : " << strerror(errno) << endl;
    }
    int portArray2HopFiles[numFILES];
    string name2HopFiles[numFILES]; 
    int *f;
    f=portArray2HopFiles;
    string *g;
    g=name2HopFiles;    
    int* a;
    a = neigPORT;
    int* e;
    e=neigID;
    string* b;
    b = fileNAME;
    string* c;
    c = myFiles;
    string* d;
    d=myFiles;
    fileDownloadDir+="/";
    thread t1(server, myid,myuid,MYPORT,numNeig,b,numFILES,fileDownloadDir,d,files.size(),f,g,MYFILEPATH);
    thread t2(client, numNeig,e,a,myuid,c,files.size(),MYFILEPATH,f,g,fileDownloadDir);
    t1.join();
    t2.join();
    string rmdir1 = "Downloaded";
    rmdir1 = MYFILEPATH + rmdir1;
    // if(numFILES!=0){
    //     std::filesystem::remove_all(rmdir1.c_str()); // Deletes one or more files recursively.
    // }
}

