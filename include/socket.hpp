
#ifndef NETWORK_CLIENT_SOCKET
#define NETWORK_CLIENT_SOCKET

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <mutex>
#include <string>
#include <vector>

namespace network {
    enum create_response {
        CREATE_SSL_CTX_FAIL,
        CREATE_FAIL,
        CREATE_SUCCESS,
    };
    enum connect_response {
        CONNECT_BAD_IP,
        CONNECT_FAIL,
        CONNECT_DNS_LOOKUP_FAIL,
        CONNECT_SSL_CREATE_FAIL,
        CONNECT_SSL_SET_FD_FAIL,
        CONNECT_SSL_HANDSHAKE_FAIL,
        CONNECT_SSL_CTX_NOT_INITIALIZED,
        CONNECT_SUCCESS,
    };
    enum receive_response {
        RECEIVE_NOT_CONNECTED,
        RECEIVE_CLOSED,
        RECEIVE_TIMEOUT,
        RECEIVE_SELECT_ERR,
        RECEIVE_ERROR,
        RECEIVE_SUCCESS,
    };
    enum send_response {
        SEND_NOT_CONNECTED,
        SEND_ERR,
        SEND_SUCCESS,
    };

    struct dns_host{
        std::string host;
        bool success;
        int err;

        dns_host() : host{}, success{false}, err{-1} {};
        ~dns_host(){}
    };

    class socket_base {
        protected:
            dns_host host;
            std::string hostname;
            int port;
            int socket_fd;

        public:
            socket_base(const std::string &hostname, int port);
            virtual ~socket_base() = 0;

            const dns_host get_dns_host() const;
            const std::string get_hostname() const;
            send_response send_data(const std::vector<uint8_t> &data, int& err);
            send_response send_string(const std::string &data, int &err);
            receive_response receive_data(std::vector<uint8_t> &buff, int &err, const int timeout_ms = 3000, const int chunk_size = 4096);
            receive_response receive_string(std::string &buff, int &err, const int timeout_ms = 3000, const int chunk_size = 4096);
            connect_response connect();
            bool close();
            virtual create_response create() = 0;
        protected:
            bool is_connected();
            bool dns_lookup(const std::string &host);
            virtual ssize_t receive_data_internal(void* data, size_t amt) = 0;
            virtual ssize_t send_data_internal(const void* data, size_t amt) = 0;
            virtual int get_error(int res) = 0;
    };

    class http_socket : public socket_base {
        public: 
            http_socket(const std::string &host, int port = 80);
            ~http_socket();

            create_response create() override;
        private:
            int get_error(int res) override;
            ssize_t receive_data_internal(void* data, size_t amt) override;
            ssize_t send_data_internal(const void* data, size_t amt) override;
    };

    class https_socket : public socket_base {
        private:
            static std::mutex ssl_ctx_mutex;
            static SSL_CTX* ssl_ctx;
            SSL* ssl;

        public:
            https_socket(const std::string &host, int port = 443);
            ~https_socket();
            
            connect_response connect();
            bool close();
            create_response create() override;
        private:
            int get_error(int res) override;
            ssize_t receive_data_internal(void* data, size_t amt) override;
            ssize_t send_data_internal(const void* data, size_t amt) override;
            void free_ssl_ctx();
    };
} // namespace network

#endif // NETWORK_CLIENT_SOCKET