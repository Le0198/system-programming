/**
* Resplendent RPCs Lab
* CS 241 - Fall 2018
*/

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "common.h"
#include "dns_query.h"
#include "dns_query_svc_impl.h"

#define CACHE_FILE "cache_files/rpc_server_cache"

char *contact_nameserver(query *argp, char *host, int port) {
    // Your code here
    // Look in the header file for a few more comments
    int sockfd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if (sockfd < 0) {
        perror("socket");
    }
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    inet_pton(AF_INET, host, &addr.sin_addr);

    ssize_t send_ret = sendto(sockfd, argp->host, strlen(argp->host), 0, (struct sockaddr *)&addr, sizeof(addr));
    
    if (send_ret == -1) {
	return NULL;
    }

    char ipaddr[MAX_BYTES_IPV4];
    memset(ipaddr, 0, MAX_BYTES_IPV4);
    socklen_t addrlen;
    addrlen = sizeof(addr);
    
    ssize_t recv_ret = recvfrom(sockfd, ipaddr, MAX_BYTES_IPV4, 0, (struct sockaddr *)&addr, &addrlen);

    while (recv_ret == -1) {
        if(errno == EAGAIN) {
            recv_ret = recvfrom(sockfd, ipaddr, 16, 0, (struct sockaddr *)&addr, &addrlen);
	}
        else {break;}
      }

    if (recv_ret == -1) {
	return NULL;
    }

    size_t count = 0;
    for (int i = 0; i < MAX_BYTES_IPV4; i++) {
	if (ipaddr[i] == ':') {
	    ipaddr[i] = '\0';
	    count = i;
	}
    }


    if (!strcmp("-1.-1.-1.-1", ipaddr)) {
	return NULL;
    }

    char* ip_address = strdup(ipaddr);
    return ip_address;
}

void create_response(query *argp, char *ipv4_address, response *res) {
    // Your code here
    // As before there are comments in the header file
    //res = malloc(sizeof(response));
    res->address = malloc(sizeof(host_address));
    res->address->host = strdup(argp->host);
    if (ipv4_address) {
	res->success = 1;
    	res->address->host_ipv4_address = strdup(ipv4_address);
    } else {
	res->success = 0;

	res->address->host_ipv4_address = strdup("hahaha");
    }
}

// Stub code

response *dns_query_1_svc(query *argp, struct svc_req *rqstp) {
    printf("Resolving query...\n");
    // check its cache, 'rpc_server_cache'
    // if it's in cache, return with response object with the ip address
    char *ipv4_address = check_cache_for_address(argp->host, CACHE_FILE);
    if (ipv4_address == NULL) {
        // not in the cache. contact authoritative servers like a recursive dns
        // server
        printf("Domain not found in server's cache. Contacting authoritative "
               "servers...\n");
        char *host = getenv("NAMESERVER_HOST");
        int port = strtol(getenv("NAMESERVER_PORT"), NULL, 10);
        ipv4_address = contact_nameserver(argp, host, port);
    } else {
        // it is in the server's cache; no need to ask the authoritative
        // servers.
        printf("Domain found in server's cache!\n");
    }

    static response res;
    xdr_free(xdr_response, &res); // Frees old memory in the response struct
    create_response(argp, ipv4_address, &res);

    free(ipv4_address);

    return &res;
}
