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
std:: mutex mtx;
bool client_acquired_lock = false;
using std::cout; using std::cin;
using std::endl; using std::vector;
#define BACKLOG 10
#define BUFFSIZE 16384 
using namespace std;

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

void client(int numNeig, int* neigPORT,int myuid, string* myFiles,int numMyFiles,string myFilePath){
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
    char msg[10000];
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
            char msg2[10000]=" ";
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
                    char s[10000] = "file"; //new
                    if(FD_ISSET(i,&read_fds1)){
                        char msg3[10000]=" ";
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
                        char msgReply[10000]=" ";
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

void server(int myid,int myuid,int MYPORT,int numNeig,string* fileName,int numFiles,string MYFILEPATH){
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
    map<int,string>uid_file_map;
    map<int,int>uid_newfd_map;
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
    bool uid_file_mapset = false;
    bool didNumberSend = false;
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
                    char msg[10000]="%N";
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
                char s[10000] = "send ";
                char msg2[10000]=" "; //new
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
                            char msg1[10000]=" ";
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
                        char msg1[10000]=" ";
                        int nbytes;
                        if ((nbytes = recv(i, &msg1, sizeof(msg1), 0)) <= 0) {
                            if (nbytes == 0) {
                            }
                            else {
                                perror("recv");
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
    while(true){
        if(client_acquired_lock)break;
    }
    mtx.lock();
    for (auto itr = file_uid_map.begin(); itr != file_uid_map.end(); ++itr) {
        
        if(itr->second == 0)    cout<<"Found "<<itr->first<<" at "<<itr->second<<" with MD5 0 at depth 0";
        else{    
            string s1= MYFILEPATH + itr->first;
            string sl = get_md5hash(s1);
            transform(sl.begin(), sl.end(), sl.begin(), ::tolower);
            cout<<"Found "<<itr->first<<" at "<<itr->second<<" with MD5 "<< sl <<" at depth 1";

        }
        cout<<endl;
    }   
    mtx.unlock();
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
    string fileDownloadDir = "Downloaded";
    fileDownloadDir = MYFILEPATH + fileDownloadDir;
    if(numFILES!=0){
        if (mkdir(fileDownloadDir.c_str(), 0777) == -1);
        // cerr << "Error : " << strerror(errno) << endl;
    }
    int* a;
    a = neigPORT;
    string* b;
    b = fileNAME;
    string* c;
    c = myFiles;
    fileDownloadDir+="/";
    thread t1(server, myid,myuid,MYPORT,numNeig,b,numFILES,fileDownloadDir);
    thread t2(client, numNeig,a,myuid,c,files.size(),MYFILEPATH);
    t1.join();
    t2.join();
}
