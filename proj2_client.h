// Modified from CSE422 FS09
// Modified from CSE422 FS12
// Modified again for CSE422 FS20

#include <climits>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <netdb.h>
#include <string>
#include <sstream>
#include <sys/stat.h>

#include "HTTP_Request.h"
#include "HTTP_Response.h"
#include "URL.h"

/**
 * Prints a brief usage string describing how to use the application,
 * in case the user passes in something that just doesn't work.
 *
 * @param exe_name The name of the executable
 * @param out The ostream
 */
void help_message(const char* exe_name, std::ostream& out) {
    out << "Usage: " << exe_name << " [options]" << std::endl;
    out << "The following options are available:" << std::endl;
    out << "    -s host URL" << std::endl;
    out << "    -p proxy URL" << std::endl;
    out << "    -i client_id" << std::endl;
    out << "    -h display help message" << std::endl;
    out << std::endl;
    out << "Example: " << exe_name 
        << "-s http://www.some_server.com/ -p 100.200.50.150:8080"  << std::endl;
}

/**
 * Parse the cmd line arguments.
 *
 * @param[in] argc The number of cmd line args
 * @param[in] argv The array of cmd line args
 * @param[out] url_addr The url to download
 * @param[out] proxy_addr The address of the proxy
 * @param[out] client_id The client identifier -- used to tag proxy output
 */
int parse_args(int argc, char *argv[],
               char **url_addr, char **proxy_addr, char** client_id) {
    for(int i = 1; i < argc; i++) {
        if((!strncmp(argv[i], "-s", 2)) ||
           (!strncmp(argv[i], "-S", 2))) {
            *url_addr = argv[++i];
        }
        else if((!strncmp(argv[i], "-p", 2)) ||
                (!strncmp(argv[i], "-P", 2))) {
            *proxy_addr = argv[++i];
        }
        else if((!strncmp(argv[i], "-i", 2)) ||
                (!strncmp(argv[i], "-I", 2))) {
            *client_id = argv[++i];
        }
        else if((!strncmp(argv[i], "-h", 2)) ||
                (!strncmp(argv[i], "-H", 2))) {
            help_message(argv[0], std::cout);
            exit(1);
        }
        else {
            std::cerr << "Invalid parameter: argv[i]" << std::endl;
            help_message(argv[0], std::cout);
            exit(1);
        }
    }

    if (*client_id == NULL) {
        *client_id = *url_addr;
    }
}

/**
 * Open a file pointer to store the data in ./Download.
 *
 * @param url The URL for the object
 * @return The file pointer
 */
// Opens a local copy of the file referenced by the given request URL, for
// writing.  Ignores any directories in the URL path, instead opening the file
// in the current directory.  Makes up a filename if none is given.
//
// Returns a pointer to the open file, or a NULL pointer if the open fails.
FILE* open_local(const URL* url) {
    FILE* outfile = NULL;

    struct stat sb; // For checking if ./Download exists

    if(stat("./Download", &sb) == -1) { // if ./Download does not exist
         mkdir("./Download", 0700);   // create it
    }

    const std::string& full_path = url->get_path();
    size_t filename_pos = full_path.rfind('/');
    // find the last '/', the substring after it should be the filename
    
    // Check that '/' found and it is not the last char
    if ((filename_pos != std::string::npos) &&
        ((filename_pos + 1) < full_path.length())) {
        std::string fn = std::string("Download/") + full_path.substr(filename_pos + 1);
        outfile =
            fopen(fn.c_str(),"wb");
    }
    else {
        outfile = fopen("Download/index.html", "wb");
    }

    return outfile;
}



/** Modular parts of the main program */
void parse_parsed_addrs(char *server_addr, char *proxy_addr, URL *&server_url, 
        URL *&proxy_url, TCP_Socket *client_sock);

void send_GET_request(TCP_Socket *client_sock, URL *server_url, std::string&);
HTTP_Response *recv_response_header(TCP_Socket *client_sock, std::string &response_body);
void recv_response_body(TCP_Socket *client_sock, URL *server_url, HTTP_Response *response,
        std::string &response_body);

int get_chunk_size(std::string &data);
