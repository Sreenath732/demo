/*
============================================================================================
File: server.cpp

This is used to provide data as JSON by accepting input data as JSON.

Authors:
Kasturi Rangan
Sateesh Reddy
Rajendra Aware
Amol Mulge


Versions:
0.1 : 2021.06.01 - First release
0.2 : 2021.07.14 - setConfig and getConfig APIs queries optimized
============================================================================================
*/
#include <iostream>
#include <string>
#include <algorithm>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
#include <pqxx/pqxx>
#include <errno.h>
#include <pwd.h>
#include <shadow.h>
#include <crypt.h>
#include <jsoncpp/json/json.h>
#include "config_file.h"
#include "pstream.h"
#include <math.h>

#include <chrono>
#include <ctime>

#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#define MINUTE_IN_SECONDS  60
#define HOUR_IN_SECONDS  60 * MINUTE_IN_SECONDS

#define VERSION_NUMBER "2.2.2"
#define VERSION_COMMENT "seventh release"


using namespace std;
using namespace pqxx;
using std::chrono::duration_cast;
using std::chrono::milliseconds;

std::string current_date();
std::string current_time();
std::string now_time();
std::string today_date();
int CheckPassword(const char*, const char*);

string changeUTCtime(const char*);
string changeTimeZone(const char*);
string PortChangeForData(const char*);
string PortChangeForAlarms(const char*);
string dataloggerapiPortChange(const char*);
string checkDataloggerAPIservice(const char*);
string PlcDataloginStatus(const char*);
string AlarmDataloginStatus(const char*);
string process_awlfile(std::vector<std::string>);
string Alarm_Confugration(const char *);
string showDefaultPorts();
string Default_Alarm_Confugration();
void write_version_to_file();
string get_versions();

void clear();
string getutcfromlocalusingdb(const char*);



string get_last_word(string);
string convertToString(char*);
bool port_validate(string);
string getTimezone();
long get_timediff();

std::string host_addr,db_port,db_uname,db_upass,db_database;
string strName, strKey, strComment, strArraySize;

void print(std::vector <string> const &a) {

   std::cout << " The vector elements are : \n";

   for(int i=0; i < a.size(); i++)
   std::cout << a.at(i) << '\n';
}

void splitLine(string strLine){
	char *pStr;
	char *pch;
	string pFinalStr, pStr1, pStr2;
	
	
	pStr = &strLine[0];

	//Remove from the line all characters between { and } inclusive
	pch = strtok( pStr, "{");
	if(pch)
		pStr1= string(pch);
	else
		pStr1 = string("");
	
	pch = strtok(NULL, "}");
	//pch = strtok(NULL, "}");
	if(pch)
		pStr2 = string(pch);
	else
		pStr2 = string("");
	
	pFinalStr = pStr1+pStr2;
		
	pStr = &pFinalStr[0];
	//The first part is the name
	pch = strtok( pStr, ":");
	if(pch)
		strName = string(pch);
	else
		strName = string("");

	//The sceond part is the key
	pch = strtok(NULL, "/");
	if(pch)
		strKey = string(pch);
	else
		strKey = string("");
	
	//Skip another '/' to get the comment
	pch = strtok(NULL, "/"); 
	if(pch)
		strComment = string(pch);
	else
		strComment = string("");

	strName.erase(remove(strName.begin(), strName.end(), ' '), strName.end());
	strName.erase(remove(strName.begin(), strName.end(),'\t'), strName.end());
	strName.erase(remove(strName.begin(), strName.end(),'\r'), strName.end());
	strName.erase(remove(strName.begin(), strName.end(),'"'), strName.end());

	strKey.erase(remove(strKey.begin(), strKey.end(), ' '), strKey.end());
	strKey.erase(remove(strKey.begin(), strKey.end(),'\t'), strKey.end());
	strKey.erase(remove(strKey.begin(), strKey.end(),'\r'), strKey.end());
	strKey.erase(remove(strKey.begin(), strKey.end(),'"'), strKey.end());
	
	strComment.erase(remove(strComment.begin(), strComment.end(),'\t'), strComment.end());
	strComment.erase(remove(strComment.begin(), strComment.end(),'\r'), strComment.end());
	
	//Remove characters from ':' in key, if they exist
	std::size_t found = strKey.find(":");
	if(found != std::string::npos){
		strKey.resize(found);
	}
	// Check if the key is an array
	found = strKey.find("Array");
	//int iSize=0;
	
	if (found==std::string::npos)
		found = strKey.find("ARRAY");
	if (found!=std::string::npos){
		std::size_t iStart = strKey.find("..");
		std::size_t iEnd = strKey.find("]");
		if ( (iStart!=std::string::npos) && (iEnd!=std::string::npos) ){
			strArraySize = strKey.substr(iStart+2, iEnd-(iStart+2));
			//Now see if the array is of Reals, Ints, Bools ot Bytes
			found = strKey.find("REAL");
			int iSize;

			if (found == std::string::npos)
				found = strKey.find("Real");
			if(found != std::string::npos)
			{
				iSize = std::stoi(strArraySize)*4;
				strArraySize = std::to_string(iSize);
			}
			found = strKey.find("INT");
			if (found == std::string::npos)
				found = strKey.find("Int");
			if(found != std::string::npos)
			{
				iSize = std::stoi(strArraySize)*2;
				strArraySize = std::to_string(iSize);
			}
			found = strKey.find("BYTE");
			if (found == std::string::npos)
				found = strKey.find("Byte");
			if(found != std::string::npos)
			{
				iSize = std::stoi(strArraySize);
				strArraySize = std::to_string(iSize);
			}
			found = strKey.find("BOOL");
			if (found == std::string::npos)
				found = strKey.find("Bool");
			if(found != std::string::npos)
			{
				iSize = std::stoi(strArraySize)/8;
				strArraySize = std::to_string(iSize);
			}
		}
		else{
			strArraySize="0";
		}
		strKey = "Array";
	}

	//cout << pFinalStr << endl;
	//cout << "Name: " << strName << " Key: " << strKey << " Comment: " << strComment << endl;
	//cout << endl;
	
}

void handler(int sig)
{
   pid_t pid;
   while ((pid = waitpid(-1, NULL, 0)) > 0); //* Reap a child 
}

