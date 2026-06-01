#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "endianess.h"
#include "list.h"
#include "types.h"
#include "buffer_writer.h"
#include "buffer_reader.h"
#include "my_string.h"
#include "hash_table.h"
#include "log.h"
#include "utils.h"

// ---
DECLARE_LIST(u32, ListU32);

// types, enums and structures
#define DNS_TYPE_A     1
#define DNS_TYPE_NS    2
#define DNS_TYPE_CNAME 5
#define DNS_TYPE_MX    15
#define DNS_TYPE_TXT   16
#define DNS_TYPE_RP    17
#define DNS_TYPE_AAAA  28
#define DNS_TYPE_LOC   29

#define DNS_CLASS_IN 1

typedef struct {
    u16 transaction_id;
    u16 flags;
    u16 ques_count;
    u16 ans_count;
    u16 authority_count;
    u16 additional_count;
} DNSHeader;

typedef struct {
    String name;
    u16 type;
    u16  class;
} DNSQuestion;

typedef struct {
    String name;
    u16 type;
    u16 class;
    u32 ttl;
    u16 data_len;
    size_t _data_off;
} DNSRecord;

typedef struct {
    DNSHeader header;
    DNSQuestion *questions;
    DNSRecord *answers;
    DNSRecord *authorities;
    DNSRecord *additionals;
} DNSRequest;

typedef DNSRequest DNSResponse;

// basic functions
void free_dns_request(DNSRequest *req) {
    for (u16 i = 0, n = req->header.ques_count; i < n; i++)
        str_free(&req->questions[i].name);
    for (u16 i = 0, n = req->header.ans_count; i < n; i++)
        str_free(&req->answers[i].name);
    for (u16 i = 0, n = req->header.authority_count; i < n; i++)
        str_free(&req->authorities[i].name);
    for (u16 i = 0, n = req->header.additional_count; i < n; i++)
        str_free(&req->additionals[i].name);
    
    if (req->questions) free(req->questions);
    if (req->answers) free(req->answers);
    if (req->authorities) free(req->authorities);
    if (req->additionals) free(req->additionals);

    *req = (DNSRequest){0};
}

#define free_dns_response free_dns_request

// serializatino
bool serialize_dns_header(BufWriter *bw, const DNSHeader *header) {
    return bw_write_u16_be(bw, header->transaction_id)
        && bw_write_u16_be(bw, header->flags)
        && bw_write_u16_be(bw, header->ques_count)
        && bw_write_u16_be(bw, header->ans_count)
        && bw_write_u16_be(bw, header->authority_count)
        && bw_write_u16_be(bw, header->additional_count);
}

