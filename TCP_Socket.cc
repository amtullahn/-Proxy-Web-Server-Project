// Modified from CSE422 FS09
// Modified from CSE422 FS12
// Modified from CSE422 FS13 for FS20

#include "TCP_Socket.h"
#include <sstream>

using namespace std;

/******* Constructor/Destructor *******/

TCP_Socket::TCP_Socket(): sock(-1) {}

TCP_Socket::~TCP_Socket() {
    Close();
}

/****** Private Methods ***************/

/**
 * Creates the socket, first closing it in case it was already open.
 *
 * @throws std::string on system error
 */
void TCP_Socket::create_socket()  {
    // close the socket if it's already open
    Close();

    // first try to make the TCP socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        throw string("TCP_Socket Exception: Unable to create socket");
    }
}

/**
 * Calls C read on sock.
 *
 * @param[out] buffer The location to put bytes read from the socket.
 * @param[in] num_bytes The number of bytes to read.
 * @return The number of bytes actually read
 */
int TCP_Socket::v_read(void *buffer, int num_bytes) {
    return read(sock, buffer, num_bytes);
}

/**
 * Calls C write on sock.
 *
 * @param buffer The location to write from.
 * @param num_bytes The number of bytes to read.
 * @return The number of bytes actually written.
 */
int TCP_Socket::v_write(void *buffer, int num_bytes) {
    return write(sock, buffer, num_bytes);
}

/*
 * Read n bytes from the socket
 *
 * @param[out] vptr The pointer to the buffer that will be used to hold the data
 * @param[in] n The number of bytes to be read
 * @return  The number of bytes read
 */
int TCP_Socket::read_n_bytes(void *vptr, int n) {
    size_t  n_left;
    ssize_t n_read;
    char    *ptr;

    ptr = (char *) vptr;
    n_left = n;

    while(n_left > 0) { // keeps reading until n is satisfied
        if((n_read = v_read(ptr, n_left)) < 0) {
            return -1;
        }
        else if(n_read == 0) {
            break;
        }
        n_left -= n_read;
        ptr    += n_read;
    }

    return (n - n_left);
}

/*
 * Read from the socket until a newline char is found.
 *
 * @param[out] vptr The pointer to the buffer that will hold the data
 * @return  the number of bytes received.
 */
int TCP_Socket::read_line(void *vptr, int maxlen) {
    int n, read_count;
    char c, *ptr;

    ptr = (char *) vptr;
    for(n = 1; n < maxlen; n++) {
        read_count = v_read(&c, 1);
        // Keeps receiving, one byte by one byte
        if(read_count == 1) {
            *ptr++ = c;
            if(c == '\n') { // check if the byte is newline
                break; // break and end this function is yes
            }
        }
        else if(read_count == 0) {
            if(n == 1) {
                return 0;
            }
            else {
                return n;
            }
        }
        else { // read_count < 0
            return -1;
        }
    }
    *ptr = 0;
    return n;
}

/*
 * Read from the socket until \r\n\r\n is captured. The purpose
 * of this function is to receive HTTP message headers
 *
 * @param[out] buffer The buffer to hold the data
 * @param[in] buffer_len The maximum length of the buffer
 * @param[out] total_received_len The total number of data received
 * @return The end position of the HTTP message header.
 */

int TCP_Socket::receive_response_headers(char *buffer, int buffer_len,
                             int &total_received_len) {
    static const char header_end[] = {'\r', '\n', '\r', '\n'};
    static const unsigned header_end_len = sizeof(header_end);

    // Piece-by-piece, buffer the server's response and look for the end
    // of the headers. Make a note of where in the buffer the end occurs.
    int bytes_recv = 0;
    int header_end_pos = -1;
    int header_end_read = 0;

    while((bytes_recv < buffer_len) && (header_end_pos < 0)) {
        // Grab however many bytes are waiting for us right now.
        int recv_len = v_read(buffer + bytes_recv, buffer_len - bytes_recv); 
        if(recv_len == -1) {
            // Something's wrong. If we cannot receive, reutrn -1
            return -1;
        }

        // Go over what we got in the buffer and look for the end of headers
        int i;
        for(i = bytes_recv; 
            (i < (bytes_recv + recv_len)) &&
            (header_end_read < header_end_len);
            i++) {
            if(buffer[i] == header_end[header_end_read]) {
                header_end_read++;
            }
            else {
                header_end_read = 0;
            }
        }

        // If we found the end, mark it.    Also keep track of how much
        // we've read total, for several reasons (not filling the
        // buffer; knowing how much we've read past the header, etc.).    
        if(header_end_read >= header_end_len) {
            header_end_pos = i;
        }
        bytes_recv += recv_len;
    }
    total_received_len = bytes_recv;

    return header_end_pos; 
    // Note that this header_end_pos here includes \r\n\r\n
}

/****** Public Methods ****************/