int main(int argc, char *argv[])
{	
    if(argc != 2)
    {
        cerr << "Usage: port" << endl;
        exit(0);
    }
    
    //version_info,below time shows compilation time in version_info.conf
  	write_version_to_file();

    int port = atoi(argv[1]);
    sockaddr_in servAddr;
    bzero((char*)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);
 
    int create_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(create_sock < 0)
    {
        cerr << "Error establishing the server socket" << endl;
        exit(0);
    }

    int bindStatus = bind(create_sock, (struct sockaddr*) &servAddr, sizeof(servAddr));
    if(bindStatus < 0)
    {
        cerr << "Error binding socket to local address" << endl;
        exit(0);
    }
		
	int listen_sock = listen(create_sock, 100);
	if(listen_sock < 0)
	{
		cerr << "Could not listen on TCP socket" << endl;
		exit(0);
	}
	
	struct sockaddr_in listen_addr, client_addr;
	socklen_t addr_len = sizeof(listen_addr);
	socklen_t client_len = sizeof(client_addr);

	getsockname(create_sock, (struct sockaddr *) &listen_addr, &addr_len);

	std::vector<std::string> ln = {"HOST_ADDR","DB_PORT","DB_UNAME","DB_UPASS","DB_DATABASE"};
	std::ifstream f_in("/etc/dataloggerapi/dataloggerapi.conf");
	CFG::ReadFile(f_in, ln, host_addr,db_port,db_uname,db_upass,db_database);
	f_in.close();

			//const char *output = "date";
			//int val = system(output);
			//cout << "val====" << val<<endl;
	
	for (;;) 
	{
		int conn_fd = accept(create_sock, (struct sockaddr*) &client_addr, &client_len);
        string cdt = current_date() +" "+ current_time();
        cout << "\n Connection Opened from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
        cout << "Current Date Time" << cdt << endl;
		printf("Connection from: %s:%d\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
		if(fork() == 0) 
		{

			char msg[804800];
			for (;;) 
			{
				int count=1;
				int read_bytes=0;
				int total_bytes=0;
				int received_bytes=0;
				ssize_t num_bytes_received;
				num_bytes_received = recv(conn_fd, msg, 10, 0);
				cout << "read : "<< read_bytes << endl;
				cout << "msg : "<< msg << endl;
				string msgS = msg;
				int j=0;
				bool check;
				if(msgS.size() > 0)
				{
					total_bytes = stoi(msgS);
					cout << "total_bytes : " << total_bytes << endl;
				}
				else
				{
					cout << "\n Connection Closed from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
					cout << "Current Date Time" << cdt << endl;
					printf("Closing connection and exiting: %s:%d\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
					shutdown(conn_fd, SHUT_WR);
					close(conn_fd);
					exit(0);
				}
				do
				{
					read_bytes = recv(conn_fd, &msg[received_bytes], sizeof(msg)-received_bytes, 0);
					if(read_bytes == 0)
					{
						break;
					}					
					if(read_bytes > 0)
					{
						received_bytes += read_bytes;
					}
					cout << count << ". " << "read_bytes : " << read_bytes << " total_received_bytes : " << received_bytes << endl;
					count++;
				}
				while(total_bytes > received_bytes);	
				//cout<<"API called. Length "<< received_bytes << endl;
				//cout << msg << endl;	
				if (received_bytes == 0) 
				{
					string cdt = current_date()+" "+current_time();
					cout << "\n Connection Closed from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
					cout << "Current Date Time" << cdt << endl;
					printf("Closing connection and exiting: %s:%d\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
					shutdown(conn_fd, SHUT_WR);
					close(conn_fd);
					exit(0);
				}

				// Start - Reading from json
				Json::CharReaderBuilder b;
				Json::CharReader* reader(b.newCharReader());
				Json::Value root;
				
				JSONCPP_STRING errs;
				bool ok = reader->parse(msg, msg + strlen(msg), &root, &errs);

				std::string endpointip;
				std::string unameip;
				std::string passcodeip;
				if (ok && errs.size() == 0)
				{
					endpointip = root[0]["endpoint"].asString();
					unameip = root[0]["username"].asString();
					passcodeip = root[0]["passcode"].asString();
				}
				else
				{
					//string cdt = current_date()+" "+current_time();
					cout << "\n Connection Closed from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
					cout << "Current Date Time" << cdt << endl;
					
					shutdown(conn_fd, SHUT_WR);
					close(conn_fd);
					exit(0);
				}
				delete reader;
				
				// End - Reading from json  
				string data;

				Json::Value resultValue;
				Json::StreamWriterBuilder wbuilder;
				std::vector<std::string> paramslineitems;
				if(true){
				std::vector<string>::iterator it;
				std::vector<string> endpointarr {"showUTCtimeAndLocaltime","changeUTCtime","changeTimeZone","PortChangeForData","PortChangeForAlarms","dataloggerapiPortChange","checkDataloggerAPIservice","RestoreAndDefault","PlcDataloginStatus","AlarmDataloginStatus","awlfile","Alarm_Confugration_640","Alarm_Confugration_1280","showDefaultPorts","RestoreAndDefault_DATAPORT","RestoreAndDefault_ALARMPORT","Default_Alarm_Confugration","get_versions"};
				
				it = std::find(endpointarr.begin(), endpointarr.end(), endpointip);
				
				if (it != endpointarr.end())
				{
			
					if(endpointip == endpointarr[0])
					{
						cout << "\n showUTCtimeAndLocaltime API Request from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
						
						std::string output1;
						std::string output2;

						// utc time
						time_t now = time(NULL);
						tm *gmtm = gmtime(&now);
						char buffer[80];

						strftime (buffer,80,"%d-%m-%Y %H:%M:%S ",gmtm);

						output1 = convertToString(buffer);

						//local time
						//time_t givemetime = time(NULL);

						tm *gmtm1 = localtime(&now);

						char buffer1[80];
						std::string current_Timezone = getTimezone();


						strftime (buffer1,80,"%d-%m-%Y %H:%M:%S ",gmtm1);

						output2 = convertToString(buffer1);

						std::string localtime_Timezone = output2;
						//std::string localtime_Timezone = output2 + current_Timezone;

						Json::Value jVal1 , jVal2,jVal3;

						jVal1["utctime"] = output1;
						jVal2["localtime"] = localtime_Timezone;
						jVal3["localtimezone"] = current_Timezone;

						resultValue["field"].append(jVal1);
						resultValue["field"].append(jVal2);
						resultValue["field"].append(jVal3);


						data = Json::writeString(wbuilder, resultValue);
						//cout << data << endl;
						send(conn_fd, data.c_str(), data.length(), 0);  
					}
					if(endpointip == endpointarr[1])
					{
						std::string newUTCtime;
						//std::string passWd;

						cout << "\n changeUTCtime API Request from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
	
						//passWd = root[0]["password"].asString();
						//const char* Pass_Word = passWd.c_str();
						newUTCtime = root[0]["newTime"].asString();
						const char* new_UTCtime = newUTCtime.c_str();

						data = changeUTCtime(new_UTCtime);
						send(conn_fd, data.c_str(), data.length(), 0);
					}
					
					if(endpointip == endpointarr[2])
					{
						
						cout << "\n changeTimeZone API Request from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
						std::string timeZone;
						//std::string passWd;
						timeZone = root[0]["timezone"].asString();
						const char* Time_Zone = timeZone.c_str();
		

						data = changeTimeZone(Time_Zone);
						send(conn_fd, data.c_str(), data.length(), 0);
					}
					if(endpointip == endpointarr[3])
					{
						cout << "\n PortChangeForData API Request from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
						std:: string Port;
						Port= root[0]["port"].asString();
						const char* PORT = Port.c_str();
						//std:: string Passwd;
						//Passwd= root[0]["password"].asString();
						//const char* Pass_Word = Passwd.c_str();

						data = PortChangeForData(PORT);
						send(conn_fd, data.c_str(), data.length(), 0);

					}
					if(endpointip == endpointarr[4])
					{
						cout << "\n PortChangeForAlarms API Request from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
						std:: string Port;
						Port= root[0]["port"].asString();
						const char* PORT = Port.c_str();
						//std:: string Passwd;
						//Passwd= root[0]["password"].asString();
						//const char* Pass_Word = Passwd.c_str();

						data = PortChangeForAlarms(PORT);
						send(conn_fd, data.c_str(), data.length(), 0);

					}
					if(endpointip == endpointarr[5])
					{
						cout << "\n dataloggerapiPortChange API Request from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
						std::string portNumber;
						portNumber = root[0]["port"].asString();
						const char* PORT = portNumber.c_str();
						//std::string passWord;
						//passWord = root[0]["password"].asString();
						//const char* Pass_Word = passWord.c_str();

						data = dataloggerapiPortChange(PORT);
						send(conn_fd, data.c_str(), data.length(), 0);

					}
					if(endpointip == endpointarr[6])
					{
						cout << "\n checkDataloggerAPIservice API Request from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
						std::string portNumber;
						portNumber = root[0]["port"].asString();
						const char* PORT = portNumber.c_str();
						//std:: string Passwd;
						//Passwd= root[0]["password"].asString();
						//const char* Pass_Word = Passwd.c_str();
						
						data = checkDataloggerAPIservice(PORT);
						send(conn_fd, data.c_str(), data.length(), 0);

					}
					if(endpointip == endpointarr[7])
					{
						cout << "\n RestoreAndDefault API Request from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
						std::string portNumber = "6789";
						const char* PORT = portNumber.c_str();
						//std::string passWord;
						//passWord = root[0]["password"].asString();
						//const char* Pass_Word = passWord.c_str();

						data = dataloggerapiPortChange(PORT);
						send(conn_fd, data.c_str(), data.length(), 0);
					}
					if(endpointip == endpointarr[8])
					{
						cout << "\n PlcDataloginStatus API Request from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
						std::string utcTime;
						utcTime = root[0]["UTCTime"].asString();
						const char* utc_time = utcTime.c_str();

						data = PlcDataloginStatus(utc_time);
						send(conn_fd, data.c_str(), data.length(), 0);
					}
					if(endpointip == endpointarr[9])
					{
						cout << "\n AlarmDataloginStatus API Request from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
						std::string utcTime;
						utcTime = root[0]["UTCTime"].asString();
						const char* utc_time = utcTime.c_str();

						data = AlarmDataloginStatus(utc_time);
						send(conn_fd, data.c_str(), data.length(), 0);

					}
					if(endpointip == endpointarr[10])
					{
						cout << "\n process_awlfile API Request from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
						int lineitemCount = root[0]["line"].size();
					
						for ( int index = 0; index < lineitemCount; ++index )
						{						
							paramslineitems.push_back(root[0]["line"][index].asString());
						}
						
						data = process_awlfile(paramslineitems);
						send(conn_fd, data.c_str(), data.length(), 0);
					}
					if(endpointip == endpointarr[11])
					{
						cout << "\n Alarm_Confugration_640 API Request from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
						std::string Alarm_counts;
						Alarm_counts = root[0]["alarm_count"].asString();
						const char* AlarmCounts = Alarm_counts.c_str();

						data = Alarm_Confugration(AlarmCounts);
						send(conn_fd, data.c_str(), data.length(), 0);
					}
					if(endpointip == endpointarr[12])
					{
						cout << "\n Alarm_Confugration_1280 API Request from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
						std::string Alarm_counts;
						Alarm_counts = root[0]["alarm_count"].asString();
						const char* AlarmCounts = Alarm_counts.c_str();

						data = Alarm_Confugration(AlarmCounts);
						send(conn_fd, data.c_str(), data.length(), 0);
					}
					if(endpointip == endpointarr[13])
					{
						cout << "\n showDefaultPorts API Request from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
				
						data = showDefaultPorts();
						send(conn_fd, data.c_str(), data.length(), 0);
					}	
					if(endpointip == endpointarr[14])
					{
						cout << "\n RestoreAndDefault_DATAPORT API Request from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
						
						std::string Port = "40001";
						const char* PORT = Port.c_str();
						//std:: string Passwd;
						//Passwd= root[0]["password"].asString();
						//const char* Pass_Word = Passwd.c_str();

						data = PortChangeForData(PORT);
						send(conn_fd, data.c_str(), data.length(), 0);
					}
					if(endpointip == endpointarr[15])
					{
						cout << "\n RestoreAndDefault_ALARMPORT API Request from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
						
						std::string Port = "40002";
						const char* PORT = Port.c_str();
						//std:: string Passwd;
						//Passwd= root[0]["password"].asString();
						//const char* Pass_Word = Passwd.c_str();

						data = PortChangeForAlarms(PORT);
						send(conn_fd, data.c_str(), data.length(), 0);
					}
					if(endpointip == endpointarr[16])
					{
						cout << "\n Default_Alarm_Confugration API Request from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
						data = Default_Alarm_Confugration();
						send(conn_fd, data.c_str(), data.length(), 0);
					}
					if(endpointip == endpointarr[17])
					{
						cout << "\n get_versions API Request from " << inet_ntoa(client_addr.sin_addr) << "," << client_addr.sin_port << endl;
						data = get_versions();
						send(conn_fd, data.c_str(), data.length(), 0);
					}
				}
				else
				{
					cout << "Wrong endpoint" << endl;
					std::string output = "Wrong endpoint";
					data = output;
				}
				shutdown(conn_fd, SHUT_RDWR);
                close(conn_fd);
                exit(0);
			  }
			  else{
			  	int checkAuth = 0;
			  	cout << "Inside checkAuth: " << checkAuth << endl;
				Json::Value jVal1, jVal2;
					
				jVal1["result"] = "Fail";
				jVal2["reason"] = "Invalid Credentials";
				resultValue["field"].append(jVal1);
				resultValue["field"].append(jVal2);
				data  = Json::writeString(wbuilder, resultValue); 
				send(conn_fd, data.c_str(), data.length(), 0);
			  }
			  //send(conn_fd, data.c_str(), data.length(), 0);
			  //printf("Sent: %d bytes\n", bytes_sent);
			}
		}
		else 
		{
			close(conn_fd);
			signal(SIGCHLD, SIG_IGN);
		}
	}
	return 0;
}


/*
----------------------------------------------------------------------------------------
This function gets called when the changeUTCtime API was invoked
This function handles error responses in the follwing cases
	1. When the database connection failed
	2. When the unidentified errors occured
This function will be used to return extract data from data_table with unpacked parameters between the given start time and end time as JOSN format.
-------------------------------------------------------------------------------------------
*/

string changeUTCtime(const char* new_UTCtime)
{
	Json::Value resultValue;
	Json::StreamWriterBuilder wbuilder;
	//std::string passWd = Pass_Word;
	std::string newUTCtime = new_UTCtime;
	std::string output;

	std::string localTimeZone = getTimezone();
	//std::string qery = "echo "+passWd+" | yes | timedatectl set-ntp 0";
	std::string qery = "sudo timedatectl set-ntp 0";
	system(qery.c_str());

	//when we need do run commissioning-server service from user then we need to provide password along with queries.
	//for now we are running commissioning-server service from root so no need of password. 
	/*std::string qry = "echo "+passWd+" | sudo -S sudo timedatectl set-timezone UTC ";
	std::string qryTimeSet = "echo "+passWd+" | sudo -S  sudo timedatectl set-time '"+newUTCtime+"'";
	std::string qry1 = "echo "+passWd+" | sudo -S  sudo timedatectl set-timezone  "+localTimeZone+" ";*/

	std::string qry = "sudo timedatectl set-timezone UTC ";
	std::string qryTimeSet = "sudo timedatectl set-time '"+newUTCtime+"'";
	std::string qry1 = "sudo timedatectl set-timezone  "+localTimeZone+" ";

	//signal handling
    signal(SIGCHLD, handler);  //go to handler
  
	int n=0;
	n=system(qry.c_str());
	//cout << "value of n : " << n << endl;
	if(n==0)
	{
		int m;
		m = system(qryTimeSet.c_str());
		//cout << "value of m :"<<m<<endl;
		if(m ==0)
		{
		    int k;
		    k = system(qry1.c_str());
		    //cout <<"value of k :"<<k<<endl;
		    if(k == 0)
		    {
    	    	Json::Value jVal1, jVal2;
		
				jVal1["result"] = "Success";
				jVal2["Description"] = "UTC Time Set to "+newUTCtime+"";
				resultValue["field"].append(jVal1);
				resultValue["field"].append(jVal2);
				output  = Json::writeString(wbuilder, resultValue); 
		    }
		    else
		    {

				Json::Value jVal1, jVal2;
				
				jVal1["result"] = "failed";
				jVal2["Description"] =" Time not set ";
				resultValue["field"].append(jVal1);
				resultValue["field"].append(jVal2);
				output  = Json::writeString(wbuilder, resultValue);
		    }

		}
		else
	    {
				  	
		    Json::Value jVal1, jVal2;
					
			jVal1["result"] = "failed";
			jVal2["Description"] =" Time not set ";
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);
			output  = Json::writeString(wbuilder, resultValue);
		}
	}
	else
	{	  
		Json::Value jVal1, jVal2;
			
		jVal1["result"] = "failed";
		jVal2["Description"] =" Time not set ";
		resultValue["field"].append(jVal1);
		resultValue["field"].append(jVal2);
		output  = Json::writeString(wbuilder, resultValue);  
	}
	return output;
}

/*
----------------------------------------------------------------------------------------
This function gets called when the changeTimeZone API was invoked
This function handles error responses in the follwing cases
	1. When the database connection failed
	2. When the unidentified errors occured
This function will be used to return extract data from data_table with unpacked parameters between the given start time and end time as JOSN format.
-------------------------------------------------------------------------------------------
*/

string changeTimeZone (const char* Time_Zone)
{
	Json::Value resultValue;
	Json::StreamWriterBuilder wbuilder;
	std::string output;
	//std::string passWd = Pass_Word;
	std::string timeZone = Time_Zone;

	//std::string qery = "echo "+passWd+" | sudo -S sudo timedatectl set-ntp 1";
	//system(qery.c_str());
	//std::string qry = "echo "+passWd+" | sudo -S sudo timedatectl set-timezone "+timeZone+"";

	std::string qry = "sudo timedatectl set-timezone "+timeZone+"";
	std::string pgtimezone = "sudo -u postgres psql -c \"ALTER DATABASE plc_data SET timezone TO '"+timeZone+"';\"";
	std::string restart = "sudo service postgresql restart";
	//cout<<qry<<endl;

	//signal handling
    signal(SIGCHLD, handler);  //go to handler
   
	int n;
	n=system(qry.c_str());
	cout<<"n=="<<n<<endl;
	if(n==0)
	{
		int o;
		o=system(pgtimezone.c_str());
		cout<<"o=="<<o<<endl;
		if(o==0)
		{
			int p;
			p=system(restart.c_str());
			cout<<"p=="<<p<<endl;
			if(p==0)
			{
				Json::Value jVal1, jVal2;
		
				jVal1["result"] = "Success";
				jVal2["Description"] = "Time Zone changed to "+timeZone+"";
				resultValue["field"].append(jVal1);
				resultValue["field"].append(jVal2);
				output  = Json::writeString(wbuilder, resultValue);
			}
			else
			{
				Json::Value jVal1, jVal2;
		
				jVal1["result"] = "failed 3";
				jVal2["Description"] = "Timezone not changed";
				resultValue["field"].append(jVal1);
				resultValue["field"].append(jVal2);
				output  = Json::writeString(wbuilder, resultValue); 
			}	
		}
		else
		{
			Json::Value jVal1, jVal2;
		
			jVal1["result"] = "failed 2";
			jVal2["Description"] = "Timezone not changed";
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);
			output  = Json::writeString(wbuilder, resultValue);
		}
				 
	}
	else
	{
		Json::Value jVal1, jVal2;
		
		jVal1["result"] = "failed 1";
		jVal2["Description"] = "Timezone not changed";
		resultValue["field"].append(jVal1);
		resultValue["field"].append(jVal2);
		output  = Json::writeString(wbuilder, resultValue);
	} 	
	return output;
}


/*-----------------------------------------------------------------------------------------------
This function gets called when the PortChangeForData API was invoked
--------------------------------------------------------------------------------------------------*/	
string PortChangeForData(const char* PORT)
{
	std::string cur_date= current_date();
	std::string cur_time= current_time();
	Json::StreamWriterBuilder wbuilder;
	Json::Value resultValue;
	std::string output;
	std::string Port = PORT;

    //by getting line_no i will get next lines sentence to fetch port no

	   ifstream aFile ("/etc/plcdatacollector/plcdatacollector.conf");   //server
       int lines_count =0;
       int line_num=0;
       std::string line,replaceStr,constant = "#Remote DATA_PORT to listen on";
       while (std::getline(aFile , line))
       {
	        ++lines_count;
	        if(constant == line)
	        {
	        	line_num=lines_count;
	        }
	        if(lines_count==line_num+1)
	        {
	        	replaceStr = line;
	        	//break;
	        }
       }   

       cout<<"line-numberrr=="<<line_num<<endl;
       cout<<"replaceStrrr=="<<replaceStr<<endl;

      	//feetching port no from the sentence
		string tmp;
		string oldport;
        stringstream sstream_word(replaceStr);
    		vector<string> words;
    		while(sstream_word >> tmp)
    		{
    			words.push_back(tmp);
    		}
        for(int i = 0; i < words.size(); i++)
        {
                if(i == 2)
                {
                	oldport = words.at(i);
                }
        }

	//signal handling
    signal(SIGCHLD, handler);  //go to handler
    
    if (port_validate(Port))
    {
    	/*std::string history_qry = "echo "+Passwd+" | sudo  -S  sudo sed -i '18 i DATA_PORT = "+Port+"   	#Changed to "+Port+" on "+cur_date+" "+cur_time+"' /etc/plcdatacollector/plcdatacollector.conf";
		std::string qry2 = "echo "+Passwd+" | sudo  -S  sed -i '19 s/^/#/' /etc/plcdatacollector/plcdatacollector.conf";
		std::string unblock = "echo "+Passwd+" | sudo -S sudo firewall-cmd --zone=public --permanent --add-port="+Port+"/udp";
		std::string reload = "echo "+Passwd+" | sudo -S sudo firewall-cmd --reload";
		std::string reload_qry = "echo "+Passwd+" | sudo  -S  sudo systemctl daemon-reload";
		std::string restart_qry = "echo "+Passwd+" | sudo  -S sudo service dataloggerapi restart"; */

		std::string history_qry = "sudo sed -i '"+to_string(line_num+1)+" i DATA_PORT = "+Port+"   	#Changed to "+Port+" on "+cur_date+" "+cur_time+"' /etc/plcdatacollector/plcdatacollector.conf";
		std::string qry2 = "sudo sed -i '"+to_string(line_num+2)+" s/^/#/' /etc/plcdatacollector/plcdatacollector.conf";
		std::string unblock = "sudo firewall-cmd --zone=public --permanent --add-port="+Port+"/udp";
		std::string reload = "sudo firewall-cmd --reload";
		std::string reload_qry = "sudo systemctl daemon-reload";
		std::string restart_qry = "sudo service plcdatacollector restart";

		int m;
		m=system(unblock.c_str());
		//cout << "value of m : " << m << endl;
		if(m==0)
		{
			int n;
			n=system(history_qry.c_str());
			//cout << "value of n : " << n << endl;
			if(n==0)
			{
				int o;
				o=system(qry2.c_str());
				//cout << "value of o : " << o << endl;
				if(o==0)
				{
					int p;
					p=system(reload.c_str());
					//cout << "value of p : " << p << endl;
					if(p==0)
					{
						int q;
						q=system(reload_qry.c_str());
						//cout << "value of q : " << q << endl;
						if(q==0)
						{
							int r;
							r=system(restart_qry.c_str());
							//cout << "value of r : " << r << endl;
							if(r==0)
							{
								if(Port != "40001")
								{
									Json::Value jVal1, jVal2;
													
									jVal1["result"] = "success";
									jVal2["reason"] = "Data Port "+oldport+" Changed to "+Port+"";
									resultValue["field"].append(jVal1);
									resultValue["field"].append(jVal2);

									output  = Json::writeString(wbuilder, resultValue);
								}
								else
								{
									Json::Value jVal1, jVal2;
													
									jVal1["result"] = "success";
									jVal2["reason"] = "Data Port "+oldport+" Changed to Default "+Port+"";
									resultValue["field"].append(jVal1);
									resultValue["field"].append(jVal2);

									output  = Json::writeString(wbuilder, resultValue);
								}


							}
							else
							{
								Json::Value jVal1, jVal2;
					
								jVal1["result"] = "Fail7";
								jVal2["reason"] = "Could not change data port";
								resultValue["field"].append(jVal1);
								resultValue["field"].append(jVal2);

								output  = Json::writeString(wbuilder, resultValue);

							}

						}
						else
						{
							Json::Value jVal1, jVal2;
					
							jVal1["result"] = "Fail6";
							jVal2["reason"] = "Could not change data port";
							resultValue["field"].append(jVal1);
							resultValue["field"].append(jVal2);

							output  = Json::writeString(wbuilder, resultValue);

						}	

					}
					else
					{
						Json::Value jVal1, jVal2;
					
						jVal1["result"] = "Fail5";
						jVal2["reason"] = "Could not change data port";
						resultValue["field"].append(jVal1);
						resultValue["field"].append(jVal2);

						output  = Json::writeString(wbuilder, resultValue);	
					}
				}
				else
				{
					Json::Value jVal1, jVal2;
					
					jVal1["result"] = "Fail4";
					jVal2["reason"] = "Could not change data port";
					resultValue["field"].append(jVal1);
					resultValue["field"].append(jVal2);

					output  = Json::writeString(wbuilder, resultValue);

				}									 
			}
			else
			{
				Json::Value jVal1, jVal2;
					
				jVal1["result"] = "Fail3";
				jVal2["reason"] = "Could not change data port";
				resultValue["field"].append(jVal1);
				resultValue["field"].append(jVal2);

				output  = Json::writeString(wbuilder, resultValue);  
			}
		}	
		/*else if(m==256)
		{
			Json::Value jVal1, jVal2;
					
			jVal1["result"] = "Fail";
			jVal2["reason"] = "Wrong password";
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);

			output  = Json::writeString(wbuilder, resultValue);  
		}*/
		else
		{
			Json::Value jVal1, jVal2;
					
			jVal1["result"] = "Fail2";
			jVal2["reason"] = "invalid data port number";
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);

			output  = Json::writeString(wbuilder, resultValue);  
		}
	}
	else
	{
	   	Json::Value jVal1, jVal2;
					
		jVal1["result"] = "Fail1";
		jVal2["reason"] = "invalid data port number";
		resultValue["field"].append(jVal1);
		resultValue["field"].append(jVal2);

		output  = Json::writeString(wbuilder, resultValue);
	}
	return output;
}

