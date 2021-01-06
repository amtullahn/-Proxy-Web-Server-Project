// Modified from CSE422 FS09
// Modified from CSE422 FS12
// Modified again for CSE422 FS20

#include "Proxy_Worker.h"
#include <sstream>

using namespace std;

/**
 * Constructor.
 *
 * @param cs The socket that is already connected to the requesting client
 */
Proxy_Worker::Proxy_Worker(TCP_Socket *cs) {
    client_sock = cs;
    port = 80; // For a full blown proxy, the server information should be 
               // obtained from each request. However, we simply assume it 
               // to be 80 for our labs.
    server_url = NULL;
               // Must be obtain from each request.
}

/**
 * Deletes all non-null pointers and closes all sockets.
 */
Proxy_Worker::~Proxy_Worker() {
    if (server_url != NULL) {
        delete server_url;
    }

    if (client_request != NULL) {
        delete client_request;
    }

    if (server_response != NULL) {
        delete server_response;
    }
    server_sock.Close();
    client_sock->Close();
}

/******* Private Methods ******/

/**
 * Receives the request from a client and parse it.
 *
 * @return A boolean indicating if getting the request was succesful or not
 */
bool Proxy_Worker::get_request() {
    string url_str;
    try {
        // TODO Get the request from the client (HTTP_Request::receive)
        client_request = HTTP_Request::receive(*client_sock);
        // TODO Check if the request is received correctly

        if (client_request <= 0) {
            cerr << "The request was not received correctly" << endl;
            exit(1);
        }

        // These lines store and remove the client identifier from the header
        // This is used to tag output for the test cases
        client_request->get_header_value("ClientID", client_id);
        client_request->remove_header_field("ClientID");

        // Obtain the server_url from the request (HTTP_Request::get_host 
        //      and HTTP_Request::get_path). url = host + path
        client_request->HTTP_Request::get_host(url_str);

        string getting_path;
        getting_path = client_request->get_path();

        // parse the url using URL::parse
        url_str += getting_path;

        server_url = URL::parse(url_str);
    }
    // Return an internal server error code (this should never happen, 
    // hopefully)
    catch (std::runtime_error &e) {	    
        print_with_pref(e.what(), cerr);	    
        proxy_response(500);	    
        return false;
    }
    // Catch an error that should be returned to the client as a response
    // This should not happen
    catch (unsigned code) {
        proxy_response(code);
        return false;
    }

    return true;
}

/**
 * Check if the request just received is valid
 *
 * @return If the request is valid or not.
 */
bool Proxy_Worker::check_request() {
    // 1. Make sure we're pointing to a server URL
    //    Respond a 404 Not Found if the server is invalid
    //    (That is server_url == NULL)
    // 2. Filter out any "host" with the keyword "facebook"
    //    Note that we are filtering out "host" with "facebook".
    //    "path" with facebook is allowed.
    //    Respond a 403 forbidden for host with facebook.

    // Call get host to get server address
    string server_address;
    client_request->HTTP_Request::get_host(server_address);

    bool address_check;
    // Checking if this is a request to facebook
    if (server_address.find("facebook") != string::npos) {
        address_check = proxy_response(403);
        return false;
    }

    // Grabing a filepath
    string file_path;
    file_path = client_request->get_path();

    
     // Concatenate filepath and server address together
    string url_requested;
    url_requested = server_address + file_path;

    // This checks the validity of server
    server_url = URL::parse(url_requested);

    // Checks if URL object is NULL
    bool url_check;
    if (server_url == NULL) {
        url_check = proxy_response(404);
        return false;
    }

    try {
        // Create a connection to the server url on the server socket
        server_sock.Connect(*server_url);
    }

    catch (string msg) {
        bool valid_address = proxy_response(404);
        return false;
    }

    return true;
}

/**
 * Forwards a client request to the server and get the response
 *      1. Forward the request to the server
 *      2. Receive the response header and modify the server field
 *      3. Receive the response body. Handle both chunk/default encoding.
 *
 * @return A boolean indicating if forwarding the request was succesful or not
 */
bool Proxy_Worker::forward_request_get_response() {
    // creates an HTTP_Request object by invoking the create_GET_request
    // pass client request to the server
    client_request = HTTP_Request::create_GET_request(server_url->get_path());
    client_request->set_host(server_url->get_host());

    // non persistent request - configuring the request
    client_request->set_header_field("Connection","close");
    client_request->set_header_field("If-Modified-Since", "0");

    // connect to the server
    server_sock.Connect(*server_url);
    // sends the request to the server
    client_request->send(server_sock);

    // THIS CODE IMPLEMENTS RECEIVING A RESPONSE HEADER FROM THE SERVER
    // Holds incoming data - since HTTP message is comprised of
    // two portions, we have two string variables
    string header_response;
    string response_body;
    // Receives the response header from the server and parse it 
    server_sock.read_header(header_response,response_body);

    // we are now constructing a response object and checking if it
    // is constructed correctly, it will also determine whether
    // response was chunked or not
    server_response = HTTP_Response::parse(header_response.c_str(),header_response.length());

    // we're checking to see if the response was chunked or not
    // Essentially implementing the identity encoding portion
    if (server_response->is_chunked() == false) {

        int total_content_len = server_response->get_content_length();

        int remaining_bytes = total_content_len - response_body.length();
       
        while (remaining_bytes != 0) {
            // decrement the amount of bytes left to read until it hits zero
            remaining_bytes -= server_sock.read_data(response_body, remaining_bytes);
        }
        // you want to append the data after reading it 
        server_response->append_content(response_body);
    }

    // account for chunked encoding transfer
    else {
        // got this code from client.cc
        int total_data = 0;
        int bytes_written = 0;

        int chunk_len;
        do {
            // Get chunk size
            chunk_len = get_chunk_size(response_body);
             if (chunk_len < 0) {
                try {
                    server_sock.read_line(response_body);
                } catch (string msg) {
                    return false;
                    // warn the others
                    throw msg;
                    
                }
                chunk_len = get_chunk_size(response_body);
            }

            if (chunk_len < 0) {
                throw string("Could not get chunk size!");
            }

            // Loop through read to get entire chunk
            int bytes_left = chunk_len+2 - response_body.length();
            try {
                while (bytes_left > 0)
                    bytes_left -= server_sock.read_data(response_body, bytes_left);
            } catch (string msg) {
                return false;
                // warn the others
                throw msg;
            }

            // Write chunk to file
            stringstream ss;
            ss << hex << chunk_len;
            server_response->append_content(ss.str());
            server_response->append_content("\r\n");
            server_response->append_content(response_body);

            bytes_written += chunk_len;
            total_data += chunk_len; 
            // Remove chunk and ending \r\n
            response_body = response_body.substr(chunk_len+2,
                    response_body.length() - chunk_len - 2);
        } while (chunk_len > 0);
    }
    return true;
}