void TCP_Socket::Connect(hostent *host, int server_port) {
    // create the socket
    create_socket();

    // make sure it's zero to start
    memset(&server_addr, 0, sizeof(server_addr));
    // designate it as part of the Internet address family
    server_addr.sin_family = AF_INET;
    // specify the port
    server_addr.sin_port = htons(server_port);
    // specify the server IP address in network byte order
    memcpy(&server_addr.sin_addr, host->h_addr, host->h_length);

    // now actually try to Connect
    if(connect(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        throw string("TCP_Socket Exception: Connect failed");
    }
}

void TCP_Socket::Connect(string &server_name, int server_port) {
    hostent *host;

    // convert the server name to a valid inet address
    if ((host = gethostbyname(server_name.c_str())) == NULL) {
        throw string("TCP_Socket Exception: could not resolve hostname");
    }

    Connect(host, server_port);
}

void TCP_Socket::Connect(const URL &url) {
    // If the port is not defined, connect to 80
    int server_port = 80;
    if(url.is_port_defined()) {
        server_port = url.get_port();
    }

    string hostname = url.get_host();
    Connect(hostname, server_port);
}

void TCP_Socket::Bind(int server_port) {
    // create the socket
    create_socket();

    // make sure it's zero to start
    memset(&server_addr, 0, sizeof(server_addr));
    // designate it as part of the Internet address family
    server_addr.sin_family = AF_INET;
    // specify the port
    server_addr.sin_port = htons(server_port);
    // specify the server IP address in network byte order
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(sock, (sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        throw string("TCP_Socket Exception: could not bind to interface");
    }

    socklen_t server_addr_len = sizeof(server_addr);
    if(getsockname(sock, (sockaddr *) &server_addr,
                   &server_addr_len) < 0) {
        throw string("TCP_Socket Exception: Unable to obtain TCP socket information");
    }
}

void TCP_Socket::Listen() {
    // listen on socket sock
    // report error when fail
    if(listen(sock, 1) < 0) {
       throw string("TCP_Socket Exception: listen call failed");
    }
}

void TCP_Socket::Accept(TCP_Socket &data_sock) {
    int new_sock;
    socklen_t sin_size;

    sin_size = sizeof(struct sockaddr_in);
    // waiting for new incoming connection
    if((new_sock = accept(sock, (struct sockaddr *) &(data_sock.server_addr), &sin_size)) < 0) {
        throw string("TCP_Socket Exception: could not accept incoming Connection");
    }

    data_sock.sock = new_sock;
}

TCP_Socket * TCP_Socket::Accept() {
    int new_sock;
    socklen_t sin_size;
    // The new TCP_Socket object to hold the connection socket
    TCP_Socket *data_sock = new TCP_Socket();
    try {
        Accept(*data_sock);
    } catch (string msg) {
        delete data_sock;
        throw msg;
    }

    return data_sock;
}

void TCP_Socket::Close() {
    // If this socket is in use
    if(sock != -1) {
        if(close(sock) < 0) {
            throw string("TCP_Socket Exception: could not close socket");
        }
        sock = -1;
    }
}

int TCP_Socket::write_string(string &data) {
    int l = 0;

    if ((l = v_write((void *)data.data(), data.size())) < 0) {
        throw string("TCP_Socket Exception: error sending data");
    }

    return l;
}

int TCP_Socket::read_string(string &data) {
    int l;

    if((l = v_read((void *)data.data(), data.size())) < 0) {
        throw string("TCP_Socket Exception: error reading data from socket");
    }
    data = data.substr(0, l);
    data += '\0';

    return l;
}

// One can check if the header is good by checking the length of header.
void TCP_Socket::read_header(string &header, string &data) {
    char buffer[BUFFER_SIZE];
    int l, total = 0;

    int header_end_pos = receive_response_headers
                         (buffer, BUFFER_SIZE - 1, total);

    if(header_end_pos < 0) {
        throw string("TCP_Socket Exception: Error receiving response header from server.");
    }

    else {
        // Store the received header and data into 
        header.append(buffer, header_end_pos);
        data.append(buffer + header_end_pos, total - header_end_pos);
    }
}


int TCP_Socket::read_data(string &data, int bytes_left) {
    int total = 0, l;
    char buffer[BUFFER_SIZE];

    // limit the number of bytes read to the buffer size
    bytes_left = (bytes_left < BUFFER_SIZE) ? bytes_left : BUFFER_SIZE;

    while(total < bytes_left) {
        memset(buffer, 0, sizeof(buffer));

        l = read_n_bytes(buffer, bytes_left);

        if(l < 0) {
            throw string("TCP_Socket Exception: error reading all data from socket");
            return -1;
        }
        else if(l == 0) {
            break;
        }

        data.append(buffer, l);
        total += l;
    }

    return total;
}

int TCP_Socket::read_line(string &data) {
    char buffer[BUFFER_SIZE];
    int l;

    if((l = read_line(buffer, BUFFER_SIZE)) < 0) {
        throw string("TCP_Socket Exception: error reading line from socket");
    }

    buffer[l] = 0;
    data += buffer;

    return l;
}

void TCP_Socket::get_port(unsigned short &getting_port) {
    getting_port = ntohs(server_addr.sin_port);
}

