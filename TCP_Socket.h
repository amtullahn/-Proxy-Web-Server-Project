// Modified from CSE422 FS09
// Modified from CSE422 FS12
// Modified from CSE422 FS13 for FS20

#ifndef _TCPSOCKET_H_
#define _TCPSOCKET_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "URL.h"

/**
 * Wraps the functionality of a TCP socket in a class. Errors are returned by
 * throwing exceptions when they occur.
 *
 * Socket creation is done automatically, so normally you would only need to
 * use Connect(), Bind(), Listen(), Accept(), Close() for setup.
 *
 * There are a few helper commands for reading/writing data from/to the socket.
 * Namely read_line(), read_header(), read_data().
 */
class TCP_Socket {
    private:
        struct sockaddr_in server_addr;

        virtual int v_read(void *buffer, int num_bytes);
        virtual int v_write(void *buffer, int num_bytes);

        int read_n_bytes(void *vptr, int n);
        int read_line(void *vptr, int maxlen);
        int receive_response_headers(char *buffer, int buffer_len,
                int &total_received_len);

    protected:
        int sock;
        virtual void create_socket();

    public:
        TCP_Socket();
        ~TCP_Socket();

        /**
         * Creates socket and connects to server TCP socket.
         *
         * @param host address of the server
         * @param server_port port number on the server
         *
         * @throws std::string Thrown on system error.
         */
        virtual void Connect(hostent *host, int server_port);

        /**
         * Creates socket and connects to server TCP socket.
         *
         * @param server_name The hostname of server
         * @param server_port The port number on the server
         *
         * @throws std::string Thrown on system error.
         */
        void Connect(std::string &server_name, int server_port);


        /**
         * Creates socket and connects to server TCP socket.
         *
         * @param url URL object identifying server name and port
         *
         * @throws str::string Thrown on system error.
         */
        void Connect(const URL &url);

        /**
         * Creates socket and binds it with given port (address to IN_ADDR_ANY)
         * gets port number (if set by OS).
         *
         * @param server_port Port number used to bind
         * 
         * @throws str::string Thrown on system error.
         */
        virtual void Bind(int server_port);

        /**
         * Initiates listening on a bound socket.
         *
         * @throws str::string Thrown on system error.
         */
        virtual void Listen();

        /**
         * Accepts a connection waiting on a bound port and passes it to given
         * TCP_Socket.
         *
         * @param[out] data_sock TCP_Socket that will hold the new connnection
         *
         * @throws str::string Thrown on system error.
         */
        virtual void Accept(TCP_Socket &data_sock);

        /**
         * Accepts a connection waiting on a bound port and returns the new
         * TCP_Socket object.
         * 
         * @return  The new TCP_Socket holding the connection
         * 
         * @throws str::string Thrown on system error.
         */
        TCP_Socket *Accept();

        /**
         * Closes socket, only if it's open.
         *
         * @throws str::string Thrown on system error.
         */
        virtual void Close();

        /**
         * Writes/Sends a string with a given length on this socket.
         *
         * @param data The string to write
         * @return The number of bytes sent
         * 
         * @throws str::string Thrown on system error.
         */
        int write_string(std::string &data);

        /**
         * Reads/Recvs to a string of a given length on this socket.
         *
         * @param[out] data The string to hold result
         * @return The number of bytes received
         * 
         * @throws str::string Thrown on system error.
         */
        int read_string(std::string &data);

        /**
         * Read from socket until \r\n\r\n, storing the header and data parts
         * in different variables.
         *
         * @param[out] header The string to store the header
         * @param[out] data The string to store part received after \r\n\r\n
         * 
         * @throws str::string Thrown on system error.
         */
        void read_header(std::string &header, std::string &data);

        /**
         * Read given numger of bytes from socket and store in data.
         *
         * @param[out] data The string to store data
         * @param[in] bytes_left int, number of bytes to receive
         * @return The number of bytes actually read
         * 
         * @throws str::string Thrown on system error.
         */
        int read_data(std::string &data, int bytes_left);

        /**
         * Read until \n (inclusive).
         *
         * @param[out] data The string to store data
         * @return The number of bytes actually read
         * 
         * @throws str::string Thrown on system error.
         */
        int read_line(std::string &data);

        /**
         * Return the port number.
         *
         * @param[out] getting_port Integer to hold the port number.
         */
        void get_port(unsigned short int &getting_port);
};

#endif