/**
 * Modify the Server field and return the response from the server to the client
 *          
 * @return A boolean indicating if returning the request was succesful or not
 *      (always true for now)
 */
bool Proxy_Worker::return_response() {
    // TODO Also modify the server field. Change it to whatever you want.
    //      As long as it contains 'CSE422', it is good.
    server_response->set_header_field("Server", "CSE422");

    // just outputting the response, it is interesting to have a look.
    string buffer;
    print_with_pref("Returning response to client ...", cout);
    print_with_pref("==========================================================", cout);

    buffer.clear();
    server_response->print(buffer);
    print_with_pref(buffer.substr(0, buffer.length() - 4), cout);

    print_with_pref("==========================================================", cout);

    // TODO Some code to return the response to the client.
    server_response->send(*client_sock);

    return true;
}

/**
 * Create a response "locally" and return it to a client
 * For error situations like 403, 404, and 500 .. etc
 *
 * @param error The error code
 * @return A boolean indicating if returning the request was succesful
 *      or not (always true for now)
 */
bool Proxy_Worker::proxy_response(int status_code) {
    string buffer;
    HTTP_Response proxy_res(status_code);
    stringstream ss;
    int content_length = int(proxy_res.get_content_length());
    ss << content_length;

    proxy_res.set_header_field("Content-Length", ss.str());

    ss.str("");
    ss.clear();

    ss << "Returning " << status_code << " to client ...";
    print_with_pref(ss.str(), cout);
    print_with_pref("==========================================================", cout);

    buffer.clear();
    proxy_res.print(buffer);

    print_with_pref(buffer.substr(0, buffer.length() - 4), cout);
    print_with_pref("==========================================================", cout);

    proxy_res.send(*client_sock);
    return true;
}

/**
 * Prints the data to the ostream with each line prefixed with the client ID tag.
 * Adds a newline character at the end.
 *
 * @param data The string to print.
 * @param out The out stream to print to.
 */
void Proxy_Worker::print_with_pref(string data, ostream &out) {
    if (client_id.length() == 0) {
        client_id = "?";
    }

    istringstream iss(data);
    string line;
    while (getline(iss, line))
        out << "[" << client_id << "] " << line << endl;
}

/**
 * Extract the chunk size from a string
 *
 * @param the string
 * @return  the chunk size in int
 *          Note that the chunk size in hex is removed from the string.
 */
// You probably will need this function in 
// Proxy_Worker::forward_request_get_response()
// You can find a similar function in client.h
// You can either remove the hex chunk size or leave it in the data string.
// Both is fine. Maybe you dont want to implement this and you are able 
// to extract the chunk size in the function. It is fine.
// If you dont want this, dont forget to remove the function prototype in 
// Proxy_Worker.h.
int Proxy_Worker::get_chunk_size(string &data) {
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

/******* Public Methods *******/

void Proxy_Worker::handle_request() {
    string buffer;

    // Get HTTP request from the client, check if the request is valid by 
    // parsing it. (parsing is done using HTTP_Request::receive)
    // From the parsed request, obtain the server address (in code, 
    // server_url).
    print_with_pref("New connection established.", cout);
    print_with_pref("New proxy child process started.", cout);
    print_with_pref("Getting request from client...", cout);
    
    // Calling get request function (I added this)
    bool request_get = get_request();

    if (!request_get)  {
        // did not get the request, something is wrong, stop this process
        exit(1);
    }  

    // Just outputting the requrst.
    print_with_pref("Received request:", cout);
    print_with_pref("==========================================================", cout);

    client_request->print(buffer);
    print_with_pref(buffer.substr(0, buffer.length() - 4), cout);

    print_with_pref("==========================================================", cout);

    print_with_pref("Checking request...", cout);

    bool request_check = check_request();
    if (!request_check) {
        // request is invalid, something is wrong, stop this process
        exit(1);
    }
    print_with_pref("Done. The request is valid.", cout);

    // Forward the request to the server.
    // Receive the response header and modify the server header field
    // Receive the response body. Handle the default and chunked transfor 
    // encoding.
    print_with_pref("Forwarding request to server...", cout);
    if (!forward_request_get_response()) {
        return;
    }

    print_with_pref("Response header received", cout);

    //return the response to the client
    return_response();
    print_with_pref("Connection served. Proxy child process terminating.", cout);

    return;
}