bool serialize_dns_question(BufWriter *bw, const DNSQuestion *ques) {
    char name_copy[ques->name.len + 1];
    strcpy(name_copy, ques->name.data);
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
    UNUSED(bw);
    UNUSED(record);
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

// deserialization
bool deserialize_dns_name(BufReader *br, String *str, bool top_lvl) {
    BufWriter name_writer = bw_init();

    bool jumped = false;
    size_t old_pos = 0;
    u8 section_size;
    br_read_u8(br, &section_size);
    while (section_size != 0) {
        if ((section_size & 0xC0) == 0xC0) {
            u8 _tmp;
            br_read_u8(br, &_tmp);
            u16 off = ((section_size & 0b00111111) << 8) | _tmp;

            jumped = true;
            old_pos = br->pos;
            br->pos = off;

            String section;
            deserialize_dns_name(br, &section, false);

            bw_write(&name_writer, section.data, section.len);
            str_free(&section);

            break;
        } else {
            char section[section_size];
            br_read_bytes(br, (byte*)section, section_size);

            bw_write(&name_writer, section, section_size);
            bw_write_u8(&name_writer, '.');
        }
        br_read_u8(br, &section_size);
    }
    if (jumped) br->pos = old_pos;
    
    if (top_lvl) name_writer.size--;  // removing last '.'

    *str = str_from_bw(&name_writer);

    return true;
}

bool deserialize_dns_header(BufReader *br, DNSHeader *header) {
    return br_read_u16_be(br, &header->transaction_id)
        && br_read_u16_be(br, &header->flags)
        && br_read_u16_be(br, &header->ques_count)
        && br_read_u16_be(br, &header->ans_count)
        && br_read_u16_be(br, &header->authority_count)
        && br_read_u16_be(br, &header->additional_count);
}

bool deserialize_dns_question(BufReader *br, DNSQuestion *ques) {
    return deserialize_dns_name(br, &ques->name, true)
        && br_read_u16_be(br, &ques->type)
        && br_read_u16_be(br, &ques->class);
}

bool deserialize_dns_record(BufReader *br, DNSRecord *record) {
    bool ret = deserialize_dns_name(br, &record->name, true)
            && br_read_u16_be(br, &record->type)
            && br_read_u16_be(br, &record->class)
            && br_read_u32_be(br, &record->ttl)
            && br_read_u16_be(br, &record->data_len);
    if (!ret) return false;
    
    record->_data_off = br->pos;

    br->pos += record->data_len;
    
    return true;
}

bool deserialize_dns_resp(BufReader *br, DNSResponse *resp) {
    if (!deserialize_dns_header(br, &resp->header)) return false;

    resp->questions   = malloc(sizeof(DNSQuestion) * resp->header.ques_count);
    resp->answers     = malloc(sizeof(DNSRecord)   * resp->header.ans_count);
    resp->authorities = malloc(sizeof(DNSRecord)   * resp->header.authority_count);
    resp->additionals = malloc(sizeof(DNSRecord)   * resp->header.additional_count);

    for (size_t i = 0, n = resp->header.ques_count; i < n; i++) {
        if (!deserialize_dns_question(br, resp->questions + i)) return false;
    }
    for (size_t i = 0, n = resp->header.ans_count; i < n; i++) {
        if (!deserialize_dns_record(br, resp->answers + i)) return false;
    }
    for (size_t i = 0, n = resp->header.authority_count; i < n; i++) {
        if (!deserialize_dns_record(br, resp->authorities + i)) return false;
    }
    for (size_t i = 0, n = resp->header.additional_count; i < n; i++) {
        if (!deserialize_dns_record(br, resp->additionals + i)) return false;
    }

    return true;
}

// resolver
static const byte ROOT_DNS_SERVERS[][4] = {
    { 198, 41 , 0  , 4   },
    { 170, 247, 170, 2   },
    { 192, 33 , 4  , 12  },
    { 199, 7  , 91 , 13  },
    { 192, 203, 230, 10  },
    { 192, 5  , 5  , 241 },
    { 192, 112, 36 , 4   },
    { 198, 97 , 190, 53  },
    { 192, 36 , 148, 17  },
    { 192, 58 , 128, 30  },
    { 193, 0  , 14 , 129 },
    { 199, 7  , 83 , 42  },
    { 202, 12 , 27 , 33  }
};

typedef struct {
    bool success;
    ListU32 addresses;
    String err;
} DNSResult;

typedef struct {
    HT *cache;
    struct {
        int lvl;
    } dbg_info;
} DNSResolver;

DNSResolver dns_resolver_new() {
    return (DNSResolver){
        ht_create(20),
        {
            .lvl = 0
        }
    };
}

void dns_resolver_free(DNSResolver *resolver) {
    if (resolver) {
        ht_free(resolver->cache);
        resolver->cache = NULL;
    }
}

void dns_result_free(DNSResult *res) {
    if (res) {
        ListU32_free(&res->addresses);
        str_free(&res->err);

        res->success = false;
        ListU32_init(&res->addresses);
        res->err = (String){0};
    }
}

#define TIMEOUT 2

DNSResult dns_resolve_ipv4(DNSResolver *resolver, String domain_name) {
    bool found = false;

    ListU32 addresses = {0};
    String err = {0};

    ListU32 server_addresses = {0};
    ListU32_push(&server_addresses, (
        ((u32)ROOT_DNS_SERVERS[0][0] << 24) |
        ((u32)ROOT_DNS_SERVERS[0][1] << 16) |
        ((u32)ROOT_DNS_SERVERS[0][2] << 8)  |
        ((u32)ROOT_DNS_SERVERS[0][3])
    ));

    int sock_fd = -1;

    DNSResponse resp = {0};

    // create request
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
        .name = str_new(domain_name.data),
        .type = DNS_TYPE_A,
        .class = DNS_CLASS_IN
    };

    BufWriter bw = bw_init();
    bool ret = serialize_dns_request(&bw, &req);

    if (!ret) {
        err = str_new("Error: Failed to serialize request.");
        goto exit;
    }

    // setup socket
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd == -1) {
        err = fmt_linux_err("Failed to create socket.");
        goto exit;
    }
    
    struct timeval timeout = {
        .tv_sec = TIMEOUT,
        .tv_usec = 0
    };
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(53);