/*-----------------------------------------------------------------------------------------------
This function gets called when the PortChangeForAlarms API was invoked
--------------------------------------------------------------------------------------------------*/	
string PortChangeForAlarms(const char* PORT)
{
	std::string cur_date= current_date();
	std::string cur_time= current_time();
	Json::StreamWriterBuilder wbuilder;
	Json::Value resultValue;
	std::string output;
	std::string Port = PORT;
	//std::string Passwd = Pass_Word;

	//by getting line_no i will get next lines sentence to fetch port no

	   ifstream aFile ("/etc/plcdatacollector/plcdatacollector.conf");   //server
       int lines_count =0;
       int line_num=0;
       std::string line,replaceStr,constant = "#Remote ALARM_PORT to listen on";
       while (std::getline(aFile , line))
       {
	        ++lines_count;
	        if(constant == line)
	        {
	        	line_num=lines_count;
	        }
	        if(lines_count==line_num+1)
	        {
	        	replaceStr = line;
	        	//break;
	        }
       }   

       cout<<"line-numberrr=="<<line_num<<endl;
       cout<<"replaceStrrr=="<<replaceStr<<endl; 
      
	//feetching port no from the replaceStr
		string tmp;
		string oldport;
        stringstream sstream_word(replaceStr);
    	vector<string> words;
    	while(sstream_word >> tmp)
    	{
    		words.push_back(tmp);
    	}
        for(int i = 0; i < words.size(); i++)
        {
            if(i == 2)
            {
            	oldport = words.at(i);
            }
        }

    //signal handling
    signal(SIGCHLD, handler);  //go to handler
   
    if (port_validate(Port))
    {
    	/*std::string history_qry = "echo "+Passwd+" | sudo -S sudo sed -i '"+fech_line_no+" i ALARM_PORT = "+Port+"   	#Changed to "+Port+" on "+cur_date+" "+cur_time+"' /etc/plcdatacollector/plcdatacollector.conf";
		std::string comment_qry = "echo "+Passwd+" | sudo -S sed -i '"+fech_comment_line_no+" s/^/#/' /etc/plcdatacollector/plcdatacollector.conf";
		std::string unblock = "echo "+Passwd+" | sudo -S sudo firewall-cmd --zone=public --permanent --add-port="+Port+"/udp";
		std::string reload = "echo "+Passwd+" | sudo -S sudo firewall-cmd --reload";
		std::string reload_qry = "echo "+Passwd+" | sudo -S sudo systemctl daemon-reload";			
		std::string restart_qry = "echo "+Passwd+" | sudo  -S sudo service dataloggerapi restart"; */

		std::string history_qry = " sudo sed -i '"+to_string(line_num+1)+" i ALARM_PORT = "+Port+"   	#Changed to "+Port+" on "+cur_date+" "+cur_time+"' /etc/plcdatacollector/plcdatacollector.conf";
		std::string comment_qry = "sudo sed -i '"+to_string(line_num+2)+" s/^/#/' /etc/plcdatacollector/plcdatacollector.conf";
		std::string unblock = "sudo firewall-cmd --zone=public --permanent --add-port="+Port+"/udp";
		std::string reload = "sudo firewall-cmd --reload";
		std::string reload_qry = "sudo systemctl daemon-reload";			
		std::string restart_qry = "sudo service alarmdatacollector restart";

		int m;
		m=system(unblock.c_str());
		//cout << "value of m : " << m << endl;
		if(m==0)
		{
			int n;
			n=system(history_qry.c_str());
			//cout << "value of n : " << n << endl;
			if(n==0)
			{
				int o;
				o=system(comment_qry.c_str());
				//cout << "value of o : " << o << endl;
				if(o==0)
				{
					int p;
					p=system(reload.c_str());
					//cout << "value of p : " << p << endl;
					if(p==0)
					{
						int q;
						q=system(reload_qry.c_str());
						cout << "value of q : " << q << endl;
						if(q==0)
						{
							int r;
							r=system(restart_qry.c_str());
							//cout << "value of r : " << r << endl;
							if(r==0)
							{
								if(Port != "40002")
								{
									Json::Value jVal1, jVal2;
													
									jVal1["result"] = "success";
									jVal2["reason"] = "Alarm Port "+oldport+" Changed to "+Port+"";
									resultValue["field"].append(jVal1);
									resultValue["field"].append(jVal2);

									output  = Json::writeString(wbuilder, resultValue);
								}
								else
								{
									Json::Value jVal1, jVal2;
													
									jVal1["result"] = "success";
									jVal2["reason"] = "Alarm Port "+oldport+" Changed to Default "+Port+"";
									resultValue["field"].append(jVal1);
									resultValue["field"].append(jVal2);

									output  = Json::writeString(wbuilder, resultValue);
								}


							}
							else
							{
								Json::Value jVal1, jVal2;
					
								jVal1["result"] = "Fail";
								jVal2["reason"] = "Could not change alarm port";
								resultValue["field"].append(jVal1);
								resultValue["field"].append(jVal2);

								output  = Json::writeString(wbuilder, resultValue);

							}

						}
						else
						{
							Json::Value jVal1, jVal2;
					
							jVal1["result"] = "Fail";
							jVal2["reason"] = "Could not change alarm port";
							resultValue["field"].append(jVal1);
							resultValue["field"].append(jVal2);

							output  = Json::writeString(wbuilder, resultValue);

						}	

					}
					else
					{
						Json::Value jVal1, jVal2;
					
						jVal1["result"] = "Fail";
						jVal2["reason"] = "Could not change alarm port";
						resultValue["field"].append(jVal1);
						resultValue["field"].append(jVal2);

						output  = Json::writeString(wbuilder, resultValue);	
					}
				}
				else
				{
					Json::Value jVal1, jVal2;
					
					jVal1["result"] = "Fail";
					jVal2["reason"] = "Could not change alarm port";
					resultValue["field"].append(jVal1);
					resultValue["field"].append(jVal2);

					output  = Json::writeString(wbuilder, resultValue);

				}									 
			}
			else
			{
				Json::Value jVal1, jVal2;
					
				jVal1["result"] = "Fail";
				jVal2["reason"] = "Could not change alarm port";
				resultValue["field"].append(jVal1);
				resultValue["field"].append(jVal2);

				output  = Json::writeString(wbuilder, resultValue);  
			}
		}
		/*else if(m==256)
		{
			Json::Value jVal1, jVal2;
					
			jVal1["result"] = "Fail";
			jVal2["reason"] = "Wrong password";
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);

			output  = Json::writeString(wbuilder, resultValue);  
		}*/	
		else
		{
			Json::Value jVal1, jVal2;
					
			jVal1["result"] = "Fail";
			jVal2["reason"] = "invalid alarm port number";
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);

			output  = Json::writeString(wbuilder, resultValue);  
		}
	
    }
    else
    {
   		Json::Value jVal1, jVal2;
					
		jVal1["result"] = "Fail";
		jVal2["reason"] = "invalid alarm port number";
		resultValue["field"].append(jVal1);
		resultValue["field"].append(jVal2);

		output  = Json::writeString(wbuilder, resultValue);	
    }
    return output;
}

/*  
----------------------------------------------------------------------------------------
This function gets called when the dataloggerapiPortChange API was invoked
This function handles error responses in the follwing cases
	1. When the unidentified errors occured
This function will be used to change port for dataloggerapi service.
-------------------------------------------------------------------------------------------
*/

