// Modified from CSE422 FS09
// Modified from CSE422 FS12

// Simple HTTP client for CSE422 F13 lab 02.

#include <climits>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <netdb.h>
#include <string>

#include "proj2_client.h"
#include "HTTP_Request.h"
#include "HTTP_Response.h"
#include "TCP_Socket.h"

using namespace std;

/**
 * Simple HTTP client program which sends/receives an HTTP request/response from a
 * server.  If the proxy url is identified, it will send its request there.
 *
 * @param argc, argv The command line arguments
 */
int main(int argc, char* argv[]) {
    char*           server_addr = NULL;
    char*           proxy_addr  = NULL;
    URL*            server_url  = NULL;
    URL*            proxy_url   = NULL;
    HTTP_Response*  response    = NULL;

    TCP_Socket      client_sock;
    string          print_buffer;

    char*           client_id_c = NULL;
    string          client_id;
    
    parse_args(argc, argv, &server_addr, &proxy_addr, &client_id_c);
    client_id = string(client_id_c);

    parse_parsed_addrs(server_addr, proxy_addr, server_url, proxy_url, &client_sock);

    try {
        send_GET_request(&client_sock, server_url, client_id);

        string response_body;
        response = recv_response_header(&client_sock, response_body);

        recv_response_body(&client_sock, server_url, response, response_body);
    } catch (string msg) {
        cerr << msg << endl;

        // Clean up resources
        if (server_url != NULL) delete server_url;
        if (proxy_url != NULL) delete proxy_url;
        if (response != NULL) delete response;
        client_sock.Close();
    }

    // If the response is not OK, something is wrong.
    // However, we still downloaded the content, because even if the response
    // is not 200, the server still replies with an error page (403, 404 ...)
    if(response->get_status_code() != 200) {
        cerr << response->get_status_code() << " " 
             << response->get_status_desc() << endl;
    }

    // everything's done.
    client_sock.Close();

    delete response;
    delete server_url;
    if(proxy_addr != NULL) {
        delete proxy_url;
    }

    return 0;
}

/**
 * Parses the addr strings to create the corrsponding URL object when
 * appropriate and the corresponding socket.  Note that the server_addr must
 * be non-NULL as the HTTP GET will need to end up there, regardless of the
 * proxy.  This function will print an error message and exit if given url
 * strings are not formatted correctly, no server_url is given, or if the
 * socket cannot be created.
 *
 * @param[in] server_addr A string indicating the server address.
 * @param[in] proxy_addr A string indicating the proxy address.
 * @param[out] server_url The location where the serverl URL will be placed
 *      when created.
 * @param[out] proxy_url The location where the proxy URL will be placed, if
 *      created.
 * @param[out] client_sock TCP_Socket that will be Connect()-ed to the
 *      server/proxy.
 */
void parse_parsed_addrs(char *server_addr, char *proxy_addr, URL *&server_url, 
        URL *&proxy_url, TCP_Socket *client_sock) {
    // Must have a server to get data from
    if(server_addr == NULL) {
        cout << "You did not specify the host address." << endl;
        help_message("client", cout);
        exit(1);
    }

    server_url = URL::parse(server_addr);
    if(server_url == NULL) { // If URL parsing is failed
        cout << "Unable to parse host address: " << server_addr << endl;
        cout << endl;
        help_message("client", cout);
        exit(1);
    }


    // Proxy is an otional argument
    // If a proxy URL is specified, the client connects to the proxy and
    // downlods from the proxy. Otherwise, the client connects to the host 
    // and downloads from the host.
    if (proxy_addr != NULL) {
        proxy_url = URL::parse(proxy_addr);
        if(proxy_url == NULL) {
            cout << "Unable to parse proxy address: " << proxy_addr << endl;
            cout << endl;
            help_message("client", cout);
            exit(1);
        }
        if(proxy_url->is_port_defined() == false) {
            cout << "Proxy port is not defined, assumed to be 8080" << endl;
            proxy_url->set_port(8080);
        }
    }

    // TCP_Socket class to handle TCP communications.
    if(proxy_addr == NULL) {
    // If proxy is not specified, connect to the host directly.
        try{
            // Connect to the target server.
            client_sock->Connect(*server_url);
        }
        // Give up if sock is not created correctly.
        catch(string msg) {
            cout << msg << endl;
            cout << "Unable to connect to server: " 
                 << server_url->get_host() << endl;
            delete server_url;
            exit(1);
        }
    } else { // proxy is specified, connect to proxy
        try {
            // Connect to the proxy, instead of server
            client_sock->Connect(*proxy_url);
        }
        // Give up if sock is not created correctly.
        catch(string msg) {
            cout << msg << endl;
            cout << "Unable to connect to proxy: " 
                 << proxy_url->get_host() << endl;
            delete server_url;
            delete proxy_url;
            exit(1);
        }
    }
}