redo:
    // send request
    req.header.transaction_id = rand_u16();

    if (server_addresses.size == 0) {
        err = str_new("Error: Ran out of DNS servers.");
        goto exit;
    }
    addr.sin_addr.s_addr = swap_32(ListU32_pop(&server_addresses));
    // LOG_D("Resolving %s from %s.", domain_name.data, inet_ntoa(addr.sin_addr));
    
    LOG_D(
        "Query=%s Server=%s TXID=%u",
        req.questions[0].name.data,
        inet_ntoa(addr.sin_addr),
        req.header.transaction_id
    );

    long sent_size = sendto(
        sock_fd,
        bw.data, bw.size,
        0,
        (struct sockaddr*)&addr,
        sizeof(struct sockaddr)
    );
    if (sent_size == -1) {
        err = fmt_linux_err("Failed to send request.");
        goto exit;
    }
    
    // receive response
    struct sockaddr_in _addr;
    socklen_t _addr_size = sizeof(struct sockaddr_in);
    byte buf[512];
    long recv_size = recvfrom(
        sock_fd,
        buf, 512,
        0,
        (struct sockaddr*)&_addr,
        &_addr_size
    );
    if (recv_size == -1) {
        err = fmt_linux_err("Failed to receive response.");
        goto exit;
    }
    
    BufReader br = br_init(buf, recv_size);
    ret = deserialize_dns_resp(&br, &resp);
    if (!ret) {
        err =  str_new("Error: Failed to deserialize response.");
        goto exit;
    }

    LOG_D(
        "Response from=%s TXID=%u "
        "ANS=%u AUTH=%u ADD=%u",
        inet_ntoa(_addr.sin_addr),
        resp.header.transaction_id,
        resp.header.ans_count,
        resp.header.authority_count,
        resp.header.additional_count
    );
    
    // cache addresses from additionals section 
    for (size_t i = 0, n = resp.header.additional_count; i < n; i++) {
        const DNSRecord *rr = resp.additionals + i;
        switch (rr->type) {
            case DNS_TYPE_A: {
                const u32 _ipv4_addr_u32 = (
                    ((u32)br.data[rr->_data_off    ] << 24) |
                    ((u32)br.data[rr->_data_off + 1] << 16) |
                    ((u32)br.data[rr->_data_off + 2] << 8 ) |
                    ((u32)br.data[rr->_data_off + 3]      )
                );
                    
                ht_put(
                    resolver->cache,
                    rr->name.data,
                    &_ipv4_addr_u32,
                    sizeof(u32)
                );

                break;
            }
            default: {
                LOG_V("Ignoring additional record of type %u for %s.", rr->type, rr->name.data);
                break;
            }
        }
    }

    // check answers
    for (size_t i = 0, n = resp.header.ans_count; i < n; i++) {
        const DNSRecord *ans = resp.answers + i;
        switch (ans->type) {
            case DNS_TYPE_A: {
                const u32 _ipv4_addr_u32 = (
                    ((u32)br.data[ans->_data_off    ] << 24) |
                    ((u32)br.data[ans->_data_off + 1] << 16) |
                    ((u32)br.data[ans->_data_off + 2] << 8 ) |
                    ((u32)br.data[ans->_data_off + 3]      )
                );

                ListU32_push(&addresses, _ipv4_addr_u32);

                found = true;

                break;
            }

            case DNS_TYPE_CNAME: {
                String canonical_name;
                br.pos = ans->_data_off;
                deserialize_dns_name(&br, &canonical_name, true);

                ListU32_clear(&addresses);
                found = false;

                str_free(&req.questions[0].name);
                req.questions[0].name = canonical_name;

                bw_free(&bw);
                serialize_dns_request(&bw, &req);

                goto redo;
            }

            default: {
                LOG_V("Unwanted answer of type %u.", ans->type);
                break;
            }
        }
    }

    if (!found) {
        bool cached_server_found = false;

        // check for provided name-servers or failures if not found
        for (size_t i = 0, n = resp.header.authority_count; i < n; i++) {
            const DNSRecord *rr = resp.authorities + i;

            switch (rr->type) {
                case DNS_TYPE_NS: {
                    String nameserver_domain_name;
                    br.pos = rr->_data_off;
                    deserialize_dns_name(&br, &nameserver_domain_name, true);

                    const u32* cached_addr = ht_get(resolver->cache, nameserver_domain_name.data);
                    if (cached_addr) {
                        ListU32_push(&server_addresses, *cached_addr);
                        cached_server_found = true;
                    } else if (!cached_server_found) {  // only do if no already cached server is found
                        DNSResult res = dns_resolve_ipv4(resolver, nameserver_domain_name);

                        if (res.success) {
                            ht_put(
                                resolver->cache,
                                nameserver_domain_name.data,
                                (void*)ListU32_ptr_at(&res.addresses, 0),
                                4
                            );

                            ListU32_push(&server_addresses, ListU32_at(&res.addresses, 0));  // TODO: try all IPv4 addresses for each server too?

                            dns_result_free(&res);
                        }
                    }

                    str_free(&nameserver_domain_name);

                    break;
                }
            }
        } 

        free_dns_response(&resp);
        goto redo;
    }

exit:
    if (sock_fd != -1) close(sock_fd);
    
    free_dns_request(&req);
    free_dns_response(&resp);
    bw_free(&bw);
    ListU32_free(&server_addresses);
    
    DNSResult res = {
        found,
        addresses,
        err
    };

    return res;
}

// main
int main(int argc, char **argv) {
    String domain_name;
    if (argc != 2) domain_name = str_new("google.com");
    else domain_name = str_new(argv[1]);

    DNSResolver resolver = dns_resolver_new();

    DNSResult res = dns_resolve_ipv4(&resolver, domain_name);

    if (!res.success) {
        LOG_E("Couldn't resolve %s.", domain_name.data);
        LOG_E("%s", res.err.data);
    } else {
        for (size_t i = 0; i < res.addresses.size; i++) {
            LOG_S("%u.%u.%u.%u",
                (res.addresses.data[i] & 0xff000000) >> 24,
                (res.addresses.data[i] & 0x00ff0000) >> 16,
                (res.addresses.data[i] & 0x0000ff00) >> 8,
                (res.addresses.data[i] & 0x000000ff)
            );
        }
    }

    dns_resolver_free(&resolver);
    dns_result_free(&res);

    str_free(&domain_name);

    return 0;
}