string dataloggerapiPortChange(const char* PORT)
{
	std::string output;
	Json::StreamWriterBuilder wbuilder;
	Json::Value resultValue;
	std::string portNumber = PORT;
	//std::string passWord = Pass_Word;

	ifstream aFile ("/etc/systemd/system/dataloggerapi.service");  
			 
	std::size_t lines_count =0;
	std::string line,replaceStr;
	while (std::getline(aFile , line))
	{
		++lines_count;
		if(lines_count == 2)
		{
			replaceStr = line;
	    }
	}         
	string s = replaceStr; 

	string oldport = get_last_word(s);
			
	/*std::string qry= "echo "+passWord+" | sudo -S sudo sed -i 's/"+oldport+"/"+portNumber+"/' /etc/systemd/system/dataloggerapi.service"; 		
	std::string unblock = "echo "+passWord+" | sudo -S sudo firewall-cmd --zone=public --permanent --add-port="+portNumber+"/tcp";
	std::string reload = "echo "+passWord+" | sudo -S sudo firewall-cmd --reload";
	std::string qry1 = "echo "+passWord+" | sudo -S sudo systemctl daemon-reload";
	std::string qry2 = "echo "+passWord+" | sudo -S sudo service dataloggerapi restart";*/

	std::string qry= "sudo sed -i 's/"+oldport+"/"+portNumber+"/' /etc/systemd/system/dataloggerapi.service"; 		
	std::string unblock = "sudo firewall-cmd --zone=public --permanent --add-port="+portNumber+"/tcp";
	std::string reload = "sudo sudo firewall-cmd --reload";
	std::string qry1 = "sudo systemctl daemon-reload";
	std::string qry2 = "sudo service dataloggerapi restart";

	//signal handling
	signal(SIGCHLD, handler);  //go to handler
		    
	if (port_validate(portNumber))
	{
		int m=0;
		m=system(unblock.c_str());
		//cout << "value of m : " << m << endl;
		if(m==0)
		{
			int n=0;
			n=system(qry.c_str());
			//cout << "value of n : " << n << endl;
			if(n==0)
			{
				int k=0;
				k=system(reload.c_str());
				//cout << "value of k : " << k << endl;
				if(k==0)
				{
					int l=0;
					l=system(qry1.c_str());
					//cout << "value of l : " << l << endl;
					if(l==0)
					{
						int p=0;
						p=system(qry2.c_str());
						//cout << "value of p : " << p << endl;
						if(p==0)
						{
							Json::Value jVal1, jVal2;
															
							jVal1["result"] = "success";
							jVal2["Description"] = "dataloggerapi port changed to "+portNumber+"";
							resultValue["field"].append(jVal1);
							resultValue["field"].append(jVal2);
							output  = Json::writeString(wbuilder, resultValue); 
						}
						else
						{
							Json::Value jVal1, jVal2;
														
							jVal1["result"] = "Fail";
							jVal2["Description"] = "Could not change port";
							resultValue["field"].append(jVal1);
							resultValue["field"].append(jVal2);
							output  = Json::writeString(wbuilder, resultValue);
						}
					}
					else
					{
						Json::Value jVal1, jVal2;
												
						jVal1["result"] = "Fail";
						jVal2["Description"] = "Could not change port";
						resultValue["field"].append(jVal1);
						resultValue["field"].append(jVal2);
						output  = Json::writeString(wbuilder, resultValue);
					}
				}
				else
				{
					Json::Value jVal1, jVal2;
											
					jVal1["result"] = "Fail";
					jVal2["Description"] = "Could not change port";
					resultValue["field"].append(jVal1);
					resultValue["field"].append(jVal2);
					output  = Json::writeString(wbuilder, resultValue);

				}		
			}
			else
			{
				Json::Value jVal1, jVal2;
									
				jVal1["result"] = "Fail";
				jVal2["Description"] = "Could not change port";
				resultValue["field"].append(jVal1);
				resultValue["field"].append(jVal2);
				output  = Json::writeString(wbuilder, resultValue);  
			}
 
		}
		/*else if(m==256)
		{
			Json::Value jVal1, jVal2;
					
			jVal1["result"] = "Fail";
			jVal2["Description"] = "Wrong password";
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);

			output  = Json::writeString(wbuilder, resultValue);  
		}*/
		else
		{
			Json::Value jVal1, jVal2;
							
			jVal1["result"] = "Fail";
			jVal2["Description"] = "invalid port number";
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);
			output  = Json::writeString(wbuilder, resultValue);  
		}

	}
	else
	{
		Json::Value jVal1, jVal2;
							
		jVal1["result"] = "Fail1";
		jVal2["Description"] = "invalid port number";
		resultValue["field"].append(jVal1);
		resultValue["field"].append(jVal2);

		output  = Json::writeString(wbuilder, resultValue);	
    }
    return output;  
    		
}


/*----------------------------------------------------------------------------------------
This function gets called when the checkDataloggerAPIservice API was invoked
This function handles error responses in the follwing cases
	1. When the unidentified errors occured
This function will be used to check the after changing port for dataloggerapi service working properly. 
-------------------------------------------------------------------------------------------*/

string checkDataloggerAPIservice(const char* PORT)
{
	std::string output;
	Json::StreamWriterBuilder wbuilder;
	Json::Value resultValue;
	//std::string Passwd = Pass_Word;
	std::string Port_no = PORT;

	ifstream aFile ("/etc/systemd/system/dataloggerapi.service");  
			 
	std::size_t lines_count =0;
	std::string line,replaceStr;
	while (std::getline(aFile , line))
	{
		++lines_count;
		if(lines_count == 2)
		{
			replaceStr = line;
	    }
	}         
	string s = replaceStr; 

	string oldport = get_last_word(s);			//current port on which service is running

	//signal handling
    signal(SIGCHLD, handler);  //go to handler
  
  	std::string status_qry = "sudo service dataloggerapi status";

    if(port_validate(Port_no))
    {
    	int n;
    	n=system(status_qry.c_str());
    	//cout << "value of n : " << n << endl;
    	/*if(n==256)
    	{
    		Json::Value jVal1, jVal2;

    		jVal1["status"] = "Fail";
    		jVal2["Description"] = "Wrong password";
    		resultValue["field"].append(jVal1);
    		resultValue["field"].append(jVal2);

    		output  = Json::writeString(wbuilder, resultValue);  
    		return output;
    	}*/

    	if(oldport == Port_no)
    	{    	
			if(n==0)
			{
				Json::Value jVal1,jVal2;
				jVal1["status"] = "success";
				jVal2["Description"] = "service active on "+Port_no+"";
				resultValue["field"].append(jVal1);
				resultValue["field"].append(jVal2);

				output  = Json::writeString(wbuilder, resultValue);
			}
			else
			{
				Json::Value jVal1,jVal2;
				jVal1["status"] = "fail";
				jVal2["Description"] = "service inactive on "+Port_no+"";
				resultValue["field"].append(jVal1);
				resultValue["field"].append(jVal2);

				output  = Json::writeString(wbuilder, resultValue);
			}
    	}
    	else
    	{
    		Json::Value jVal1,jVal2;
			jVal1["status"] = "fail";
			jVal2["Description"] = "service is not running on "+Port_no+"";
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);

			output  = Json::writeString(wbuilder, resultValue);	
    	}
    }
    else
    {
    	Json::Value jVal1,jVal2;
		jVal1["status"] = "fail";
		jVal2["Description"] = "invalid port number";
		resultValue["field"].append(jVal1);
		resultValue["field"].append(jVal2);

		output  = Json::writeString(wbuilder, resultValue);	
    }
	return output;
}


/*----------------------------------------------------------------------------------------
This function gets called when the PlcDataloginStatus API was invoked
This function handles error responses in the follwing cases
	1. When the unidentified errors occured
This function will be used to check the after changing port for PlcDataloginStatus service working properly. 
-------------------------------------------------------------------------------------------*/

string PlcDataloginStatus(const char* utc_time)
{  
	std::string output;
	std::string pdt = utc_time;
	Json::StreamWriterBuilder wbuilder;
	Json::Value resultValue;

	ifstream aFile ("/etc/plcdatacollector/plcdatacollector.conf");   //for devel system

    std::size_t lines_count =0;
    std::string line,replaceStr;
    while (std::getline(aFile , line))
    {
       	++lines_count;
        if(lines_count == 18){
            replaceStr = line;
       	 }
    }        

    //feetching port no from the sentence
	string tmp;
	string port;
    stringstream sstream_word(replaceStr);
    vector<string> words;
    while(sstream_word >> tmp){
    	words.push_back(tmp);
    }
    for(int i = 0; i < words.size(); i++){
        if(i == 2){
            port = words.at(i);
            break;
        }
    }

	std::string strConnection = std::string("dbname = "+db_database+" user = "+db_uname+" password = "+db_upass+" hostaddr = "+host_addr+" port = "+db_port+"");
	//std::string strConnection = std::string("dbname = plc_data user = postgres password = postgres hostaddr = 127.0.0.1 port = 5432");
	try{
		connection C(strConnection);
		if (C.is_open())
		{
			cout << "Opened database successfully " << C.dbname() << endl;
		} 
		else
		{
			cout << "Can't open database" << endl;
			Json::Value jVal1, jVal2;	
			jVal1["result"] = "Fail";
			jVal2["Description"] = "Could not open database";
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);
			output  = Json::writeString(wbuilder, resultValue);  
			return output;
		} 
		/*string pdt;
		time_t now = time(NULL);
		tm *gmtm1 = localtime(&now);
		char buffer1[80];
		strftime (buffer1,80,"%Y-%m-%d %H:%M:%S",gmtm1);
		pdt = convertToString(buffer1);
		const char* currtime = pdt.c_str();
		cout<<"pdt =="<<pdt<<endl;*/

		//std::string utctime = getutcfromlocalusingdb(currtime);

		//std::string qryselect = "SELECT count(*) from data_view where insert_time_stamp >((SELECT TIMESTAMP '"+pdt+"'  AT TIME ZONE current_setting('TimeZone') AT TIME ZONE 'UTC'))";
		std::string qryselect = "SELECT count(*) from data_view where insert_time_stamp >('"+pdt+"')";

		const char* sql = qryselect.c_str();
		cout<<qryselect<<endl;
		nontransaction N(C);
		int t =2;
		float time_in_sec;
		std::string qry[7];
		int cnt[7];

		for(int i=0; i< 7; i++)
		{
			//qry[i]= "SELECT count(*) from data_view where insert_time_stamp >((SELECT TIMESTAMP '"+pdt+"'  AT TIME ZONE current_setting('TimeZone') AT TIME ZONE 'UTC'))";
			qry[i]="SELECT count(*) from data_view where insert_time_stamp >('"+pdt+"')";
			result R( N.exec( qry[i].c_str() ));
			for (result::const_iterator c = R.begin(); c != R.end(); ++c)
			{
				cnt[i] = c[0].as<int>(); 
			}
			cout<<"cnt"<<i<<"="<<cnt[i]<<endl;
			if(cnt[0]<cnt[i])
			{
				std::string qrysel1 = "SELECT EXTRACT(EPOCH FROM ((select insert_time_stamp from data_view order by insert_time_stamp desc limit 1) - (select insert_time_stamp from data_view order by insert_time_stamp desc OFFSET 100 limit 1) ))FROM data_view limit 1";
				const char* sql1 = qrysel1.c_str();
				result R1 ( N.exec( sql1 ));
				N.commit();
				for (result::const_iterator c = R1.begin(); c != R1.end(); ++c)
				{
					time_in_sec = c[0].as<float>(); 
				}
				cout<<"time_in_sec =="<<time_in_sec<<endl;
				int freq=((round(time_in_sec)*1000)/100);
				cout<<"freq== "<<freq<<endl;
				Json::Value jVal1,jVal2;
				jVal1["result"] = "success";
				jVal2["Description"] = "Data is logging "+to_string(freq)+" miliseconds per packet into data_table on DATA_PORT = "+port+"";				resultValue["field"].append(jVal1);
				resultValue["field"].append(jVal2);
				C.disconnect();	
				break;		
			}
			sleep(t);
		}
		if(cnt[0]==cnt[5]){
			Json::Value jVal1,jVal2;
			jVal1["result"] = "failed";
			jVal2["Description"] = "Data is not logging into data_table, waiting on DATA_PORT = "+port+"";
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);							
			output  = Json::writeString(wbuilder, resultValue);	
			N.commit();
			C.disconnect();
			return output;
		}
	}
	catch(const std::exception &e){
		Json::Value jVal1,jVal2,jVal3;
		jVal1["result"] = "failed";
		jVal2["Description"] = "DB Error";	
		resultValue["field"].append(jVal1);
		resultValue["field"].append(jVal2);
		//resultValue["field"].append(jVal3);
		output  = Json::writeString(wbuilder, resultValue); 
		return output;
	}
	output = Json::writeString(wbuilder, resultValue);	
	return output; 
}


