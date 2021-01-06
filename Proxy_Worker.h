// Modified from CSE422 FS09
// Modified from CSE422 FS12
// Modified again for CSE422 FS20

#ifndef _PROXYWORKER_H_
#define _PROXYWORKER_H_

#include <iostream>
#include <string>

#include "HTTP_Response.h"
#include "HTTP_Request.h"
#include "TCP_Socket.h"
#include "URL.h"



/**
 * Acts as a single worker to handle a request from a proxy. It 
 * does not cache anything in this lab. It just pass the requst
 * to the server and return the response to the client.
 */
class Proxy_Worker {
private:
    URL                *server_url;      // Server's URL, obtained from
                                         // each request.
    unsigned short int port;             // For a full blown proxy

    TCP_Socket         *client_sock;     // The socket for client
    TCP_Socket         server_sock;
    HTTP_Request       *client_request;  // Obj to handle client request
    HTTP_Response      *server_response; // Obj to handle server response

    std::string         client_id;       // Identifies the client

    // Details for these functions in Proxy_Worker.cc
    bool get_request();
    bool check_request();
    bool forward_request_get_response();
    bool return_response();
    bool proxy_response(int);
    int get_chunk_size(std::string &data);

    void print_with_pref(std::string, std::ostream &);

public:
    Proxy_Worker(TCP_Socket *);
    ~Proxy_Worker();

    /**
     * Handles the next request from the client on the other side of the
     * TCP_Socket.  This receives the request, filter's out hosts containing
     * "facebook", and if valid sends off the request to the server and
     * forwards the response back to the client.
     *
     * If the URL cannot be found, the request doesn't pass the filter, or
     * there is some other unexpected error, an appropriate response is returned
     * to the client.
     */
    void handle_request();
};

#endif
