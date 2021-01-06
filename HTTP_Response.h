// Modified from CSE422 FS09
// Modified from CSE422 FS12
// Modified again for CSE422 FS20

#ifndef _HTTP_RESPONSE_H_
#define _HTTP_RESPONSE_H_

#include "HTTP_Message.h"
#include "TCP_Socket.h"
#include <string>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <sstream>

/**
 * Class representing an HTTP response message.  May be used
 * both to parse existing HTTP response into a comprehensible object, and to
 * construct new responses from scratch and print them out to a text string.
 * Makes no attempt to handle the body of the response -- only the response code
 * and the headers will be captured.
 *
 * If you're planning on servicing GET and HEAD requests only, you can use
 * the Create_standard_response() method to have a lot of headers automatically
 * set up for you.  The HTTP specification mandates these headers, and some
 * clients may expect them.
 *
 * Also see the HTTP_Message class for methods that can be used to query and
 * set the response headers.
 */
class HTTP_Response : public HTTP_Message {
    private:
        unsigned status_code;
        std::string version;
        std::string status_desc;

        std::string content;
        bool chunked;
        int content_len;

        void build_status();
        std::string build_time() const;

    public:
        /**
         * Constructs a new HTTP_Response. Note that nothing is done to check
         * the validity of the arguments -- make sure you trust your input
         * and/or yourself.
         *
         * @param code The code representing the response status (e.g. 200,
         *      403).
         * @param status_desc A one-line textual description of the response.
         * @param version The HTTP version used to transmit the response.
         * @param data The content of the response
         */
        HTTP_Response(unsigned code = 0, const std::string& status_desc = "",
            const std::string& version = "HTTP/1.1", const std::string &data = "");

        virtual ~HTTP_Response();

        /**
         * Parse the response from server to construct a HTTP_Response object.
         * Check if the response is formatted correctly.
         *
         * NOTE: People parse the response differently. The way they slice the
         * header varies as well. In this implementation The header MUST END
         * WITH \r\n\r\n.
         *
         * @param data The received data piece
         * @param length The length of the data
         * @return Pointer to an HTTP_Response object, if this data is good,
         *      NULL otherwise
         */
        static HTTP_Response *parse(const char* data, unsigned length);

        /**
         * Constructs a new HTTP_Response with some mandatory header fields
         * convenienty set for you (unlike the constructor, which sets no
         * fields for you at all). Assumes that you will be sending back some
         * kind of message body, and that the message body will be sent
         * verbatim (i.e. not compressed).  Also assumes that the connection
         * will be closed immediately after the send ends.
         *
         * @param content_length The length of the message body that will be
         *      sent following this response.
         * @param code The code representing the response status (e.g. 500).
         * @param status_desc A short description of the response code's
         *      meaning.
         * @param version The HTTP version used to transmit the response.
         * @return Pointer to the created HTTP_Response created for the given
         *      input, containing all of the mandatory headers.
         */
        static HTTP_Response* create_standard_response(unsigned content_length,
            unsigned code = 0,
            const std::string& status_desc = "",
            const std::string& version = "HTTP/1.1");

        /**
         * Extract the "Content-Length" from the header.
         *
         * @return The content length as int
         */
        const int get_content_length() const;

        /**
         * Looks up the version of the HTTP response (e.g. HTTP/1.1).
         * 
         * @return The response's HTTP version.
         */
        const std::string& get_version() const;

        /**
         * Looks up the status code of the HTTP response (e.g. 404, 500).
         * 
         * @return The response's status code.
         */
        unsigned get_status_code() const;

        /**
         * Looks up the description of the response (e.g. "OK").
         *
         * @return The response's associated status_desc string.
         */
        const std::string& get_status_desc() const;

        /**
         * Looks up the content length of the response body
         *
         * @return the content length
         */
        const int get_content_len() const;

        /**
         * Looks up the content, the response body
         *
         * @return The content as a string
         */
        const std::string &get_content() const;

        /**
         * Appends new content to existing content.
         *
         * @param new_content The new content to append.
         */
        void append_content(const std::string& new_content);

        /**
         * Clears the content.
         */
        void clear_content();

        /**
         * Looks up if the response is chunked encoding transfer
         * 
         * @return true if this response is chunked, false otherwise
         */
        const bool is_chunked() const;

        /**
         * Prints the response object to a text string, suitable for sending to
         * an HTTP client.  Includes the terminating blank line and all
         * response headers.
         *
         * @param[out] output_string Will be set to the response text.
         */
        void print(std::string& output_string) const;

        /**
         * Prints the response object to a text string, suitable for sending to
         * an HTTP client.  Includes the terminating blank line and all
         * response headers.
         *
         * @param[out] output_buffer The text buffer into which the response
         *      should be printed.  Will be null-terminated.
         * @param[in] buffer_length The number of characters available for
         *      writing in the buffer.  Printing stops after this many
         *      characters have been written.
         */
        void print(char* output_buffer, unsigned buffer_length) const;

        /**
         * Sets the HTTP version of the response (e.g. HTTP/1.1).
         * 
         * @param[out] version The version to set.
         */
        void set_version(const std::string& version);

        /**
         * Sets the status code to indicate in the response.
         *
         * @param code The HTTP status code to set.
         */
        void set_status_code(const unsigned code);

        /**
         * Sets the status_desc for the given status code being sent.
         *
         * @param status_desc The text string to set as the response's
         *      description.
         */
        void set_status_desc(const std::string& status_desc);

        /**
         * Send this response to this TCP socket sock
         *
         * @param sock the socket to send to
         * @return Number of bytes sent.
         */
        int send(TCP_Socket &sock);
};

#endif // ndef _HTTP_RESPONSE_H_