/*----------------------------------------------------------------------------------------
This function gets called when the checkDataloggerAPIservice API was invoked
This function handles error responses in the follwing cases
	1. When the unidentified errors occured
This function will be used to check the after changing port for dataloggerapi service working properly. 
-------------------------------------------------------------------------------------------*/
string AlarmDataloginStatus(const char* utc_time)
{
	std::string output;
	std::string pdt = utc_time;
	Json::StreamWriterBuilder wbuilder;
	Json::Value resultValue;

	//finding ref line for alarmport
    ifstream fl ("/etc/plcdatacollector/plcdatacollector.conf"); //server
    std::size_t l_cnt =0;
    std::string line_num;
   	int AlarmNo=0;
    while (std::getline(fl ,line_num)){
        ++l_cnt;
       	if(line_num == "#Remote ALARM_PORT to listen on")
       	{
          	AlarmNo = l_cnt;
       	}
    }
    int line_no= AlarmNo+1;

    //by getting line_no i will get next lines sentence to fetch port no
	ifstream aFile ("/etc/plcdatacollector/plcdatacollector.conf");   //server
    std::size_t lines_count =0;
    std::string line,replaceStr;
    while (std::getline(aFile , line)){
        ++lines_count;
	    if(lines_count == line_no){
	        replaceStr = line;
	    }
    }           
    string sentence = replaceStr; 
     
    //feetching port no from the sentence
	string tmp;
	string port;
    stringstream sstream_word(replaceStr);
    vector<string> words;
    while(sstream_word >> tmp){
    	words.push_back(tmp);
    }
    for(int i = 0; i < words.size(); i++)
    {
        if(i == 2)
        {
            port = words.at(i);
            break;
        }
    }
	std::string strConnection = std::string("dbname = "+db_database+" user = "+db_uname+" password = "+db_upass+" hostaddr = "+host_addr+" port = "+db_port+"");
	//std::string strConnection = std::string("dbname = plc_data user = postgres password = postgres hostaddr = 127.0.0.1 port = 5432");
	try{
		connection C(strConnection);
		if (C.is_open())
		{
			cout << "Opened database successfully " << C.dbname() << endl;
		} 
		else
		{
			cout << "Can't open database" << endl;
			Json::Value jVal1, jVal2;	
			jVal1["result"] = "Fail";
			jVal2["Description"] = "Could not open database";
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);
			output  = Json::writeString(wbuilder, resultValue);  
			return output;
		} 
		/*string pdt;
		time_t now = time(NULL);
		tm *gmtm1 = localtime(&now);
		char buffer1[80];
		strftime (buffer1,80,"%Y-%m-%d %H:%M:%S",gmtm1);
		pdt = convertToString(buffer1);
		const char* currtime = pdt.c_str();
		cout<<"pdt =="<<pdt<<endl;*/

		//std::string qryselect = "SELECT count(*) from alarm_view where insert_time_stamp >((SELECT TIMESTAMP '"+pdt+"'  AT TIME ZONE current_setting('TimeZone') AT TIME ZONE 'UTC'))";
		std::string qryselect = "SELECT count(*) from alarm_view where insert_time_stamp >('"+pdt+"')";
		const char* sql = qryselect.c_str();
		cout<<qryselect<<endl;

		nontransaction N(C);
		float t =2;
		float time_in_sec;
		std::string qry[7];
		int cnt[7];
		for(int i=0; i< 7; i++)
		{
			//qry[i]= "SELECT count(*) from alarm_view where insert_time_stamp >((SELECT TIMESTAMP '"+pdt+"'  AT TIME ZONE current_setting('TimeZone') AT TIME ZONE 'UTC'))";
			qry[i]="SELECT count(*) from alarm_view where insert_time_stamp >('"+pdt+"')";
			result R( N.exec( qry[i].c_str() ));
			for (result::const_iterator c = R.begin(); c != R.end(); ++c)
			{
				cnt[i] = c[0].as<int>(); 
			}
			cout<<"cnt"<<i<<"="<<cnt[i]<<endl;
			if(cnt[0]<cnt[i])
			{
				std::string qrysel1 = "SELECT EXTRACT(EPOCH FROM ((select insert_time_stamp from alarm_table order by insert_time_stamp desc limit 1) - (select insert_time_stamp from alarm_table order by insert_time_stamp desc OFFSET 100 limit 1) ))FROM alarm_table limit 1";
				const char* sql1 = qrysel1.c_str();
				result R1 ( N.exec( sql1 ));
				N.commit();
				for (result::const_iterator c = R1.begin(); c != R1.end(); ++c)
				{
					time_in_sec = c[0].as<float>(); 
				}
				int freq=((round(time_in_sec)*1000)/100);

				Json::Value jVal1,jVal2;
				jVal1["result"] = "success";
				jVal2["Description"] = "Alarm Data is logging "+to_string(freq)+" miliseconds per packet into alarm_table on Alarm_PORT = "+port+"";
				resultValue["field"].append(jVal1);
				resultValue["field"].append(jVal2);
				C.disconnect();
				break; 		
			}
			sleep(t);
		}
		if(cnt[0]==cnt[5])
		{
			Json::Value jVal1,jVal2;
			jVal1["result"] = "failed";
			jVal2["Description"] = "Data is not logging into alarm_table, waiting on ALARM_PORT = "+port+"";
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);
							
			output  = Json::writeString(wbuilder, resultValue);	

			N.commit();
			C.disconnect();
			return output;
		}

	}
	catch(const std::exception &e)
	{
		Json::Value jVal1,jVal2,jVal3;
		jVal1["result"] = "failed";
		jVal2["Description"] = "DB Error";	
		resultValue["field"].append(jVal1);
		resultValue["field"].append(jVal2);
		//resultValue["field"].append(jVal3);
		output  = Json::writeString(wbuilder, resultValue); 
		cout << output << endl;
		return output;
	}
	output = Json::writeString(wbuilder, resultValue);	
	return output;
}

/*
----------------------------------------------------------------------------------------
This function gets called when the process_awlfile API was invoked
This function handles error responses in the follwing cases
	1. When the database connection failed
	2. When the unidentified errors occured
This function will be used to return extract data from data_table with unpacked parameters between the given start time and end time as JOSN format.
-------------------------------------------------------------------------------------------
*/

