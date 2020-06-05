#include <iostream>
#include <unistd.h>
#include <filesystem>
#include <vector>
#include <cstdlib>
#include <wait.h>
#include <strings.h>
#include <algorithm>

using namespace std;
namespace fs = filesystem;

/**
 * Written by Javid Asgarov &
 *            Aaron Nietgen
 */

/**
 * The method that searched starting from the path for each of the file names
 * @param searchPath
 * @param fileNames
 * @param isRecursive: if true, it will search recursively
 * @param ignoreCase: if true, will find files that match names ignoring case
 */
void search(const string &searchPath, vector<string> &fileNames, bool isRecursive, bool ignoreCase);

/**
 * Gets the path from the arguments and if the path is relative converts it to an absolute one
 * @param arguments
 * @return
 */
string getSearchPath(const vector<string> &arguments);

/**
 * Get's all of the file names to be searched for from arguments
 * @param arguments
 * @return
 */
vector<string> getFileNames(const vector<string> &arguments);

/**
 * Helper function that returns a vector of arguments
 * Simply because vectors are easier to work with
 * @param argc
 * @param args
 * @return
 */
vector<string> getAllArguments(int argc, char *const *args);

/**
 * uses getopt() function to read and set the options
 * @param argc
 * @param args
 * @param isIgnoreCase
 * @param isRecursive
 */
void setOptions(int argc, char *const *args, bool &isIgnoreCase, bool &isRecursive);

/**
 * parses the file name from path which is necessary for comparing them to the file names that are being searched for
 * @param path
 * @return
 */
string parseFileName(const string &path);

/**
 * helper function that compares strings ignoring case
 * @param str1
 * @param str2
 * @return
 */
bool ignoreCaseCompare(string &str1, string &str2);

/**
 * function that compares strings, either literally or ignoring case, depending on ignoreCase flag
 * @param fileName
 * @param name
 * @param ignoreCase
 * @return
 */
bool compareStrings(string &fileName, string &name, bool ignoreCase);

int main(int argc, char *args[]) {
    // Reading and storing all filenames that I will need to search for
    vector<string> arguments = getAllArguments(argc, args);

    string searchPath = getSearchPath(arguments);
    vector<string> fileNames = getFileNames(arguments);

    // setting options
    bool isIgnoreCase;
    bool isRecursive;
    setOptions(argc, args, isIgnoreCase, isRecursive);

    search(searchPath, fileNames, isRecursive, isIgnoreCase);
    return 0;
}

std::string getSearchPath(const vector<string> &arguments) {
    string searchPath;
    for (auto &arg : arguments) {
        if (arg.find('/') != string::npos) {
            searchPath = arg;
        }
    }
    //turning relative path into absolute (doesnt affect absolute paths)
    fs::path path = fs::path("..") / searchPath;
    return fs::canonical(path).string();
}

void setOptions(int argc, char *const *args, bool &isIgnoreCase, bool &isRecursive) {
    int opt;
    while ((opt = getopt(argc, args, "iR")) != -1) {
        switch (opt) {
            case 'i': {
                isIgnoreCase = true;
                break;
            }
            case 'R': {
                isRecursive = true;
                break;
            }
            case '?':
                printf("unknown option: %c\n", optopt);
                break;
        }
    }
}

vector<string> getFileNames(const vector<string> &arguments) {
    vector<string> fileNames;
    for (auto &arg : arguments) {
        if (arg.compare("-R") == 0 ||
            arg.compare("-i") == 0 ||
            arg.compare("-Ri") == 0 ||
            arg.compare("-iR") == 0 ||
            arg.find('/') != string::npos) { continue; }
        fileNames.emplace_back(arg);
    }
    return fileNames;
}

vector<string> getAllArguments(int argc, char *const *args) {
    vector<string> arguments;
    arguments.reserve(argc);

    //starting from 1 because argc[0] is the path of the program
    for (int i = 1; i < argc; ++i) {
        arguments.emplace_back(args[i]);
    }
    return arguments;
}

string parseFileName(const string &path) {
    int startOfFileName = path.find_last_of('/');
    string fileName = path.substr(startOfFileName + 1);
    return fileName;
}

bool ignoreCaseCompare(std::string &str1, std::string &str2) {
    return ((str1.size() == str2.size()) && std::equal(str1.begin(), str1.end(), str2.begin(), [](char &c1, char &c2) {
        return (c1 == c2 || std::toupper(c1) == std::toupper(c2));
    }));
}

bool compareStrings(string &fileName, string &name, bool ignoreCase) {
    return ignoreCase ? ignoreCaseCompare(fileName, name) : fileName.compare(name) == 0;
}

void search(const string &searchPath, vector<string> &fileNames, bool isRecursive, bool ignoreCase) {
    for (auto &p: fs::directory_iterator(searchPath)) {
        if (isRecursive) {
            try {
                if (p.is_directory()) {
                    pid_t pid = fork();
                    if (pid == 0) {
                        search(p.path().string(), fileNames, isRecursive, ignoreCase);
                        exit(0);
                    }
                    //parent waits for child process to finish
                    wait(NULL);
                    continue;
                }
            } catch (std::filesystem::__cxx11::filesystem_error error) {
                //in case user doesn't have rights for some directories on the search path this error will be thrown
                //nothing to handle -> program will keep searching
            }

        }

        string path = p.path().string();
        string fileName = parseFileName(path);

        for (auto &name : fileNames) {
            if (compareStrings(fileName, name, ignoreCase)) {
                cout << getpid() << " : " << fileName << " : " << path << endl;
            }
        }
    }
}
