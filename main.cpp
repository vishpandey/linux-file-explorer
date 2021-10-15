#include <filesystem>
#include <iostream>
#include <fstream>
#include <bits/stdc++.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h> 
#include<sys/wait.h>
 #include <errno.h>
#include <cstdio>
#include <experimental/filesystem>

using namespace std;
namespace fs = experimental::filesystem;

vector<string> visitedDirectoryRecord;
int currentDirectoryIndex = 0;
int windowNum = 0;
int numOfDirectoryRows = 0;
int minAllowedXCoord = 1;
int maxAllowedXCoord = 5;
int startWindow = 0;
int endWindow = 4;
int currentCursorIndex = 1;
int oldXCoordinate;
int permittedRowsInTerminal = 5;
int maxRowInTerminal;
int xCoord = 1;
int yCoord = 0;
string oldCurrentPath;

string rootPath() {
   char buff[PATH_MAX];
   getcwd( buff, PATH_MAX );
   string cwd(buff);
   return cwd;
}

string rootDir = rootPath();
string currentPath = "" + rootDir;

class DirectoryContents {
	string filename;
	string fileSize;
	string groupOwner;
	string userOwner;
	string permissions;
	string lastModified;
	char type;

public:
	DirectoryContents(string filename, string fileSize, string groupOwner, string userOwner, string permissions, 
						string lastModified, char type) {
		this->filename = filename;
		this->fileSize = fileSize;
		this->groupOwner = groupOwner;
		this->userOwner = userOwner;
		this->permissions = permissions;
		this->lastModified = lastModified;
		this->type = type;
	}

	string getFileName() {
		return this->filename;
	}

	string getFileSize() {
		return this->fileSize;
	}

	string getGroupOwner() {
		return this->groupOwner;
	}

	string getUserOwner() {
		return this->userOwner;
	}

	string getPermissions() {
		return this->permissions;
	}

	string getLastModified() {
		return this->lastModified;
	}

	char getType() {
		return this->type;
	}	
};


// showDirectoryLayout() {

// }

char* readable_fs(double size, char *buf) {
    int i = 0;
    const char* units[] = {"B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    while (size > 1024) {
        size /= 1024;
        i++;
    }
    sprintf(buf, "%.*f %s", i, size, units[i]);
    return buf;
}

char* getPermissions(mode_t mode, char type) {
	char* permissions = (char *)malloc(sizeof(char) * 11 + 1);

	permissions[0] = (type == 'd') ? 'd' : '-';
	permissions[1] = (mode & S_IRUSR) ? 'r' : '-';
    permissions[2] = (mode & S_IWUSR) ? 'w' : '-';
    permissions[3] = (mode & S_IXUSR) ? 'x' : '-';
    permissions[5] = (mode & S_IRGRP) ? 'r' : '-';
    permissions[6] = (mode & S_IWGRP) ? 'w' : '-';
    permissions[7] = (mode & S_IXGRP) ? 'x' : '-';
    permissions[8] = (mode & S_IROTH) ? 'r' : '-';
    permissions[9] = (mode & S_IWOTH) ? 'w' : '-';
    permissions[10] = (mode & S_IXOTH) ? 'x' : '-';
    permissions[11] = '\0';
    
    return permissions; 
}

void outputDirectoryContent(DirectoryContents temp) { 
	printf("%s\t", temp.getFileName().c_str());
	printf("\t%s", temp.getPermissions().c_str());
	printf("\t%s", temp.getGroupOwner().c_str());
	printf("\t%s", temp.getUserOwner().c_str());
	printf("\t%s", temp.getFileSize().c_str());
	printf("\t%s", temp.getLastModified().c_str());
} 

void displayDirectoryContents(vector<DirectoryContents> dirStub) {
	printf("\033[H\033[J");
	printf("%c[%d;%dH", 27, 1, 1);
    
    for(int i = startWindow; i <= endWindow && i < dirStub.size(); i++) {
    	outputDirectoryContent(dirStub[i]);
    }

	printf("%c[%d;%dH", 27, 1, 1);
}