string process_awlfile(std::vector<std::string> iplinearr)
{
	/*for(int i=0; i<iplinearr.size(); i++){
		cout<< iplinearr[i] << endl;
	}*/
	//cout<<"inside aawl file" << endl;

	Json::StreamWriterBuilder wbuilder;
	Json::Value resultValue;
	std::string output;

	int iLevel=0;
	string strLine;
	string strDataFieldName, strBaseFieldName, strPackedFieldBaseName;
	int bitNumber = 1;
	
	std::vector<std::string> dataFieldName;
	std::vector<std::string> dataFieldType;
	
	std::vector<std::string> fdParamName;
	std::vector<std::string> fdParamDataType;
	std::vector<std::string> fdParamFieldName;
	std::vector<std::string> fdBitNumber;
	std::vector<std::string> fdBitMask;
	std::vector<std::string> fdParamDesc;
	std::vector<std::string> fdByteIndex;
	std::vector<std::string> fdFieldSize;
	
	string strFileIn, strHost, strPort, strUser, strPwd, strDB;
	bool bSkip = false;
	int iByteIndex = 0;
	int iLastFieldIndex=0;

	map<string, int> numberOfBytes {
		{"Byte", 1}, {"BYTE", 1}, {"Int", 2}, {"INT",2}, {"Long", 4}, {"LONG",4}, {"Real",4}, {"REAL",4}, {"UInt",2}, {"UINT",2}
	};
	map<string, string> fieldTypes {
		{"Byte", "SMALLINT"}, {"BYTE", "SMALLINT"},
		{"Int" , "SMALLINT"}, {"INT","SMALLINT"},
		{"Long", "INT"}, {"LONG","INT"}, 
		{"Real", "REAL"}, {"REAL","REAL"},
		{"UInt", "SMALLINT"}, {"UINT","SMALLINT"}
	};

	//Set the defaults for the cammaond line parameters
	strFileIn=std::string("");
	strHost = std::string("127.0.0.1");
	strPort = std::string("5432");
	strUser = std::string("postgres");
	strPwd = std::string("postgres");
	strDB = std::string("plc_data");
	int index1;
	int index2;
	int iplineCnt = iplinearr.size();
	for(int i=0; i<iplineCnt; i++){
		strLine = iplinearr[i];
		//cout<<"strLine = "<<strLine<<endl;
		strLine.erase(remove(strLine.begin(), strLine.end(),'\t'), strLine.end());
		strLine.erase(remove(strLine.begin(), strLine.end(), ';'), strLine.end());
		splitLine(strLine);
			//cout << strName << " | " << strKey << " | " << strComment << endl;
		switch(iLevel){	
			case 0:
				if (strName == "STRUCT")
					iLevel++;
				else if(strName == "END_STRUCT")
					iLevel--;
				break;
				
			case 1:
				if((strKey == "STRUCT") || (strKey == "Struct") ){ 
					strBaseFieldName = "";
					iLevel++;
				}
				else if(strName == "END_STRUCT"){
					iLevel--;
				}
				break;
			case 2:
				if( (strKey == "END_STRUCT") || (strName == "END_STRUCT") ){
					iLevel--;
					bitNumber=1;
				}
				else if( (strKey == "STRUCT") || (strKey == "Struct") ){
					strBaseFieldName = strName;
					strDataFieldName = strBaseFieldName;
					
					strPackedFieldBaseName = strDataFieldName;
					bitNumber=1;
					iLevel++;
				}
				else if(strKey == "Array"){
					dataFieldName.push_back(strName);
					dataFieldType.push_back("VARCHAR("+strArraySize+")");

							//A record in the field Defs table is required
					fdParamName.push_back(strName);
					fdParamDataType.push_back("Array");
					fdParamDesc.push_back(strComment);
					fdParamFieldName.push_back(strName);
					fdBitNumber.push_back(std::to_string(0));
					fdBitMask.push_back(std::to_string(0));
					fdByteIndex.push_back(std::to_string(iByteIndex));
					fdFieldSize.push_back(strArraySize);
					iByteIndex += std::stoi(strArraySize);
					bitNumber=1;
				}
				else{
					strDataFieldName =  strName;

					dataFieldName.push_back(strDataFieldName);
					dataFieldType.push_back(strKey);

					//A record in the field Defs table is required
					fdParamName.push_back(strDataFieldName);
					fdParamDataType.push_back(strKey);
					fdParamDesc.push_back(strComment);
					fdParamFieldName.push_back(strDataFieldName);
					fdBitNumber.push_back(std::to_string(0));
					fdBitMask.push_back(std::to_string(0));
					fdByteIndex.push_back(std::to_string(iByteIndex));
					fdFieldSize.push_back(std::to_string(numberOfBytes[strKey]));
					iByteIndex += numberOfBytes[strKey];
				}
				break;
			case 3:
				if(strName == "END_STRUCT"){
					iLevel--;
					bitNumber=1;
				}
				else if( (strKey == "STRUCT") || (strKey == "Struct") ){
					strDataFieldName = strBaseFieldName + "_" +  strName;					
					strPackedFieldBaseName = strDataFieldName;
					bitNumber=1;
					iLevel++;

				}
				else if(strKey == "Array"){
					dataFieldName.push_back(strDataFieldName + "_" + strName);
					dataFieldType.push_back("VARCHAR("+strArraySize+")");

							//A record in the field Defs table is required
					fdParamName.push_back(strDataFieldName + "_" + strName);
					fdParamDataType.push_back("Array");
					fdParamDesc.push_back(strComment);
					fdParamFieldName.push_back(strDataFieldName + "_" + strName);
					fdBitNumber.push_back(std::to_string(0));
					fdBitMask.push_back(std::to_string(0));
					fdByteIndex.push_back(std::to_string(iByteIndex));
					fdFieldSize.push_back(strArraySize);
					iByteIndex += std::stoi(strArraySize);
					bitNumber=1;
				}
				else if( (strKey == "BOOL") || (strKey == "Bool") ){
							//Field type to be created in the data table - only on the first bool 
					if(bitNumber%8 == 1){
						dataFieldName.push_back(strBaseFieldName + std::to_string(bitNumber / 8));
						dataFieldType.push_back("Byte"); //
						index1 = bitNumber /8;
						//A record in the field Defs table is also required
						fdParamName.push_back(strBaseFieldName + std::to_string(bitNumber / 8));
						fdParamDataType.push_back("Packed");
						fdParamDesc.push_back(strComment);
						fdParamFieldName.push_back(strBaseFieldName + std::to_string(bitNumber / 8));
						fdBitNumber.push_back(std::to_string(0));
						fdBitMask.push_back(std::to_string(1));
						fdByteIndex.push_back(std::to_string(iByteIndex));					// iByteCount will be incremented on exit from this level
						fdFieldSize.push_back(std::to_string(0));							// Will get edited on exiting from this level 
						iByteIndex += 1; 
					}
							
					//A record in the field Defs table is required
					fdParamName.push_back(strPackedFieldBaseName + "__" + strName);
					fdParamDataType.push_back("BOOL");
					fdParamDesc.push_back(strComment);
					fdParamFieldName.push_back(strPackedFieldBaseName+ to_string(index1));
					fdBitNumber.push_back(std::to_string(bitNumber%8));
					fdBitMask.push_back(std::to_string(0x01<<((bitNumber-1) % 8)));
					fdByteIndex.push_back(std::to_string(iByteIndex));						// iByteCount remains the same
					fdFieldSize.push_back(std::to_string(0));
					bitNumber++;
				}
				else{
					strDataFieldName = strBaseFieldName + "_" +  strName;
							
					dataFieldName.push_back(strDataFieldName);
					dataFieldType.push_back(strKey);

					//A record in the field Defs table is required
					fdParamName.push_back(strDataFieldName);
					fdParamDataType.push_back(strKey);
					fdParamDesc.push_back(strComment);
					fdParamFieldName.push_back(strDataFieldName);
					fdBitNumber.push_back(std::to_string(0));
					fdBitMask.push_back(std::to_string(0));
					fdByteIndex.push_back(std::to_string(iByteIndex));
					fdFieldSize.push_back(std::to_string(numberOfBytes[strKey]));
					iByteIndex += numberOfBytes[strKey];
				}
				break;
			case 4:
				if(strName == "END_STRUCT"){
					iLevel--;
					bitNumber=1;
				}
				else if((strKey == "Byte") || (strKey == "BYTE") || 
						(strKey == "INT") || (strKey == "Int") || 
						(strKey == "Real") || (strKey == "REAL") ||
						(strKey == "UInt") || (strKey == "UINT") ){
					dataFieldName.push_back(strDataFieldName + "_" + strName);
					dataFieldType.push_back(strKey);

						//A record in the field Defs table is required
					fdParamName.push_back(strDataFieldName + "_" + strName);
					fdParamDataType.push_back(strKey);
					fdParamDesc.push_back(strComment);
					fdParamFieldName.push_back(strDataFieldName + "_" + strName);
					fdBitNumber.push_back(std::to_string(0));
					fdBitMask.push_back(std::to_string(0));
					fdByteIndex.push_back(std::to_string(iByteIndex));
					fdFieldSize.push_back(std::to_string(numberOfBytes[strKey]));
					iByteIndex += numberOfBytes[strKey];
					bitNumber=1;
				}
				else if(strKey == "Array"){
					dataFieldName.push_back(strDataFieldName + "_" + strName);
					dataFieldType.push_back("VARCHAR("+strArraySize+")");

						//A record in the field Defs table is required
					fdParamName.push_back(strDataFieldName + "_" + strName);
					fdParamDataType.push_back("Array");
					fdParamDesc.push_back(strComment);
					fdParamFieldName.push_back(strDataFieldName + "_" + strName);
					fdBitNumber.push_back(std::to_string(0));
					fdBitMask.push_back(std::to_string(0));
					fdByteIndex.push_back(std::to_string(iByteIndex));
					fdFieldSize.push_back(strArraySize);
					iByteIndex += std::stoi(strArraySize);
					bitNumber=1;
				}
				else{
						//Field type to be created in the data table - only on the first bool 
					if(bitNumber%16 == 1){
						dataFieldName.push_back(strDataFieldName+std::to_string(bitNumber / 16));
						dataFieldType.push_back("Byte");
						index2 = bitNumber / 8;
							//A record in the field Defs table is also required
						fdParamName.push_back(strDataFieldName+std::to_string(bitNumber / 16));
						fdParamDataType.push_back("Packed");
						fdParamDesc.push_back(strComment);
						fdParamFieldName.push_back(strDataFieldName+std::to_string(bitNumber / 16));
						fdBitNumber.push_back(std::to_string(0));
						fdBitMask.push_back(std::to_string(1));
							fdByteIndex.push_back(std::to_string(iByteIndex));					// iByteCount will be incremented on exit from this level
							fdFieldSize.push_back(std::to_string(0));							// Will get edited on exiting from this level 
							iByteIndex += 1;
						}
						
						//A record in the field Defs table is required
						fdParamName.push_back(strPackedFieldBaseName + "__" + strName);
						fdParamDataType.push_back("BOOL");
						fdParamDesc.push_back(strComment);
						fdParamFieldName.push_back(strPackedFieldBaseName + to_string(index2));
						fdBitNumber.push_back(std::to_string(bitNumber%8));
						fdBitMask.push_back(std::to_string(0x01<<((bitNumber-1) % 8)));
						fdByteIndex.push_back(std::to_string(iByteIndex));						// iByteCount remains the same
						fdFieldSize.push_back(std::to_string(0));
						bitNumber++;
				}
				break;
			default:
			//cout << "Default case name:" << strName << ", key" << strKey << "\n"; 
			break;
		}
	}	
	// The databse processing starts here	
	//std::string strConnection = std::string("dbname = ")+DBNAME+" user = "+DBUSER+" password = "+DBPWD+" hostaddr = "+DBHOST+" port = " + DBPORT;
	std::string strConnection = std::string("dbname = ")+strDB+" user = "+strUser+" password = "+strPwd+" hostaddr = "+strHost+" port = " + strPort;
	string qryCreate=std::string("");
	string qryInsert=std::string("");		

	try {
		std::string qery1 = "sudo service plcdatacollector stop";
		system(qery1.c_str());

		connection C(strConnection);
		if (!C.is_open()) 
		{
			cout << "Can't open database" << endl;

			Json::Value jVal1,jVal2,jVal3,jVal4;
			jVal1["result"] = "failed";
			jVal2["Description_af_create"] = "Could not open database";	
			jVal3["Description_af_update"] = "DB Error";
			jVal4["Description_at_create"] = "DB Error";
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);
			resultValue["field"].append(jVal3);
			resultValue["field"].append(jVal4);
			output  = Json::writeString(wbuilder, resultValue); 
			return output;		
		}
		
		qryCreate=std::string("DROP VIEW IF EXISTS data_view;\n");
		work W(C);
		W.exec( qryCreate );		
		//cout << "View data_view dropped\n";

		qryCreate=std::string("ALTER TABLE IF EXISTS data_table RENAME TO data_table_")+today_date()+"_"+now_time()+";";
		//cout << qryCreate << endl;
		W.exec( qryCreate );		
		//cout << "Table data_table renamed\n";

		qryCreate=std::string("ALTER TABLE IF EXISTS field_defs RENAME TO field_defs_")+today_date()+"_"+now_time()+";";
		//cout << qryCreate << endl;
		W.exec( qryCreate );		
		//cout << "Table data_table renamed\n";

		//Remove the data_table 
		/*qryCreate=std::string("DROP TABLE IF EXISTS data_table cascade;\n");
		//work W(C);
		W.exec( qryCreate );		
		cout << "Table data_table dropped\n";
		
		//Remove the field_defs table
		qryCreate=std::string("DROP TABLE IF EXISTS field_defs cascade;\n");
		W.exec( qryCreate );		
		cout << "Table field_defs dropped\n";*/
		
		qryCreate = std::string("");
		//Create the data_table
		qryCreate = "CREATE TABLE data_table(\n";
		qryCreate += "insert_time_stamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP(2),\n";
		qryCreate += "sample_number BIGSERIAL,\n";
		qryCreate += "source_ip VARCHAR(20),\n";
		qryCreate += "source_port VARCHAR(10),\n";
		qryCreate += "caller_id VARCHAR(20),\n";

		string strDBFieldType=std::string("");
		for(int i=0; i < dataFieldType.size(); i++)
		{
			strDBFieldType = fieldTypes[dataFieldType[i]] != "" ? fieldTypes[dataFieldType[i]] : dataFieldType[i];
			qryCreate = qryCreate + "\"" + dataFieldName[i] + "\" " + strDBFieldType + ",\n";
		}
		qryCreate.pop_back();	//Remove the \n
		qryCreate.pop_back();	//Remove the ','

		qryCreate += ");";
		//cout << qryCreate << endl;
		W.exec( qryCreate );
		//cout << "data_table created\n";
		
		//Create the field_defs table
		qryCreate = "CREATE TABLE field_defs(\n";
		qryCreate += "insert_time_stamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP(2),\n";
		qryCreate += "source_ip VARCHAR(20),\n";
		qryCreate += "source_port VARCHAR(10),\n";
		qryCreate += "caller_id VARCHAR(20),\n";
		qryCreate += "parameter_name VARCHAR(50),\n";
		qryCreate += "parameter_data_type VARCHAR(20),\n";
		qryCreate += "field_name VARCHAR(50),\n";
		qryCreate += "bit_number INT DEFAULT 0,\n";
		qryCreate += "off_set INT DEFAULT 0,\n";
		qryCreate += "scaling_factor REAL DEFAULT 1.0,\n";
		qryCreate += "bit_mask INT DEFAULT 0,\n";
		qryCreate += "comment VARCHAR(100),\n";
		qryCreate += "param_serial INT,\n";
		qryCreate += "number_of_bytes INT";

		qryCreate += ");";
		//cout << qryCreate << endl;
		W.exec( qryCreate );
		//cout << "field_defs table created\n";		
		
		//Create view data_view
		qryCreate=std::string("create or replace view data_view as select * from data_table;\n");
		W.exec( qryCreate );		
		cout << "View data_view created\n";         

		//Commit transactions so far
		W.commit();

		//Change the table to a hyertable on the insert_time_stamp
		work Wh(C);
		Wh.exec("SELECT create_hypertable('data_table', 'insert_time_stamp');");
		//cout << "data_table converted to HyperTable\n";
		Wh.commit();
	
		//Insert the field definitions in the field_defs table
		for(int i=0, j=0; i < fdParamName.size(); i++)
		{
			//string qryInsert="INSERT INTO field_defs (insert_time_stamp, source_ip, source_port, caller_id,  parameter_name, parameter_data_type, field_name, bit_number, scaling_factor, bit_mask, comment, param_serial) VALUES\n";
			qryInsert += "INSERT INTO field_defs (parameter_name, parameter_data_type, field_name, bit_number, scaling_factor, bit_mask, comment, param_serial, number_of_bytes) VALUES\n";
			qryInsert += "    ('" + fdParamName[i] + "', '" + fdParamDataType[i] + "', '" + fdParamFieldName[i] + "', '" + fdBitNumber[i] + "', 1.0, " + fdBitMask[i] + ", '" + fdParamDesc[i] + "', " + fdByteIndex[i] +", " + fdFieldSize[i] + ");\n" ;
			j = j + numberOfBytes[fdParamDataType[i]];
		}
		//cout << qryInsert << "\n";
		
		work Wd(C);
		Wd.exec(qryInsert);
		//cout << "field_defs table updated\n";

		/*
		//execute queries fron below file 
		//ifstream aFile ("/home/amol/devel/setup/field_update_qry.sql");
		ifstream aFile ("/home/devel/set_up/field_update_qry.sql");
		std::string line;
		std::string QUERY = "";
      	while (std::getline(aFile , line))
      	{
        	QUERY += line;
       	} 
       	//cout<<"QUERY == "<<QUERY<<endl;
       	Wd.exec(QUERY); 
       	*/
       	Wd.commit();

		Json::Value jVal1,jVal2,jVal3,jVal4;
		jVal1["result"] = "success";
		jVal2["Description_af_create"] = "field_defs table created successfully";
		jVal3["Description_af_update"] = "field_defs table updated successfully";
		jVal4["Description_at_create"] = "data_table created successfully";
			
		resultValue["field"].append(jVal1);
		resultValue["field"].append(jVal2);
		resultValue["field"].append(jVal3);
		resultValue["field"].append(jVal4);
		
		std::string qery = "sudo service plcdatacollector start";
		system(qery.c_str());

		output  = Json::writeString(wbuilder, resultValue);
		return output;
	} 
	catch (const std::exception &e) 
	{
		cerr << e.what() << std::endl;
		//cout << "==========================================" << endl;
		//cout << qryCreate << endl;
		//cout << "==========================================" << endl;
		//cout << qryInsert << endl;
		
	    Json::Value jVal1,jVal2,jVal3,jVal4;
		jVal1["result"] = "failed";
		jVal2["Description_af_create"] = "DB Error/exception occured";	
		jVal3["Description_af_update"] = "service stopped,unable to change confugration";
		jVal4["Description_at_create"] = e.what();
		resultValue["field"].append(jVal1);
		resultValue["field"].append(jVal2);
		resultValue["field"].append(jVal3);
		resultValue["field"].append(jVal4);
		output  = Json::writeString(wbuilder, resultValue); 
		return output;
		
	}	
}


/*----------------------------------------------------------------------------------------
This function gets called when the Alarm_Confugration API was invoked
This function handles error responses in the follwing cases
	1. When the unidentified errors occured
-------------------------------------------------------------------------------------------*/

