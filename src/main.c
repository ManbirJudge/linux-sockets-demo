#include <math.h>
#include <stdalign.h>
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
#include "buffer_reader.h"
#include "my_string.h"
#include "utils.h"

// types, enums and structures
#define DNS_TYPE_A    1
#define DNS_TYPE_NS   2
#define DNS_TYPE_MX   15
#define DNS_TYPE_TXT  16
#define DNS_TYPE_RP   17
#define DNS_TYPE_AAAA 28
#define DNS_TYPE_LOC  29

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
    for (u16 i = 0, n = req->header.ans_count; i < n; i++) {
        str_free(&req->answers[i].name);
        // if (req->answers[i].data) free(req->answers[i].data);
    }
    for (u16 i = 0, n = req->header.authority_count; i < n; i++) {
        str_free(&req->authorities[i].name);
        // if (req->authorities[i].data) free(req->authorities[i].data);
    }
    for (u16 i = 0, n = req->header.additional_count; i < n; i++) {
        str_free(&req->additionals[i].name);
        // if (req->additionals[i].data) free(req->additionals[i].data);
    }
    
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
bool deserialize_dns_name(BufReader *br, String *str) {
    BufWriter name_writer = bw_init();

    size_t old_pos = 0;
    u8 section_size;
    br_read_u8(br, &section_size);
    while (section_size != 0) {
        if ((section_size & 0xC0) == 0xC0) {
            u8 _tmp;
            br_read_u8(br, &_tmp);
            u16 off = ((section_size & 0b00111111) << 8) | _tmp;

            old_pos = br->pos;
            br->pos = off;

            String section;
            deserialize_dns_name(br, &section);

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
    if (old_pos != 0) br->pos = old_pos;

    name_writer.size--;

    // str_free(str);
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
    return deserialize_dns_name(br, &ques->name)
        && br_read_u16_be(br, &ques->type)
        && br_read_u16_be(br, &ques->class);
}

bool deserialize_dns_record(BufReader *br, DNSRecord *record) {
    bool ret = deserialize_dns_name(br, &record->name)
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

// main
int main(void) {
    // init
    srand((unsigned)time(NULL));
    
    // user input
    char domain_name[256] = {0};

    printf("Enter a domain name: ");
    scanf("%255s", domain_name);
    if (strlen(domain_name) == 0) strcpy(domain_name, "google.com");

    // initialize request
    DNSRequest req = {0};
    req.header = (DNSHeader){
        .transaction_id = rand_u16(),
        .flags = 0x0100,
        .ques_count = 1,
        .ans_count = 0,
        .authority_count = 0,
        .additional_count = 0
    };
    req.questions = malloc(sizeof(DNSQuestion));
    req.questions[0] = (DNSQuestion){
        .name = str_new(domain_name),
        .type = DNS_TYPE_LOC,
        .class = DNS_CLASS_IN
    };

    BufWriter bw = bw_init();
    int ret = serialize_dns_request(&bw, &req);

    free_dns_request(&req);
    
    if (!ret) {
        printf("Error: Failed to serialize DNS request.");
        return 2;
    }

    printf("\nRequest:\n");
    print_bytes(bw.data, bw.size);

    // initalize socket
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        print_err("Failed to open socket.");
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(53);
    inet_pton(AF_INET, "192.168.1.1", &addr.sin_addr);

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

    // send request
    struct sockaddr_in _addr;
    socklen_t _addr_size = sizeof(_addr);
    byte buf[512];
    long recv_size = recvfrom(
        fd,
        buf, 512,
        0,
        (struct sockaddr*)&_addr,
        &_addr_size
    );
    if (recv_size == -1) {
        print_err("Failed to receive.");
        return 3;
    }
    
    close(fd);
    bw_free(&bw);

    // read reponse
    BufReader br = br_init(buf, recv_size);

    printf("\nRaw response:\n");
    print_bytes(br.data, br.size);

    DNSResponse resp = {0};
    ret = deserialize_dns_resp(&br, &resp);
    if (!ret) {
        printf("Error: Failed to deserialize response.\n");
        return 4;
    }

    printf("\nCount:\n  Questions: %u\n  Answers: %u\n  Authorities: %u\n  Additionals: %u\n",
        resp.header.ques_count,
        resp.header.ans_count,
        resp.header.authority_count,
        resp.header.additional_count
    );

    printf("\n");
    for (size_t i = 0, n = resp.header.ans_count; i < n; i++) {  // TODO: maybe use a local BufReader to automatically enforce record boundries
        const DNSRecord *ans = &resp.answers[i];
        switch (ans->type) {
            case DNS_TYPE_A: {
                if (ans->data_len != 4) {
                    printf("Invalid IPv4 record data lenght.\n");
                    break;
                }
                printf("IPv4: %u.%u.%u.%u\n",
                    br.data[ans->_data_off],
                    br.data[ans->_data_off + 1],
                    br.data[ans->_data_off + 2],
                    br.data[ans->_data_off + 3]
                );
                break;
            }
            case DNS_TYPE_AAAA: {
                if (ans->data_len != 16) {
                    printf("Invalid IPv6 record data lenght.\n");
                    break;
                }

                struct in6_addr ipv6_addr;
                memcpy(&ipv6_addr, br.data + ans->_data_off, 16);

                char ipv6_addr_str[INET6_ADDRSTRLEN];
                if (inet_ntop(AF_INET6, &addr, ipv6_addr_str, sizeof(ipv6_addr_str)) == NULL) {
                    perror("abcd");
                    break;
                }

                printf("IPv6: %s\n", ipv6_addr_str);

                break;
            }
            // case DNS_TYPE_NS: {
            //     String ns;

            //     BufReader br = br_init(ans->data, ans->data_len);
            //     if (!deserialize_dns_name(&br, &ns)) {
            //         printf("Failed to deserialize NS.\n");
            //         break;
            //     }

            //     printf("NS: %s\n", ns.data);

            //     break;
            // }
            case DNS_TYPE_MX: {
                struct {
                    u16 priority;
                    String domain_name;
                } mail_data;

                br.pos = ans->_data_off;

                br_read_u16_be(&br, &mail_data.priority);
                deserialize_dns_name(&br, &mail_data.domain_name);

                printf("MX:\n  Priority: %u\n  Domain: %s\n", mail_data.priority, mail_data.domain_name.data);

                str_free(&mail_data.domain_name);

                break;
            }
            case DNS_TYPE_RP: {
                struct {
                    String mail_box_domain;
                    String txt_domain;
                } responsible_person;

                br.pos = ans->_data_off;

                deserialize_dns_name(&br, &responsible_person.mail_box_domain);
                deserialize_dns_name(&br, &responsible_person.txt_domain);

                printf("Responsible person:\n  Mailbox domain: %s\n  Text domain: %s\n", responsible_person.mail_box_domain.data, responsible_person.txt_domain.data);

                str_free(&responsible_person.mail_box_domain);
                str_free(&responsible_person.txt_domain);

                break;
            }
            case DNS_TYPE_TXT: {
                br.pos = ans->_data_off;
                size_t ans_end = ans->_data_off + ans->data_len;

                printf("TXT:\n");
                while (br.pos < ans_end) {
                    u8 txt_len;
                    br_read_u8(&br, &txt_len);

                    if (br.pos + txt_len > ans_end) break;

                    unsigned char txt[txt_len + 1];
                    br_read_bytes(&br, txt, txt_len);
                    txt[txt_len] = '\0';

                    printf("  - %s\n", txt);
                }

                break;
            }
            case DNS_TYPE_LOC: {
                struct {
                    u8 ver;
                    u8 size;
                    u8 horiz_precision;
                    u8 vert_precision;

                    u32 latitude;
                    u32 longitude;
                    u32 altitue;
                } location_data_raw;
                
                br.pos = ans->_data_off;

                br_read_u8(&br, &location_data_raw.ver);
                if (location_data_raw.ver != 0) break;

                br_read_u8(&br, &location_data_raw.size);
                br_read_u8(&br, &location_data_raw.horiz_precision);
                br_read_u8(&br, &location_data_raw.vert_precision);
                br_read_u32_be(&br, &location_data_raw.latitude);
                br_read_u32_be(&br, &location_data_raw.longitude);
                br_read_u32_be(&br, &location_data_raw.altitue);

                int size_base = location_data_raw.size >> 4;
                int size_exponent = location_data_raw.size & 0x0f;
                float size = (float)size_base * powf(10.f, (float)size_exponent - 3.f);
                
                int horiz_pre_base = location_data_raw.horiz_precision >> 4;
                int horiz_pre_exponent = location_data_raw.horiz_precision & 0x0f;
                float horiz_precision = (float)horiz_pre_base * powf(10.f, (float)horiz_pre_exponent - 3.f);  // m
                
                int vert_pre_base = location_data_raw.vert_precision >> 4;
                int vert_pre_exponent = location_data_raw.vert_precision & 0x0f;
                float vert_precision = (float)vert_pre_base * powf(10.f, (float)vert_pre_exponent - 3.f);  // m

                float latitude  = (float)((i64)location_data_raw.latitude  - 2147483648LL) / 3600000.f;  // degrees
                float longitude = (float)((i64)location_data_raw.longitude - 2147483648LL) / 3600000.f; // degrees

                float altitude = (float)(location_data_raw.altitue - 10000000) / 100.;  // m

                char _lat_dir = (latitude  >= 0) ? 'N' : 'S';
                char _lon_dir = (longitude >= 0) ? 'E' : 'W';
                float _lat_abs = fabs(latitude);
                float _lon_abs = fabs(longitude);
                int   _lat_deg     = (int)_lat_abs;
                float _lat_rem_min = (_lat_abs - _lat_deg) * 60.f;
                int   _lat_min     = (int)_lat_rem_min;
                float _lat_sec     = (_lat_rem_min - _lat_min) * 60.f;
                int   _lon_deg     = (int)_lon_abs;
                float _lon_rem_min = (_lon_abs - _lon_deg) * 60.f;
                int   _lon_min     = (int)_lon_rem_min;
                float _lon_sec     = (_lon_rem_min - _lon_min) * 60.f;

                printf("Location data:\n  Coordaintes: %d°%d'%.2f\" %c, %d°%d'%.2f\" %c\n  Altitude: %.2f m\n  Size: %.2f m\n  Horizontal precisoin: %.2f m\n  Vertical precision: %.2f m\n",
                    _lat_deg, _lat_min, _lat_sec, _lat_dir, 
                    _lon_deg, _lon_min, _lon_sec, _lon_dir, 
                    altitude,
                    size, 
                    horiz_precision,
                    vert_precision
                );

                break;
            }
            default: {
                printf("Warning: Unkown record type: %u\n", ans->type);
                break;
            }
        }
    }

    free_dns_response(&resp);
    
    // ---
    printf("\nDONE!\n");
    return 0;
}