DirectoryContents createOutputStub(char* name, struct stat fileInfo) {
	char type;
	
	if ((fileInfo.st_mode & S_IFMT) == S_IFDIR) {
		type = 'd';
	} else {
		type = 'f';
	}

	char buf[10];
	string size = readable_fs(fileInfo.st_size, buf);

	string modTime = ctime(&fileInfo.st_mtime);

	string fileName = name;

	string permissions = getPermissions(fileInfo.st_mode, type);

	struct passwd *pw = getpwuid(fileInfo.st_uid);
	struct group  *gr = getgrgid(fileInfo.st_gid);

	DirectoryContents temp(fileName, size, gr->gr_name, pw->pw_name, permissions, modTime, type);

	//outputDirectoryContent(temp);

	return temp;
}

void setCursorPosition(int x, int y) {
	printf("%c[%d;%df",0x1B,x,y);
}

bool compareFileName(DirectoryContents obj1, DirectoryContents obj2) {
	return (obj1.getFileName() < obj2.getFileName());
}

vector<DirectoryContents> loadDirectoryContent(string path, vector<DirectoryContents> dirStub) {
	DIR *dir; 
	struct dirent *diread;

	string finalPath;

	if ((dir = opendir(path.c_str())) != nullptr) {
        
        while ((diread = readdir(dir)) != nullptr) {
            
            struct stat fileInfo;
            
            finalPath = path + "/" + diread->d_name;
            char *path = new char[finalPath.length() + 1];

            strcpy(path, finalPath.c_str());
            
            stat(path, &fileInfo);
        	
        	dirStub.push_back(createOutputStub(diread->d_name, fileInfo));
        }
        
        closedir (dir);
    
    } else {
        
        perror ("opendir");
        cout << EXIT_FAILURE << endl;
    
    }

    sort(dirStub.begin(), dirStub.end(), compareFileName);

    //printf("%c[2J", 27);

    return dirStub;
}

vector<string> parseInput(string input) {
	string word = "";
	vector<string> commandString;

	//cout << input << endl;
	for (int i = 0; i < input.length(); i++) {
        if (input.at(i) == ' ') {
            commandString.push_back(word);
            word = "";
        }
        else {
            word = word + input.at(i);
        }
    }

    if(word != "") {
    	commandString.push_back(word);
    }

    return commandString;
}


void copyFile(string source, struct stat s, string destination) {
	
	string srcPath = source;

	char *destinationPath = new char[destination.length() + 1];
    strcpy(destinationPath, destination.c_str());

    string line;
    ifstream ini_file;
    
    ini_file.open(srcPath);

    ofstream out_file;

    out_file.open(destination, fstream::in | fstream::out | fstream::trunc);
    if(!out_file) {
    	out_file.open(destination, ios::out);    
        out_file <<"\n";
    }

    if(ini_file && out_file){
 
        while(getline(ini_file,line)){
            out_file << line << "\n";
        }
    } else {
    	perror("fileRead");
    }

    int n, err;
    unsigned char buffer[1024];

	int status = chown(destinationPath, s.st_uid, s.st_gid);
	if(status != 0) {
		cout<<endl;
		cout<<"\033[0;31m"<< "Could not set ownership of file" <<endl;
		cout<<"\033[0m";
		cout<<":";
	}

	status = chmod(destinationPath, s.st_mode);
	if(status != 0) {
		cout<<endl;
		cout<<"\033[0;31m"<< "Could not set permissions of file" <<endl;
		cout<<"\033[0m";
		cout<<":";
	}

	ini_file.close();
 	out_file.close();
}


void removeFile(string source) {
	
	char *path = new char[source.length() + 1];
    strcpy(path, source.c_str());

	if(remove(source.c_str()) != 0) {
	 	perror("removeFile");
	}
}