string Alarm_Confugration(const char *  AlarmCounts)
{
	Json::StreamWriterBuilder wbuilder;
	Json::Value resultValue;
	std::string output;
	std::string Alarm_count = AlarmCounts;
	int alarm_counts = stoi(Alarm_count);

 	string strHost, strPort, strUser, strPwd, strDB;
  	//Set the defaults for the cammand line parameters
 	strHost = std::string("127.0.0.1");
 	strPort = std::string("5432");
 	strUser = std::string("postgres");
  	strPwd = std::string("postgres");
  	strDB = std::string("plc_data");

  	std::string strConnection = std::string("dbname = ")+strDB+" user = "+strUser+" password = "+strPwd+" hostaddr = "+strHost+" port = " + strPort;
  	string qryCreate = std::string("");
  	string qryInsert = std::string("");
  
  	try
  	{
    	connection C(strConnection);
    	if(!C.is_open())
    	{
      		cout << "Can't open database" << endl;

			Json::Value jVal1,jVal2,jVal3,jVal4;
			jVal1["result"] = "failed";
			jVal2["Description_af_create"] = "Could not open database";	
			jVal3["Description_af_update"] = "DB Error";
			jVal4["Description_at_create"] = "DB Error";	
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);
			resultValue["field"].append(jVal3);
			resultValue["field"].append(jVal4);
			output  = Json::writeString(wbuilder, resultValue); 
			return output;    
    	}
    	std::string qery1 = "sudo service alarmdatacollector stop";
		system(qery1.c_str());

	    //Remove the alarm_defs 
	   /* qryCreate=std::string("DROP TABLE IF EXISTS alarm_defs;\n");
	    work W(C);
	    W.exec( qryCreate );    
	    cout << "Table alarm_defs dropped\n";*/

	    qryCreate=std::string("DROP VIEW IF EXISTS alarm_view;\n");
		work W(C);
		W.exec( qryCreate );		
		//cout << "View alarm_view dropped\n";
		qryCreate=std::string("ALTER TABLE IF EXISTS alarm_defs RENAME TO alarm_defs_")+today_date()+"_"+now_time()+";";
		//cout << qryCreate << endl;
		W.exec( qryCreate );		
		//cout << "Table alarm_defs renamed\n";
	    

	    qryCreate = std::string("");
	    //Create the alarm_defs
	    qryCreate = "CREATE TABLE alarm_defs(\n";
	    qryCreate += "alarm_id INT DEFAULT 0,\n";
	    qryCreate += "db_field_name VARCHAR(20),";
	    qryCreate += "bit_number INT DEFAULT 0,";
	    qryCreate += "field_type VARCHAR(5))";
	    //cout << "Query" << qryCreate;
	    W.exec( qryCreate );  
	    W.commit();

	    qryInsert = std::string("");
	    //Create the alarm_defs
	    qryInsert = "INSERT INTO alarm_defs(alarm_id,db_field_name,bit_number,field_type) values ";
	    
	    int str2Counter = 1;
	    int fnCounter = 1;
	    int bitCounter = 0;
	    string dbfn = "";
	    for(int i=1;i<=alarm_counts;i++)
	    {
	      	std::string str1 = "Alarm";
	     	std::string str2 = "Int";
	     	int counter = 16;
	      	str1 = str1 + std::to_string(i);
	      	//cout << str1 << "\n";
	      	//cout << "bitCounter" << bitCounter << "\n";
	      	//cout << "Packed Bool Int" << fnCounter << "\n";
	      	dbfn = "Packed Bool Int" + std::to_string(fnCounter);
	      	qryInsert += "('"+std::to_string(i)+"','"+dbfn+"',"+std::to_string(bitCounter)+",'BOOL'),";
	      	if(i%counter == 0)
	      	{
	        	bitCounter = 0;
	        	fnCounter = fnCounter + 1;
	      	}
	      	else
	      	{
	        	bitCounter++;
	      	}
	    }    
	    qryInsert.pop_back(); //Remove the ','
	    //cout << "Query" << qryInsert << endl;
	    
	    work N(C);
	    N.exec( qryInsert );
	    //Commit transactions so far
	    N.commit();
	    
	    cout << "alarm_defs table updated\n";
  	} 
  	catch (const std::exception &e) 
  	{
	    cerr << e.what() << std::endl;

	    Json::Value jVal1,jVal2,jVal3,jVal4;
		jVal1["result"] = "failed";
		jVal2["Description_af_create"] = "DB Error";	
		jVal3["Description_af_update"] = "exception occured";
		jVal4["Description_at_create"] = e.what();
		resultValue["field"].append(jVal1);
		resultValue["field"].append(jVal2);
		resultValue["field"].append(jVal3);
		resultValue["field"].append(jVal4);
		output  = Json::writeString(wbuilder, resultValue); 
		return output; 
 	}

  	string qrySelect = std::string("");
  	const char * sql;
  	string fieldName;
  	std::vector<std::string> arrayFieldName;
  	qrySelect += "select db_field_name from alarm_defs group by db_field_name order by LENGTH(db_field_name),db_field_name";
  	sql = qrySelect.c_str();
  	std::string strConnection1 = std::string("dbname = ")+strDB+" user = "+strUser+" password = "+strPwd+" hostaddr = "+strHost+" port = " + strPort;
  	try 
  	{
    	connection C(strConnection1);
    	if (!C.is_open()) 
    	{
    		cout << "Can't open database" << endl;

		    Json::Value jVal1,jVal2,jVal3,jVal4;
			jVal1["result"] = "failed";
			jVal2["Description_af_create"] = "Could not open database";	
			jVal3["Description_af_update"] = "DB Error";
			jVal4["Description_at_create"] = "DB Error";	
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);
			resultValue["field"].append(jVal3);
			resultValue["field"].append(jVal4);
			output  = Json::writeString(wbuilder, resultValue); 
			return output;
   		} 
   		
	    //* Create a non-transactional object. 
	    work N(C);

	    //* Execute SQL query 
	    result R( N.exec( sql ));

	    //* List down all the records 
	    for (result::const_iterator c = R.begin(); c != R.end(); ++c) 
	    {

	      	fieldName = c[0].as<string>();
	      	arrayFieldName.push_back(fieldName);
	    }
	    N.commit();
	    //Remove the alarm_table table
	  /*  qryCreate=std::string("DROP TABLE IF EXISTS alarm_table;\n");
	    work W(C);
	    W.exec( qryCreate );    
	    cout << "Table alarm_table dropped\n";
	    W.commit();*/

	    qryCreate=std::string("DROP VIEW IF EXISTS alarm_view;\n");
		work W(C);
		W.exec( qryCreate );		
		//cout << "View alarm_view dropped\n";
		qryCreate=std::string("ALTER TABLE IF EXISTS alarm_table RENAME TO alarm_table_")+today_date()+"_"+now_time()+";";
		//cout << qryCreate << endl;
		W.exec( qryCreate );		
		//cout << "Table alarm_table_ renamed\n";
		W.commit();

	    qryCreate = std::string("");
	    //Create the data_table
	    work WC(C);
	    qryCreate = "CREATE TABLE alarm_table(\n";
	    qryCreate += "insert_time_stamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP(2),\n";

	    string strDBFieldType=std::string("");
	    for(int i=0; i < arrayFieldName.size(); i++)
	    {
	      qryCreate = qryCreate + "\"" + arrayFieldName[i] + "\" INT ,\n";
	    }
	    qryCreate.pop_back(); //Remove the \n
	    qryCreate.pop_back(); //Remove the ','

	    qryCreate += ");";
	    //cout << qryCreate << endl;
	    WC.exec( qryCreate );
	    WC.commit();
	    cout << "alarm_table created\n";
	    //Change the table to a hyertable on the insert_time_stamp
	    work Wh(C);
	    Wh.exec("SELECT create_hypertable('alarm_table', 'insert_time_stamp');");
	    cout << "data_table converted to HyperTable\n";

	    //Create view alarm_view
		qryCreate=std::string("create or replace view alarm_view as select * from alarm_table;\n");
		Wh.exec( qryCreate );		
		cout << "View alarm_view created\n";

	    Wh.commit();
	    cout << endl << "Disconnecting from DB...\n";
	    C.disconnect();

    	Json::Value jVal1,jVal2,jVal3,jVal4;
		jVal1["result"] = "success";
		jVal2["Description_af_create"] ="alarm_defs table created successfully";
		jVal3["Description_af_update"] ="alarm_defs table updated successfully";
		jVal4["Description_at_create"] ="alarm_table created successfully";
		resultValue["field"].append(jVal1);
		resultValue["field"].append(jVal2);
		resultValue["field"].append(jVal3);
		resultValue["field"].append(jVal4);

		std::string qery2 = "sudo service alarmdatacollector start";
		system(qery2.c_str());
		
		output  = Json::writeString(wbuilder, resultValue);
		return output;    
 	} 
  	catch (const std::exception &e) 
  	{
	    cerr << e.what() << std::endl;
	    
	    Json::Value jVal1,jVal2,jVal3,jVal4;
		jVal1["result"] = "failed";
		jVal2["Description_af_create"] = "DB Error";	
		jVal3["Description_af_update"] = "exception occured";
		jVal4["Description_at_create"] =e.what();
		resultValue["field"].append(jVal1);
		resultValue["field"].append(jVal2);
		resultValue["field"].append(jVal3);
		resultValue["field"].append(jVal4);
		output  = Json::writeString(wbuilder, resultValue); 
		return output;
  	}
}


/*----------------------------------------------------------------------------------------
This function gets called when the showDefaultPorts API was invoked
This function handles error responses in the follwing cases
	1. When the unidentified errors occured
-------------------------------------------------------------------------------------------*/
string showDefaultPorts()
{
	Json::StreamWriterBuilder wbuilder;
	Json::Value resultValue;

  	ifstream aFile ("/etc/plcdatacollector/plcdatacollector.conf");			//server  
  	std::size_t lines_count =0;
   	std::string line,replaceStr;
  	while (std::getline(aFile , line))
  	{
    	++lines_count;
    	if(lines_count == 18)
   	 	{
        	replaceStr = line;
   	 	}
   	}           
   	string s = replaceStr; 

  	//feetching port no from the sentence
	string tmp;
	string Data_port;
    stringstream sstream_word(s);
		vector<string> words;
		while(sstream_word >> tmp)
		{
			words.push_back(tmp);
		}
    for(int i = 0; i < words.size(); i++)
    {
            if(i == 2)
            {
            	Data_port = words.at(i);
            }
    }
    //cout<<"Default Data_port == "<<Data_port<<endl;

	//finding ref line for alarmport
	ifstream fl ("/etc/plcdatacollector/plcdatacollector.conf"); 	//server
	std::size_t l_cnt =0;
	std::string line_num;
		int AlarmNo=0;
	while (std::getline(fl ,line_num)){
    	 ++l_cnt;
   		 if(line_num == "#Remote ALARM_PORT to listen on")
   		 {
      	 	AlarmNo = l_cnt;
   		 }
	}

	int line_no= AlarmNo+1;
	//cout<<"line_no == "<<line_no<<endl;

	//by getting line_no i will get next lines sentence to fetch port no
   	ifstream f2 ("/etc/plcdatacollector/plcdatacollector.conf");   //server
   	std::size_t lines_count1 =0;
   	std::string line1,replaceStr1;
   	while (std::getline(f2 , line1)){
    	++lines_count1;
   		if(lines_count1 == line_no)
   		{
       	 	replaceStr1 = line1;
   		}
  	}           
   	string sentence = replaceStr1; 
   	//cout<<"sentence == "<<sentence<<endl;

	//feetching port no from the sentence
	string temp;
	string Alarm_port;
    stringstream sstream_word1(sentence);
	vector<string> words1;
	while(sstream_word1 >> temp)
	{
		words1.push_back(temp);
	}
    for(int i = 0; i < words1.size(); i++)
    {
            if(i == 2)
            {
            	Alarm_port = words1.at(i);
            }
    }
   	// cout<<"Default Alarm_port == "<<Alarm_port<<endl;
    //return Alarm_port;

	ifstream f3 ("/etc/systemd/system/dataloggerapi.service");  	 
	std::size_t count =0;
	std::string line2,replaceStr2;
	while (std::getline(f3 , line2))
	{
		++count;
		if(count == 2)
		{
			replaceStr2 = line2;
	    }
	}         
	string s1 = replaceStr2; 

	string service_port = get_last_word(s1);

   // cout<<"Default service_port == "<<service_port<<endl;

	std::string output;
	if(Data_port!="" && Alarm_port !="" && service_port != "")
	{
		Json::Value jVal1,jVal2,jVal3,jVal4;
		jVal1["status"] = "success";
		jVal2["Data_Port"] = ""+Data_port+"";
		jVal3["Alarm_Port"] = ""+Alarm_port+"";
		jVal4["Service_Port"] = ""+service_port+"";
		resultValue["field"].append(jVal1);
		resultValue["field"].append(jVal2);
		resultValue["field"].append(jVal3);
		resultValue["field"].append(jVal4);

		output  = Json::writeString(wbuilder, resultValue);
	}
	else
	{
		Json::Value jVal1,jVal2,jVal3,jVal4;
		jVal1["status"] = "fail";
		jVal2["Data_Port"] = "not found";
		jVal3["Alarm_Port"] = "not found";
		jVal4["Service_Port"] = "not found";
		resultValue["field"].append(jVal1);
		resultValue["field"].append(jVal2);
		resultValue["field"].append(jVal3);
		resultValue["field"].append(jVal4);

		output  = Json::writeString(wbuilder, resultValue);
	}
	return output;
}

/*----------------------------------------------------------------------------------------
This function gets called when the showDefaultPorts API was invoked
This function handles error responses in the follwing cases
	1. When the unidentified errors occured
-------------------------------------------------------------------------------------------*/
string Default_Alarm_Confugration()
{
	Json::StreamWriterBuilder wbuilder;
	Json::Value resultValue;
	std::string output;

	std::string strConnection = std::string("dbname = "+db_database+" user = "+db_uname+" password = "+db_upass+" hostaddr = "+host_addr+" port = "+db_port+"");
	//std::string strConnection = std::string("dbname = plc_data user = postgres password = postgres hostaddr = 127.0.0.1 port = 5432");
	try
	{
		connection C(strConnection);
		if (C.is_open())
		{
			cout << "Opened database successfully " << C.dbname() << endl;
		} 
		else
		{
			cout << "Can't open database" << endl; 
			Json::Value jVal1, jVal2;	
			jVal1["result"] = "Fail";
			jVal2["Description"] = "Could not open database";
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);
			output  = Json::writeString(wbuilder, resultValue);  
			return output;
		} 

		std::string qrySelect;
		int fields;
		qrySelect="select count(*) from information_schema.columns where table_name='alarm_table'";
		const char* sql = qrySelect.c_str();
		nontransaction N(C);
		result R( N.exec(sql));
			for (result::const_iterator c = R.begin(); c != R.end(); ++c)
			{
				fields = c[0].as<int>(); 
			}    
		N.commit();

		cout<<"fields == "<<fields<<endl;
		if(fields == 41)
		{
			cout<<"inside if"<<endl;
			Json::Value jVal1,jVal2;
			jVal1["result"] = "success";
			jVal2["Description"] = "Alarm_Confugration_640";
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);

			output  = Json::writeString(wbuilder, resultValue);
		}
		else
		{
			Json::Value jVal1,jVal2;
			jVal1["result"] = "success";
			jVal2["Description"] = "Alarm_Confugration_1280";
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);

			output  = Json::writeString(wbuilder, resultValue);
		}
	}
	catch (const std::exception &e) 
  	{
	    cerr << e.what() << std::endl;

		Json::Value jVal1,jVal2;
		jVal1["result"] = "failed";
		jVal2["Description"] = "DB Error";	
		resultValue["field"].append(jVal1);
		resultValue["field"].append(jVal2);
		output  = Json::writeString(wbuilder, resultValue); 
	}
	return output;
}

/*----------------------------------------------------------------------------------------
This function gets called when the get_versions API was invoked
    1. This function will show current versions of all backend files.(version no,relese date,relese comment)
        It will show info of below files:
        a.plcdatacollector.cpp
        b.alarmdatacollector.cpp
        c.server.cpp
        d.kpidata.cpp
        e.kpicollectioncron.cpp
        f.commissioning-server.cpp

This function handles error responses in the follwing cases
    1. When the unidentified errors occured    
    2. versionconfig.conf file should be there in /etc/versionInfo/ otherwise it will show error
-------------------------------------------------------------------------------------------*/