/**
 * Sends a GET request via the passed TCP_Socket passed according to the given
 * URL.
 *
 * @param client_sock Socket to send the message on.
 * @param server_url Denotes the HTTP GET's final destination and what to grab.
 *
 * @throws string Error message if the TCP_Socket throws an error.
 */
void send_GET_request(TCP_Socket *client_sock, URL *server_url, string &client_id) {
    string print_buffer; 
    HTTP_Request *request;

    // Send a GET request for the specified file.
    // No matter connecting to the server or the proxy, the request is 
    // alwasy destined to the server.
    stringstream ss;
    ss << server_url->get_path(); // << "?" << server_url->get_query();
    request = HTTP_Request::create_GET_request(ss.str());
    request->set_host(server_url->get_host());
    // This request is non-persistent.
    request->set_header_field("Connection", "close");
    // For real browsers, If-Modified-Since field is always set.
    // if the local object is the latest copy, the browser does not 
    // respond the object.
    request->set_header_field("If-Modified-Since", "0");
    request->set_header_field("ClientID", client_id);

    try  {   
        request->send(*client_sock);
    }   
    catch(string msg) {
        delete request; // clean up local resource
        throw msg; // warn others
    }   

    // output the request
    cout << "Request sent..." << endl;
    cout << "==========================================================" 
         << endl;
    request->print(print_buffer);
    cout << print_buffer.substr(0, print_buffer.length() - 4) << endl;
    cout << "==========================================================" 
         << endl;

    delete request; // We do not need it anymore
}

/**
 * Receives the response header (and part of the body).
 *
 * @param[in] client_sock The socket connected to the server.
 * @param[out] response_body A string to hold the part of the body received when the
 *      headers are read.
 * @return The HTTP_Response built from the received headers.
 * @throws string Error message if the TCP_Socket throws an error.
 */
HTTP_Response *recv_response_header(TCP_Socket *client_sock, string &response_body) {
    string print_buffer, response_header;
    HTTP_Response *response;

    // Read enough of the server's response to get all of the headers,
    // then have that response interpreted so we at least know what
    // happened.
    
    client_sock->read_header(response_header, response_body);
    response = HTTP_Response::parse(response_header.c_str(), 
                                    response_header.length());

    if(response != NULL) {
        // output the response header
        cout << endl << "Response header received" << endl;
        cout << "=========================================================="
             << endl;
        response->print(print_buffer);
        cout << print_buffer.substr(0, print_buffer.length() - 4) << endl;
        cout << "==========================================================" 
             << endl;
    
    } else {
        throw string("Unable to parse the response header.");
    }

    return response;
}

/**
 * Receive the response body from the given socket and writes it to a file.
 * 
 * @param client_sock The socket connected to the client.
 * @param server_url The url containing the file name (to open the corresponding file)
 * @param response The HTTP response header containing information about the transfer
 *      encoding.
 * @param response_body A string containing a part of the body already received.
 * @throws string Error message if the TCP_Socket throws an error.
 */
