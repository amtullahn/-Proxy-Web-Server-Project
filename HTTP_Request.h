// Modified from CSE422 FS09
// Modified from CSE422 FS12
// Modified again for CSE422 FS20

#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include "HTTP_Message.h"
#include "TCP_Socket.h"
#include <string>

/**
 * Class representing an HTTP request message.  May be used both
 * to parse an existing HTTP request into a comprehensible object, and to
 * construct new requests from scratch and print them out to a text string.
 * Makes no attempt to handle the body of the request -- only the request line
 * and the headers will be captured.
 *
 * If all you want to do is download a file, call create_GET_request() with
 * the path of the file that you want to download, and then call set_host()
 * on the returned object with the hostname of the server from which you'll
 * be downloading.  You should then be able to Print() the request out to a
 * character buffer to get something that the server will accept.
 *
 * Also see the HTTP_Message class for methods that can be used to query and
 * set the request's headers.
 */
class HTTP_Request : public HTTP_Message {
    private:
        std::string method;
        std::string url;
        std::string version;

    public:
        /**
         * Constructor, constructs a new HTTP_Request. Note that nothing is
         * done to check the validity of the arguments -- make sure you trust
         * your input.
         *
         * @param method The action being requested (e.g. GET, POST, etc).
         * @param url_str The URL of the resource to which the request applies.
         *      In most cases, this will typically just be the path of the
         *      resource on the server (e.g. /somedir/something.txt).
         * @param version The HTTP version of the client making the request.
         *      Default is HTTP 1.1 (which ought to be what you support).
         */
        HTTP_Request(const std::string& method = "",
            const std::string& url_str = "",
            const std::string& version = "HTTP/1.1");

        virtual ~HTTP_Request();

        /**
         * Constructs an HTTP_Request object corresponding to the actual
         * request text in the given buffer. Use this if you've received a
         * request and want to know what it's asking.
         *
         * @param data The text buffer in which the request is stored.
         * @param length The length of the request data, in bytes.
         * @return An HTTP_Request parsed from the request text. If parsing
         *      fails, a NULL pointer will be returned instead.
         */
        static HTTP_Request* parse(const char* data, unsigned length);

        /**
         * Constructs a new HTTP GET request, with a header or two set to make
         * it more likely that the server will return an easy-to-handle result.
         *
         * @param url The URL of the resource to get.
         * @param version The HTTP version to associate with the request.
         * @return A new HTTP_Request object for the GET request.
         */
        static HTTP_Request* create_GET_request(const std::string& url ="",
            const std::string& version = "HTTP/1.1");

        /**
         * Send this request to the socket sock.
         *
         * @param sock The socket we want to send to
         */
        void send(TCP_Socket &sock);

        /**
         * Receive data from the socket until CLRF and create an HTTP_Request
         * object by parsing that piece of data.
         * 
         * @param sock The socket we want to receive from
         * @return Pointer to the created HTTP_Request object
         */
        static HTTP_Request *receive(TCP_Socket &);

        /**
         * Looks up the method of the request (e.g. GET, PUT, DELETE).
         *
         * @return The request's method.
         */
        const std::string& get_method() const;

        /**
         * Looks up the path targeted by the request (e.g. /stuff.txt).
         * 
         * @return The request's path.
         */
        const std::string& get_path() const;

        /**
         * Looks up the HTTP version of the requesting client (e.g. HTTP/1.1).
         * 
         * @return The request's HTTP version.
         */
        const std::string& get_version() const;

        /**
         * Looks up the host for which the request is intended, from the 
         * request's Host header.
         *
         * @param[out] out_host Will be set to the request's target host.  If
         *      the host has not yet been entered, it will be set to a blank
         *      string.
         */
        void get_host(std::string& out_host) const;

        /**
         * Prints the request object to a text string, suitable for
         * transmission to an HTTP server. Includes the terminating blank line
         * and all request headers.
         *
         * @param[out] output_string Will be set to the request text.
         */
        void print(std::string& output_string) const;

        /**
         * Prints the request object to a text string, suitable for transmission
         * to an HTTP server.  Includes the terminating blank line and all
         * request headers.
         *
         * @param[out] output_buffer The text buffer into which the request
         *      should be printed.  Will be null-terminated.
         * @param[in] buffer_length The number of characters available for 
         *      writing in the buffer.  Printing stops after this many
         *      characters have been written.
         */
        void print(char* output_buffer, unsigned buffer_length) const;

        /**
         * Sets the method of the HTTP request (e.g. GET, PUT, DELETE).
         *
         * @param method The method to set for the request.
         */
        void set_method(const std::string& method);

        /**
         * Sets the URL that the request should target (e.g. /stuff.txt).
         *
         * @param url The URL to set for the request.
         */
        void set_url(const std::string& url);

        /**
         * Sets the HTTP version supported by the request's client.
         * 
         * @param version The HTTP version the request should indicate.
         */
        void set_version(const std::string& version);

        /**
         * Sets the host for which the request is intended, into the request's
         * Host header.
         *
         * @param host The host to set for the request.
         */
        void set_host(const std::string& host);
};

#endif //ndef _HTTP_REQUEST_H_
