#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "types.h"
#include "buffer_writer.h"
#include "utils.h"

typedef enum {
    A = 1  // ipv4?
} DNSType;

typedef enum {
    IN = 1  // internet
} DNSClass;

typedef struct {
    u16 transaction_id;
    u16 flags;
    u16 ques_count;
    u16 ans_count;
    u16 authority_count;
    u16 additional_count;
} DNSHeader;

typedef struct {
    const char *name;
    DNSType type;
    DNSClass  class;
} DNSQuestion;

typedef struct {
    byte *name;
    DNSType type;
    DNSClass class;
    u32 ttl;
    u16 data_len;
    byte *data;
} DNSRecord;

typedef struct {
    DNSHeader header;
    DNSQuestion *questions;
    DNSRecord *answers;
    DNSRecord *authorities;
    DNSRecord *additionals;
} DNSRequest;

void free_dns_request(DNSRequest *req) {
    if (req->questions) free(req->questions);
    if (req->answers) free(req->answers);
    if (req->authorities) free(req->authorities);
    if (req->additionals) free(req->additionals);

    *req = (DNSRequest){0};
}

bool serialize_dns_header(BufWriter *bw, const DNSHeader *header) {
    return bw_write_u16_be(bw, header->transaction_id)
        && bw_write_u16_be(bw, header->flags)
        && bw_write_u16_be(bw, header->ques_count)
        && bw_write_u16_be(bw, header->ans_count)
        && bw_write_u16_be(bw, header->authority_count)
        && bw_write_u16_be(bw, header->additional_count);
}

bool serialize_dns_question(BufWriter *bw, const DNSQuestion *ques) {
    char name_copy[strlen(ques->name)];
    strcpy(name_copy, ques->name);
    char *token = strtok(name_copy, ".");
    while (token != NULL) {
        size_t section_len = strlen(token);

        bw_write_u8(bw, section_len);
        bw_write(bw, token, section_len);

        token = strtok(NULL, ".");
    }
    bw_write_u8(bw, 0);

    bw_write_u16_be(bw, ques->type);
    bw_write_u16_be(bw, ques->class);

    return true;
}

bool serialize_dns_record(BufWriter *bw, const DNSRecord *record) {
    return false;
}

bool serialize_dns_request(BufWriter *bw, const DNSRequest *req) {
    if (!serialize_dns_header(bw, &req->header)) return false;
    for (u16 i = 0, n = req->header.ques_count; i < n; i++) {
        if (!serialize_dns_question(bw, req->questions + i)) return false;
    }
    for (u16 i = 0, n = req->header.ans_count; i < n; i++) {
        if (!serialize_dns_record(bw, req->answers + i)) return false;
    }
    for (u16 i = 0, n = req->header.authority_count; i < n; i++) {
        if (!serialize_dns_record(bw, req->authorities + i)) return false;
    }
    for (u16 i = 0, n = req->header.additional_count; i < n; i++) {
        if (!serialize_dns_record(bw, req->additionals + i)) return false;
    }
    return true;
}

int main(void) {
    srand((unsigned)time(NULL));

    // ---
    DNSRequest req = {0};
    req.header = (DNSHeader){
        .transaction_id = rand_u16(),
        .flags = 0,
        .ques_count = 1,
        .ans_count = 0,
        .authority_count = 0,
        .additional_count = 0
    };
    req.questions = malloc(sizeof(DNSQuestion));
    req.questions[0] = (DNSQuestion){
        .name = "google.com",
        .type = A,
        .class = IN
    };

    // ---
    BufWriter bw = bw_init();

    int ret = serialize_dns_request(&bw, &req);
    
    free_dns_request(&req);
    
    if (!ret) {
        printf("Error: Failed to serialize DNS request.");
        return 2;
    }
    print_bytes(bw.data, bw.size);

    // ---
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        print_err("Failed to open socket.");
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(53);
    inet_pton(AF_INET, "8.8.8.8", &addr.sin_addr);

    long sent_size = sendto(
        fd,
        bw.data, bw.size,
        0,
        (struct sockaddr*)&addr,
        sizeof(addr)
    );
    if (sent_size == -1) {
        print_err("Failed to send request.");
        return 3;
    }

    struct timeval timeout = {
        .tv_sec = 2,
        .tv_usec = 0
    };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    struct sockaddr_in _addr;
    socklen_t _addr_size = sizeof(_addr);
    byte buf[256];
    long recv_size = recvfrom(
        fd,
        buf, 256,
        0,
        (struct sockaddr*)&_addr,
        &_addr_size
    );
    if (recv_size == -1) {
        print_err("Failed to receive.");
        return 3;
    }

    printf("Response:\n");
    print_bytes(buf, recv_size);
 
    // ---
    close(fd);

    bw_free(&bw);

    printf("DONE!\n");
    return 0;
}