void recv_response_body(TCP_Socket *client_sock, URL *server_url, HTTP_Response *response,
        string &response_body) {
    FILE* out = NULL;

    // Open a local copy in which to store the file.
    out = open_local(server_url);
    // check
    if(!out) {
        cout << "Error opening local copy for writing." << endl;
        // clean up if failed
        exit(1);
    }

    cout << endl << "Downloading rest of the file ... " << endl;

    int bytes_written = 0, bytes_left, chunk_len;
    int total_data;

    if(response->is_chunked() == false) {
        // none-chunked encoding transfer does not split the data into 
        // chunks. The header specifies a content_length. The client knows 
        // exactly how many data it is expecting. The client keeps receiving
        // the response until it gets the amount.

        cout << "Default encoding transfer" << endl;
        cout << "Content-length: " << response->get_content_len() << endl;
        bytes_left = response->get_content_len();
        do {
            // If we got a piece of the file in our buffer for the headers,
            // have that piece written out to the file, so we don't lose it.
            fwrite(response_body.c_str(), 1, response_body.length(), out);
            bytes_written += response_body.length();
            bytes_left -= response_body.length();
            //cout << "bytes written:" <<  bytes_written << endl;
            //cout << "data gotten:" <<  response_body.length() << endl;

            response_body.clear();
            try {
                // Keeps receiving until it gets the amount it expects.
                client_sock->read_data(response_body, bytes_left);
            }
            catch(string msg) {
                // something bad happend, clean up
                fclose(out);
                // warn the others
                throw msg;
            }
        } while (bytes_left > 0);
    }
    else { // chunked encoding
        cout << "Chunked encoding transfer" << endl;

        // As mentioned above, read_header function already split the
        // data from the header from us. The beginning of this respnse_data
        // now holds the first chunk size.
        total_data = 0;

        int chunk_len;
        do {
            // Get chunk size
            chunk_len = get_chunk_size(response_body);
            if (chunk_len < 0) {
                try {
                    client_sock->read_line(response_body);
                } catch (string msg) {
                    // something bad happend, clean up
                    fclose(out);
                    // warn the others
                    throw msg;
                }
                chunk_len = get_chunk_size(response_body);
            }

            if (chunk_len < 0) {
                fclose(out);
                throw string("Could not get chunk size!");
            }

            cout << "chunk length: " << chunk_len<< endl;

            // Loop through read to get entire chunk
            bytes_left = chunk_len+2 - response_body.length();
            try {
                while (bytes_left > 0)
                    bytes_left -= client_sock->read_data(response_body, bytes_left);
            } catch (string msg) {
                // something bad happend, clean up
                fclose(out);
                // warn the others
                throw msg;
            }

            // Write chunk to file
            fwrite(response_body.c_str(), 1, chunk_len, out);
            bytes_written += chunk_len;
            total_data += chunk_len; 

            // Remove chunk and ending \r\n
            response_body = response_body.substr(chunk_len+2,
                    response_body.length() - chunk_len - 2);
        } while (chunk_len > 0);
    }
    fclose(out);

    cout << endl << "Download complete (" << bytes_written;
    cout << " bytes written)" << endl;
   
    // This checks if the chunked encoding transfer mode is downloading
    // the contents correctly. -- THIS IS BOGUS
    if((total_data != bytes_written) && response->is_chunked() == true) {
        cout << "WARNING" << endl 
             << "Data received does not match chunk size." << endl;
    }
}

/**
 * For a given data, try to extract the chunk_len.  Fails if the chunk size is
 * not in the string.
 *
 * @param data The data as string
 * @return The extracted chunk length if possible, or -1 otherwise.
 */
// For the client, it needs to remove the chunk_len from the data string
// because the client is storing the data as a file. The chunk length
// is no longer needed anymore. However, for proxies, they need to keep the 
// chunk length, so that the forwarded response body can be decoded/received
// by the clients.
int get_chunk_size(std::string &data) {
    int chunk_len = -1;     // The value we want to obtain
    int chunk_len_str_end;  // The var to hold the end of chunk length string
    std::stringstream ss;   // For hex to in conversion

    chunk_len_str_end = data.find("\r\n"); // Find the first CLRF
    if (chunk_len_str_end != std::string::npos) {
        // take the chunk length string out
        std::string chunk_len_str = data.substr(0, chunk_len_str_end);

        // convert the chunk length string hex to int
        ss << std::hex << chunk_len_str;
        ss >> chunk_len;

        // reorganize the data
        // remove the chunk length string and the CLRF
        data = data.substr(chunk_len_str_end + 2, data.length() - chunk_len_str_end - 2);
    }

    return chunk_len;
}