void copyDirectory(string source, struct stat s, string destination) {
	
	string srcPath = source;
	char *destinationPath = new char[destination.length() + 1];
    strcpy(destinationPath, destination.c_str());

    struct stat st;
    int status = 0;

	if (stat(destinationPath, &st) != 0) {
        if (mkdir(destinationPath, s.st_mode) != 0 && errno != EEXIST) {
        	int status = mkdir(destinationPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			if(status == -1) {
			 	perror("mkdir");
			}
        }
    }

    DIR *dir; 
	struct dirent *diread;

	string tempSourcePath, tempDestinationPath;

	if ((dir = opendir(srcPath.c_str())) != nullptr) {
        
        while ((diread = readdir(dir)) != nullptr) {
            
            struct stat fileInfo;

            if( (string(diread->d_name) == "..") || (string(diread->d_name) == ".") )	{
				continue;   
		    }
            
            tempSourcePath = srcPath + "/" + string(diread->d_name);
            tempDestinationPath = destination + "/" + string(diread->d_name);
            
            char *path = new char[tempSourcePath.length() + 1];
            strcpy(path, tempSourcePath.c_str());
            
            if (stat(path, &fileInfo) == -1) {
            	perror("stat");
            } else if((S_ISDIR(fileInfo.st_mode))) {
        		copyDirectory(tempSourcePath, fileInfo, tempDestinationPath);
        	} else if (S_ISREG(fileInfo.st_mode)) {
        		copyFile(tempSourcePath, fileInfo, tempDestinationPath);
        	}
        }
        
        closedir (dir);
    
    } else {
    	cout << "opendir error: " << srcPath << endl;
        perror ("opendir");
        cout << EXIT_FAILURE << endl;
    
    }
}


void removeDirectory(string source) {
	
	string srcPath = source;

    struct stat st;
    int status = 0;

	if (stat(srcPath.c_str(), &st) != 0) {
        perror("stat");
    }

    DIR *dir; 
	struct dirent *diread;

	string tempSourcePath, tempDestinationPath;

	if ((dir = opendir(srcPath.c_str())) != nullptr) {
        
        while ((diread = readdir(dir)) != nullptr) {
            
            struct stat fileInfo;

            if( (string(diread->d_name) == "..") || (string(diread->d_name) == ".") )	{
				continue;   
		    }
            
            tempSourcePath = srcPath + "/" + string(diread->d_name);
           
            char *path = new char[tempSourcePath.length() + 1];
            strcpy(path, tempSourcePath.c_str());
            if (stat(path, &fileInfo) == -1) {
            	perror("stat");
            } else if((S_ISDIR(fileInfo.st_mode))) {
        		removeDirectory(tempSourcePath);
        		if (rmdir(tempSourcePath.c_str()) != 0) {
        			perror("removeDirectory");
        		}
        	} else if (S_ISREG(fileInfo.st_mode)) {
        		removeFile(tempSourcePath);
        	}
        }
        
        closedir (dir);
    
    } else {
    	cout << "opendir error: " << srcPath << endl;
        perror ("opendir");
        cout << EXIT_FAILURE << endl;
    
    }
}

void createFile(string source, string destination) {
	ofstream out_file(destination);
}

void createDirectory(string source, string destination) {
	char *destinationPath = new char[destination.length() + 1];
    strcpy(destinationPath, destination.c_str());
	if (mkdir(destinationPath, 0777) != 0 && errno == EEXIST) {
    	int status = mkdir(destinationPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(status == -1) {
		 	perror("mkdir");
		}
    }
}

vector<DirectoryContents> dirStub;
void refreshScreen() {
	dirStub.clear();
	dirStub = loadDirectoryContent(currentPath, dirStub);
	numOfDirectoryRows = dirStub.size();

	displayDirectoryContents(dirStub);
	xCoord = 1;
	yCoord = 0;
}

void refreshCommandTab() {
	printf("%c[%d;%dH",27, maxRowInTerminal, 1);
	printf("%c[2K", 27);
	cout<<":";
}

void refreshCommandTabWithError(string errMsg) {
	printf("%c[%d;%dH",27, maxRowInTerminal, 1);
	printf("%c[2K", 27);
	cout<<":" << errMsg;
}


bool startsWith(string mainStr, string prefix) {
    if(mainStr.find(prefix) == 0)
        return true;
    else
        return false;
}

void setCurrentPathToParent() {
	if(currentPath == rootDir) {
		return;
	}
	int iter = currentPath.length() - 1;
    while(iter >= 0 && currentPath.at(iter) != '/') {
    	iter--;
    }
    
    if(iter > 0 && iter < currentPath.length() - 1 && currentPath.at(iter) == '/') {
    	currentPath = currentPath.substr(0, iter);
    }
}


bool searchCurrentDirectory(string source, bool found, string keyword) {
	vector<DirectoryContents> tempStub;

	string srcPath = source;
	DIR *dir; 
	struct dirent *diread;

	if ((dir = opendir(srcPath.c_str())) != nullptr) {
        
        while ((diread = readdir(dir)) != nullptr) {
            
            struct stat fileInfo;

            if( (string(diread->d_name) == "..") || (string(diread->d_name) == ".") )	{
				continue;   
		    }

		    if (keyword == string(diread->d_name)) {
            	found = true;
            	return found;
            }
            
            string tempSourcePath = srcPath + "/" + string(diread->d_name);
           
            char *path = new char[tempSourcePath.length() + 1];
            strcpy(path, tempSourcePath.c_str());
			if (stat(path, &fileInfo) == -1) {
            	perror("stat");
            }
            if((S_ISDIR(fileInfo.st_mode))) {
        		found = searchCurrentDirectory(tempSourcePath, found, keyword);
        	}
        }
        
        closedir (dir);
    
    } else {
    	cout << "opendir error: " << srcPath << endl;
        perror ("opendir");
        cout << EXIT_FAILURE << endl;
    
    }

    return found;
}

string rootDirRep = "~";
string prevDirRep = "..";
void enterCommandMode(int permittedRowsInTerminal) {
	
	vector<string> command;
	vector<string> source;
	vector<string> destination;
	string tempDestinationPath;
	string tempSourcePath;

	while(1) {
		
		string input = "";
		
		char ch;
		while(((ch = getchar())!= 10) && ch != 27) {
			if(ch == 127) {
				refreshCommandTab();
				if(input.length() == 1) {
					input = "";
					cout << input;
				} else {
					input = input.substr(0,input.length()-1);
				}

				cout << input;

			} else {
				input += ch;
				cout << ch;
			}
		}

		if(ch == 27) {
			return;
		}

		command = parseInput(input);

		if(command[0] == "copy") {
			if(command.size() < 3) {
				refreshCommandTabWithError("Atleast 3 arguments needed");
				continue;
			}

			int len = command.size();
			if (startsWith(command[len - 1], rootDirRep)) {
				command[len - 1] = rootDir + command[len - 1].substr(rootDirRep.length());
			} else if (startsWith(command[len - 1], "/")) {
				command[len - 1] = command[len - 1];
			} else {
				command[len - 1] = rootDir + "/" + command[len - 1];
			}

			for(int i = 1; i < command.size() - 1; i++) {	
				struct stat s;

				if( stat(command[i].c_str(),&s) == 0 )
				{
				    if(S_ISDIR(s.st_mode))
				    {
				    	tempDestinationPath = command[len - 1] + "/" + command[i];
				    	tempSourcePath = currentPath + "/" + command[i];
				        copyDirectory(tempSourcePath, s, tempDestinationPath);
				    }
				    else if(S_ISREG(s.st_mode))
				    {
				    	tempDestinationPath = command[len - 1] + "/" + command[i]; 
				        copyFile(command[i], s, tempDestinationPath);
				    }
					
					refreshCommandTab();
				}
				else
				{
					refreshCommandTabWithError("error while copying file");
				}	
			}
		} else if(command[0] == "move") {
			if(command.size() < 3) {
				refreshCommandTabWithError("Atleast 3 arguments needed");
				continue;
			}

			int len = command.size();
			if (startsWith(command[len - 1], rootDirRep)) {
				command[len - 1] = rootDir + "/" + command[len - 1].substr(rootDirRep.length());
			} else if (startsWith(command[len - 1], "/")) {
				command[len - 1] = command[len - 1];
			} else {
				command[len - 1] = rootDir + "/" + command[len - 1];
			}

			for(int i = 1; i < command.size() - 1; i++) {	
				struct stat s;
				if( stat(command[i].c_str(),&s) == 0 )
				{
				    if(S_ISDIR(s.st_mode))
				    {
				    	tempDestinationPath = command[len - 1] + "/" + command[i];
				    	copyDirectory(command[i], s, tempDestinationPath);
				        removeDirectory(command[i]);
				        if (rmdir(command[i].c_str()) != 0) {
							perror("removeDirectory");
						}
				    }
				    else if(S_ISREG(s.st_mode))
				    {
				    	tempDestinationPath = command[len - 1] + "/" + command[i]; 
				        copyFile(command[i], s, tempDestinationPath);
				        removeFile(command[i]);
				    }
					
					refreshScreen();
					refreshCommandTab();
				}
				else
				{
					refreshCommandTabWithError("error while copying file");
					continue;
				}	
			}
		} else if(command[0] == "rename") {
			if(command.size() != 3) {
				refreshCommandTabWithError("Only 3 arguments needed");
				continue;
			}
			struct stat s;
			if( stat(command[1].c_str(),&s) == 0 )
			{
			    if(S_ISDIR(s.st_mode))
			    {
			    	copyDirectory(command[1], s, command[command.size() - 1]);
			        removeDirectory(command[1]);
			        if (rmdir(command[1].c_str()) != 0) {
        				refreshCommandTabWithError("rename opration failed");
        				continue;
        			}
			    }
			    else if(S_ISREG(s.st_mode))
			    {
			    	string tempDestination = command[command.size() - 1]; 
			        copyFile(command[1], s, tempDestination);
			        removeFile(command[1]);
			    }
				
				refreshScreen();
				refreshCommandTab();
			} else {
				refreshCommandTabWithError("error while copying file");
			}		
		} else if (command[0] == "create_file") {
			if(command.size() != 3) {
				refreshCommandTabWithError("Only 3 arguments allowed");
				continue;
			}	

			struct stat s;
			string tempDestination;

			if (startsWith(command[2], rootDirRep)) {
				tempDestination = rootDir + "/" + command[2].substr(rootDirRep.length()) + "/" + command[1];
			} else if(command[2] == ".") {
				tempDestination = currentPath + "/" + command[1];
			} else if (startsWith(command[1], "/")) {
				tempDestination = command[2] + "/" + command[1];
			} else {
				tempDestination = rootDir + "/" + command[2] + "/" + command[1];
			}
			
			createFile(command[1], tempDestination);
			refreshScreen();
			refreshCommandTab();
		} else if (command[0] == "create_dir") {
			if(command.size() != 3) {
				refreshCommandTabWithError("Only 3 arguments allowed");
				continue;
			}

			struct stat s;
			string tempDestination;

			if (startsWith(command[2], rootDirRep)) {
				tempDestination = rootDir + "/" + command[2].substr(rootDirRep.length()) + "/" + command[1];
			} else if(command[2] == ".") {
				tempDestination = currentPath + "/" + command[1];
			} else if (startsWith(command[1], "/")) {
				tempDestination = command[2] + "/" + command[1];
			} else {
				tempDestination = rootDir + "/" + command[2] + "/" + command[1];
			}

			createDirectory(command[1], tempDestination);
			refreshScreen();
			refreshCommandTab();
		} else if (command[0] == "delete_file") {
			if(command.size() != 2) {
				refreshCommandTabWithError("Only 2 arguments allowed");
				continue;
			}

			if (startsWith(command[1], rootDirRep)) {
				command[1] = rootDir + "/" + command[1].substr(rootDirRep.length()) + "/" + command[1];
			} else if (startsWith(command[1], "/")) {
				command[1] = command[1];
			} else {
				command[1] = rootDir + "/" + command[1];
			}

			removeFile(command[1]);
			refreshScreen();
			refreshCommandTab();
		} else if (command[0] == "delete_dir") {
			if(command.size() != 2) {
				refreshCommandTabWithError("Only 2 arguments allowed");
				continue;
			}

			if (startsWith(command[1], rootDirRep)) {
				command[1] = rootDir + "/" + command[1].substr(rootDirRep.length()) + "/" + command[1];
			} else if (startsWith(command[1], "/")) {
				command[1] = command[1];
			} else {
				command[1] = rootDir + "/" + command[1];
			}

			removeDirectory(command[1]);
			if (rmdir(command[1].c_str()) != 0) {
				perror("removeDirectory");
			}
			refreshScreen();
			refreshCommandTab();
		} else if (command[0] == "goto") {
			if(command.size() != 2) {
				refreshCommandTabWithError("Only 2 arguments allowed");
				continue;
			}

			if (startsWith(command[1], rootDirRep)) {
				command[1] = rootDir + "/" + command[1].substr(rootDirRep.length());
			} else if (startsWith(command[1], "/")) {
				command[1] = command[1];
			} else if (startsWith(command[1], ".")) {
				continue;
			} else {
				command[1] = rootDir + "/" + command[1];
			}

			currentPath = command[1];
			visitedDirectoryRecord.push_back(currentPath);
    		currentDirectoryIndex++;
    		return;
		} else if (command[0] == "search") {
			if(command.size() != 2) {
				refreshCommandTabWithError("Only 2 arguments allowed");
				continue;
			}

			bool found = searchCurrentDirectory(currentPath, false, command[1]);

			if(found) {
				printf("%c[%d;%dH",27, maxRowInTerminal+1, 1);
				printf("%c[2K", 27);
				cout<<":" << "True";
				continue;
			}

			printf("%c[%d;%dH",27, maxRowInTerminal+1, 1);
			printf("%c[2K", 27);
			cout<<":" << "False";
		}
	}
}

void clearScreen() {
	printf("\033[H\033[J");
	printf("%c[%d;%dH", 27, 1, 1);
}


int main(int argc, char * argv[]) {
	int index;

	struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    maxRowInTerminal = w.ws_row;

    permittedRowsInTerminal = 5;
    startWindow = 0;
    endWindow = 4;
    minAllowedXCoord = 1;
    maxAllowedXCoord = 5;

	visitedDirectoryRecord.push_back(currentPath);
	
	refreshScreen();

	maxAllowedXCoord = (dirStub.size() <= 5) ? dirStub.size() : 5;
    
    setCursorPosition(xCoord, yCoord);
    
    char key;

    struct termios oldSettings, newSettings;

    tcgetattr( fileno( stdin ), &oldSettings );
    newSettings = oldSettings;
    newSettings.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr( fileno( stdin ), TCSANOW, &newSettings );


    while(1) {
    	key = cin.get();
    	if (key ==  'A') {
    		if(xCoord > minAllowedXCoord) {
	    		xCoord--;
	    		setCursorPosition(xCoord, yCoord);
    		}
    	} else if (key == 'B') {
    		if(xCoord < maxAllowedXCoord) {
	    		xCoord++;
	    		setCursorPosition(xCoord, yCoord);
    		}
    	} else if (key == 'l') {
    		if(startWindow < dirStub.size() - 1 && 
    			endWindow < dirStub.size() - 1) {
    			startWindow++;
    			endWindow++;
    		}
    		if(xCoord > 1) {
    			xCoord--;
    		}
    		
    		refreshScreen();

    	} else if (key == 'k') {
    		if(startWindow > 0 && 
    			endWindow > 4) {
    			startWindow--;
    			endWindow--;
    		}
    		if(xCoord < 5) {
    			xCoord++;
    		}
    		
    		refreshScreen();

    	} else if (key == 10) {
    		index = startWindow + xCoord - 1;
    		oldCurrentPath = "" + currentPath;
    		if(startsWith(dirStub[index].getFileName(), "..")) {
    			if(currentPath == rootDir) {
    				continue;
    			}
    			setCurrentPathToParent();
			} else if(startsWith(dirStub[index].getFileName(), ".")) {
				continue;
			} else {
    			currentPath += "/" + dirStub[index].getFileName();
			}
    		
    		if(dirStub[index].getType() == 'd') {			
	    		visitedDirectoryRecord.push_back(currentPath);
	    		currentDirectoryIndex++;

	    		int ch=chdir(currentPath.c_str());
	    		if(ch != 0) {
	    			cout << "directory change not successfull" << endl;
	    		}

	    		refreshScreen();
	    		startWindow = 0;
	    		endWindow = 4;
    		} else {
    			oldXCoordinate = xCoord;
    			pid_t childPid = fork();
    			
    			char *cp = new char[currentPath.size()+1];
				strcpy(cp, currentPath.c_str());
    			
    			if(childPid == 0) {
    				char* args[3] = {"vi", cp, NULL};
    				execvp("vi", args);
    			} else {
    				int *w;
    				wait(w);
    			}

    			currentPath = oldCurrentPath;
    			dirStub.clear();
	    		dirStub = loadDirectoryContent(currentPath, dirStub);
	    		displayDirectoryContents(dirStub);
	    		xCoord = oldXCoordinate;
		    	yCoord = 0;
		    	setCursorPosition(xCoord, yCoord);
    		}

    	} else if (key == 'C') {
    		// right arrow
    		if(currentDirectoryIndex < visitedDirectoryRecord.size() - 1) {
    			
    			currentDirectoryIndex++;
    			currentPath = visitedDirectoryRecord.at(currentDirectoryIndex);

    			int ch=chdir(currentPath.c_str());
	    		if(ch != 0) {
	    			cout << "directory change not successfull" << endl;
	    		}
    			refreshScreen();
    			startWindow = 0;
	    		endWindow = 4;
    		}

    	} else if (key == 'D') {
    		// left arrow
    		if(currentDirectoryIndex > 0) {
    			
    			currentDirectoryIndex--;
    			currentPath = visitedDirectoryRecord.at(currentDirectoryIndex);
    			
    			int ch=chdir(currentPath.c_str());
	    		if(ch != 0) {
	    			cout << "directory change not successfull" << endl;
	    		}
    			refreshScreen();
    			startWindow = 0;
	    		endWindow = 4;
    		}
    	} else if (key == 104 || key == 72) {
    		if(currentPath != rootDir) {
    			currentPath = rootDir;
    			visitedDirectoryRecord.push_back(currentPath);
    			currentDirectoryIndex++;

    			int ch=chdir(currentPath.c_str());
	    		if(ch != 0) {
	    			cout << "directory change not successfull" << endl;
	    		}
    			refreshScreen();
    			startWindow = 0;
	    		endWindow = 4;
    		}
    	} else if (key == 127) {

    		if(currentPath == rootDir) {
    			continue;
    		}
			
			setCurrentPathToParent();

            int ch=chdir(currentPath.c_str());
    		if(ch != 0) {
    			cout << "directory change not successfull" << endl;
    		}
            refreshScreen();
            startWindow = 0;
	    	endWindow = 4;
    	
    	} else if (key == 58) {
    		refreshCommandTab();
			enterCommandMode(permittedRowsInTerminal);

			int ch=chdir(currentPath.c_str());
    		if(ch != 0) {
    			cout << "directory change not successfull" << endl;
    		}
			refreshScreen();
			startWindow = 0;
	    	endWindow = 4;
    	} else if(key == 'q') {
    		clearScreen();
    		break;
    	}
    }

    tcsetattr( fileno( stdin ), TCSANOW, &oldSettings );
	return 0;
}