string get_versions(){

	clear();
    system("/bin/versionInfo/versionAPI >> str.txt");
   
    fstream f("str.txt", fstream::in );
    string s="";

    while(!f.eof()){
        string p;
        getline( f, p);
        p+="\n";
        s+=p;
    }
    f.close();
    return s;
}
void clear()
{
    ofstream file("str.txt");
    file<<"";
}

//===================================Functions required================================================

string convertToString(char* a)
{
  string s = a;
  return s;
}

string getTimezone(){
	redi::ipstream proc("cat /etc/timezone",redi::pstreams::pstdout | redi::pstreams::pstderr );
	std::string line;

	// read child's stdout
	while (std::getline(proc.out(), line)){
		std::cout << "stdout: " << line << '\n';
		return line;
	}
	if (proc.eof() && proc.fail())
		proc.clear();
	// read child's stderr
	while (std::getline(proc.err(), line)){
		std::cout << "stderr: " << line << '\n';
		return line;
	}
	return line;
}

bool port_validate(string Port)
{
	for(int i=0;i<Port.length();i++)
	{
		if(isdigit(Port[i]) == false)
		return false; 	
	}
	return true;	
}

std::string getutcfromlocalusingdb(const char* currtime){
	std::string inputStr = currtime;
	cout<<"timrstr == "<<currtime<<endl;
	Json::FastWriter resultWriter;
	Json::StreamWriterBuilder wbuilder;
	Json::Value resultValue;
	std::string output;
	std::string strConnection = std::string("dbname = "+db_database+" user = "+db_uname+" password = "+db_upass+" hostaddr = "+host_addr+" port = "+db_port+"");
	const char * factorsql;
	string factorSelect = "select '"+inputStr+"'::TIMESTAMP WITHOUT TIME ZONE at time zone current_setting('TimeZone') at TIME ZONE 'utc'";
	factorsql = factorSelect.c_str();
	cout<<"qry =="<<factorSelect<<endl;

	try 
	{
		connection C(strConnection);
		if (!C.is_open()) 
		{
				//std::string output;
			Json::Value jVal1, jVal2;

			jVal1["result"] = "Fail1";
			jVal2["reason"] = "Could not open database";
			resultValue["field"].append(jVal1);
			resultValue["field"].append(jVal2);
			output  = Json::writeString(wbuilder, resultValue);  
			return output;
		}
		nontransaction N(C);
		result R( N.exec( factorsql ));
		int resSize = R.size();
		cout<<"size = "<<resSize<<endl;
		if(resSize>0)
		{
			for (result::const_iterator c = R.begin(); c != R.end(); ++c) 
			{
				output = c[0].as<string>();
				cout<<"output time == "<<output<<endl;
			}
		}
		N.commit();
		C.disconnect ();
	}
	catch (const std::exception &e) 
	{
		Json::Value jVal1, jVal2;

		jVal1["result"] = "Fail2";
		jVal2["reason"] = e.what();
		resultValue["field"].append(jVal1);
		resultValue["field"].append(jVal2);
	}
	cout<<"output time == "<<output<<endl;
	return output;
}

void write_version_to_file()
{
	 /*    map<string, string> month {
        {"Jan", "01"}, {"Feb", "02"}, {"Mar","03"}, {"Apr","04"}, {"May", "05"}, {"Jun","06"}, {"Jul","07"}, {"Aug","08"}, {"Sep","09"}, {"Oct","10"},
        {"Nov","11"},{"Dec","12"}
    };  */

    std::string date = __DATE__;
    std::string time = __TIME__;

    string samp;
    stringstream sstream_word7(date);
    vector<string> param;
    while(sstream_word7 >> samp){
        param.push_back(samp);
    }
    if(stoi(param.at(1)) >=10){
        date = param.at(1)+"-"+param.at(0)+"-"+param.at(2);
    }else{
        	date ="0"+param.at(1)+"-"+param.at(0)+"-"+param.at(2);
    }

    std::string date_time = date+" "+time;

	bool changed_version_num = false;
	bool changed_version_date = false;
	bool changed_version_comment = false;
	int lines_count=0;
	int count1=0,count2=0,count3=0;
  	std::string cur_date = current_date();
	std::string cur_time = current_time();
	ifstream aFile ("/etc/versionInfo/versionconfig.conf");			//server 

 	std::string line,findStr1,findStr2,findStr3,getStr1="#commissioning-server";
    	while (std::getline(aFile , line))
    	{
    		   ++lines_count;
      		if(getStr1 == line){
          	count1 = lines_count;
     	 		}
     	 	if(lines_count == count1+1){
     	 		findStr1=line;
     	 	}	
     	 	if(lines_count == count1+2){
     	 		findStr2=line;
     	 	}
     	 	if(lines_count == count1+3){
     	 		findStr3=line;
     	 	}
     	}   	
    count2=count1+2;
    count3=count1+3;
    //feetching old version number from the sentence
	string tmp;
	string oldVer_num;

    stringstream sstream_word(findStr1);
    vector<string> words;
    	while(sstream_word >> tmp)
    		{
    			words.push_back(tmp);
    		}
        for(int i = 0; i < words.size(); i++)
        {
                if(i == 2)
                {
                	oldVer_num = words.at(i);
                }
        }

	std::string var_num = VERSION_NUMBER;
 
  	if( oldVer_num != var_num )
  	{
  	//cout<<"version no is changed"<<endl;
	std::string qry1="sudo sed -i '"+to_string(count1+1)+" s/"+findStr1+"/version_number = "+var_num+"  #Changed to "+var_num+" on "+cur_date+" "+cur_time+"/g' /etc/versionInfo/versionconfig.conf";

		int n;
		n=system(qry1.c_str());
		//cout<<"value of n :"<<n<<endl;
		if(n == 0){
			//cout<<"New version added in config file."<<endl;
			changed_version_num = true;
		}else{
			cout<<"Not able to Change version number."<<endl;
		}

  	}
  	else{
  	//cout<<"Version number already present"<<endl;
  	}   

//----------------------------version date-------------------------------------------
    //feetching old date and time from the sentence
		string tm;
		string old_date_time;

        stringstream sstream_words(findStr2);
    	vector<string> word;
    	while(sstream_words >> tm){
    			word.push_back(tm);
    	}
        for(int i = 0; i < word.size(); i++){
            if(i == 2 ||i == 3){
                old_date_time += word.at(i);
                old_date_time += " ";
            }
        }

        old_date_time.pop_back();	//removes the last " "

	  //  cout<<"old_date-time:"<<old_date_time<<endl;

	  // cout<<"new_date-time:"<<date_time<<endl;
	   // cout<<findStr2<<endl;

	if(old_date_time != date_time)
 	 {
  		//cout<<"old date_time and new date_time are different."<<endl;
		std::string qry3="sudo sed -i '"+to_string(count2)+" s/"+findStr2+"/version_date = "+date_time+"  #Changed to "+date_time+" on "+cur_date+" "+cur_time+"/g' /etc/versionInfo/versionconfig.conf";

  	    int p;
		p=system(qry3.c_str());
		//cout<<"value of p :"<<p<<endl;
		if(p == 0){

		//cout<<"New date and time added in config file."<<endl;
		changed_version_date = true;

		}else{
			cout<<"Not able to Change date and time."<<endl;
		}

 	 }else{
   		//cout<<"date and time are same"<<endl; 	
  	}

 //--------------------------------version comment--------------------------------------------

    //feetching old version comment from the sentence
	string temp;
	string old_Ver_comment;

    stringstream sstream_word1(findStr3);
    vector<string> Word;
    	while(sstream_word1 >> temp)
    		{
    			Word.push_back(temp);
    		}
        for(int i = 0; i < Word.size(); i++){
                if(i >= 2){
                	if(Word.at(i)=="#Changed"){
                		break;
                	}
                	old_Ver_comment += Word.at(i);
                	old_Ver_comment += " ";
                }
        }      
        old_Ver_comment.pop_back();	//removes the last " "

      //  cout<<"old comment :"<<old_Ver_comment<<endl;
      //  cout<<"new comment :"<<VERSION_COMMENT<<endl;

  		std::string new_comment = VERSION_COMMENT;

  	if(old_Ver_comment != new_comment)
  	{
  //	cout<<"old comment and new comment are different."<<endl;

		std::string qry5="sudo sed -i '"+to_string(count3)+" s/"+findStr3+"/version_comment = "+new_comment+"  #Changed to "+new_comment+" on "+cur_date+" "+cur_time+"/g' /etc/versionInfo/versionconfig.conf";

  		int y;
		y=system(qry5.c_str());
		//cout<<"value of y :"<<y<<endl;
		if(y == 0){
			//cout<<"New version comment added in config file."<<endl;
			changed_version_comment = true;
		}
		else{
			cout<<"Not able to Change version comment."<<endl;
		}
  	}
  	else{
   		//cout<<"old and new version comment are same"<<endl; 	
 	}

  if(changed_version_comment || changed_version_date || changed_version_num){

  		std::string qry10 = "sudo sed -i '"+to_string(count1+4)+" a \n' /etc/versionInfo/versionconfig.conf";
  		std::string qry7 = "sudo sed -i '"+to_string(count1+5)+" i #"+findStr3+"' /etc/versionInfo/versionconfig.conf";
  		std::string qry8 = "sudo sed -i '"+to_string(count1+5)+" i #"+findStr2+"' /etc/versionInfo/versionconfig.conf";
  		std::string qry9 =  "sudo sed -i '"+to_string(count1+5)+" i #"+findStr1+"' /etc/versionInfo/versionconfig.conf";

  		int h7,h8,h9,h10;
  		h10= system(qry10.c_str()); //cout<<"h10:"<<h10<<endl; 
  		if(h10==0){
  			//cout<<"New line added."<<endl;
  		}else{
  			cout<<"Failed to add new line."<<endl;
  		}

  		h7= system(qry7.c_str());	//cout<<"h7:"<<h7<<endl;
  		if(h7==0){
  			//cout<<"old version_comment line commented."<<endl;
  		}else{
  			cout<<"Failed to comment old version_comment line."<<endl;
  		}  	

  		h8= system(qry8.c_str());	//cout<<"h8:"<<h8<<endl;
   		if(h8==0){
  			//cout<<"old version_date line commented."<<endl;
  		}else{
  			cout<<"Failed to comment old version_date line."<<endl;
  		}  

  		h9= system(qry9.c_str());	//cout<<"h9:"<<h9<<endl;
   		if(h9==0){
  			//cout<<"old version_number line commented."<<endl;
  		}else{
  			cout<<"Failed to comment old version_number line."<<endl;
  		}  		
  	}
}


/**
 * @brief [This function will be used to validates the given credentails]
 * @details [In order to use the APIs, the caller must provide the credentials. Upon verifying the credentials, the API will return the result. So this function will be used to validate the credentials provided.]
 * 
 * @param user [description]
 * @param password [description]
 * 
 * @return [description]
 */
int CheckPassword( const char* user, const char* password )
{
    struct passwd* passwdEntry = getpwnam( user );
    if ( !passwdEntry )
    {
        printf( "User '%s' doesn't exist\n", user );
        return 1;
    }

    if ( 0 != strcmp( passwdEntry->pw_passwd, "x" ) )
    {
        return strcmp( passwdEntry->pw_passwd, crypt( password, passwdEntry->pw_passwd ) );
    }
    else
    {
        // password is in shadow file
        struct spwd* shadowEntry = getspnam( user );
        if ( !shadowEntry )
        {
            printf( "Failed to read shadow entry for user '%s'\n", user );
            return 1;
        }

        return strcmp( shadowEntry->sp_pwdp, crypt( password, shadowEntry->sp_pwdp ) );
    }
}

// Function to remove duplicate elements
// This function returns new size of modified
// array.
int removeDuplicates(int arr[], int n)
{
    // Return, if array is empty
    // or contains a single element
    if (n==0 || n==1)
        return n;
 
    int temp[n];
 
    // Start traversing elements
    int j = 0;
    for (int i=0; i<n-1; i++)
 
        // If current element is not equal
        // to next element then store that
        // current element
        if (arr[i] != arr[i+1])
            temp[j++] = arr[i];
 
    // Store the last element as whether
    // it is unique or repeated, it hasn't
    // stored previously
    temp[j++] = arr[n-1];
 
    // Modify original array
    for (int i=0; i<j; i++)
        arr[i] = temp[i];
 
    return j;
}

//This function returns current local date
std::string current_date(){
    time_t now = time(NULL);
    struct tm tstruct;
    char buf[40];
    tstruct = *localtime(&now);
    //format: day DD-MM-YYYY
    strftime(buf, sizeof(buf), "%A %d-%m-%Y", &tstruct);
    return buf;
}

//This function returns current local time
std::string current_time(){
    time_t now = time(NULL);
    struct tm tstruct;
    char buf[40];
    tstruct = *localtime(&now);
    //format: HH:mm:ss
    strftime(buf, sizeof(buf), "%X", &tstruct);
    return buf;
}
//this function returns the last word of respective line from plcdatacollector.conf file 
std::string get_last_word(std::string s) 
{
	auto index = s.find_last_of(' ');
	std::string last_word = s.substr(++index);
    return last_word;
}
std::string today_date(){
    time_t now = time(NULL);
    struct tm tstruct;
    char buf[40];
    tstruct = *localtime(&now);
    //format: day DD-MM-YYYY
    strftime(buf, sizeof(buf), "%d_%m_%Y", &tstruct);
    return buf;
}
std::string now_time(){
    time_t now = time(NULL);
    struct tm tstruct;
    char buf[40];
    tstruct = *localtime(&now);
    //format: HH:mm:ss
    strftime(buf, sizeof(buf), "%H_%M_%S", &tstruct);
    return buf;
}
//-------------------------------------------------------------------------------------


