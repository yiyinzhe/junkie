// -*- c-basic-offset: 4; c-backslash-column: 79; indent-tabs-mode: nil -*-
// vim:sw=4 ts=4 sts=4 expandtab
#include <stdlib.h>
#include <assert.h>
#include <junkie/cpp.h>
#include "lib.h"
#include "proto/ip.c"

/*
 * Parse check
 */

static struct parse_test {
    size_t size;
    uint8_t const packet[100];
    struct ip_proto_info expected;
} parse_tests [] = {
    {
        .size = 16*3+1, .packet = {
            0x45U, 0x00U, 0x00U, 0x31U, 0x00U, 0x00U, 0x40U, 0x00U, 0x01U, 0x11U, 0x77U, 0x3eU, 0xc0U, 0xa8U, 0x0aU, 0x06U,
            0xefU, 0xffU, 0x47U, 0xd0U, 0xb0U, 0x72U, 0xb0U, 0x72U, 0x00U, 0x1dU, 0x11U, 0xa9U, 0x43U, 0x6cU, 0x69U, 0x71U,
            0x75U, 0x65U, 0x01U, 0x03U, 0xccU, 0xfeU, 0x12U, 0x56U, 0x01U, 0xccU, 0xfeU, 0x12U, 0x56U, 0x4bU, 0x69U, 0xe1U,
            0xc8U,
        }, .expected = {
            .info = { .head_len = 20, .payload = 29 }, .version = 4, .ttl = 1,
            .key = {
                .protocol = 17,
                .addr = {
                    { .family = AF_INET, .u = { .v4 = { 0x060aa8c0U } } },
                    { .family = AF_INET, .u = { .v4 = { 0xd047ffefU } } },
                },
            },
        },
    }, {
        .size = 16*3+4, .packet = {
            0x45U, 0x10U, 0x00U, 0x34U, 0x07U, 0x71U, 0x40U, 0x00U, 0x40U, 0x06U, 0x53U, 0xf5U, 0xc0U, 0xa8U, 0x0aU, 0x09U,
            0x52U, 0x43U, 0xc2U, 0x59U, 0xd1U, 0x96U, 0x08U, 0xaeU, 0x29U, 0x1cU, 0x9eU, 0x07U, 0x64U, 0xfbU, 0x34U, 0xe0U,
            0x80U, 0x10U, 0x01U, 0xf5U, 0xb2U, 0x55U, 0x00U, 0x00U, 0x01U, 0x01U, 0x08U, 0x0aU, 0x0dU, 0x55U, 0x2dU, 0x1dU,
            0x2eU, 0xe9U, 0x3eU, 0x85U
        }, .expected = {
            .info = { .head_len = 20, .payload = 32 }, .version = 4, .ttl = 64,
            .key = {
                .protocol = 6,
                .addr = {
                    { .family = AF_INET, .u = { .v4 = { 0x090aa8c0U } } },
                    { .family = AF_INET, .u = { .v4 = { 0x59c24352U } } },
                },
            },
        },
    },
};

static unsigned current_test;

static int ip_info_check(struct proto_layer *layer)
{
    // Check layer->info against parse_tests[current_test].expected
    struct ip_proto_info const *const info = DOWNCAST(layer->info, info, ip_proto_info);
    struct ip_proto_info const *const expected = &parse_tests[current_test].expected;
    assert(info->info.head_len == expected->info.head_len);
    assert(info->info.payload == expected->info.payload);
    assert(info->version == expected->version);
    assert(0 == ip_addr_cmp(info->key.addr+0, expected->key.addr+0));
    assert(0 == ip_addr_cmp(info->key.addr+1, expected->key.addr+1));
    assert(info->key.protocol == expected->key.protocol);
    assert(info->ttl == expected->ttl);

    return 0;
}

static void parse_check(size_t cap_len)
{
    struct timeval now;
    timeval_set_now(&now);
    struct parser *ip_parser = proto_ip->ops->parser_new(proto_ip, &now);
    assert(ip_parser);

    for (current_test = 0; current_test < NB_ELEMS(parse_tests); current_test++) {
        size_t const len = parse_tests[current_test].size;
        int ret = ip_parse(ip_parser, NULL, 0, parse_tests[current_test].packet, len < cap_len ? len : cap_len, len, &now, ip_info_check);
        assert(0 == ret);
    }

    parser_unref(ip_parser);
}

int main(void)
{
    log_init();
    ip_init();
    log_set_level(LOG_DEBUG, NULL);
    log_set_file("ip_check.log");

    parse_check(65535);
    for (size_t cap_len = 40; cap_len < 50; cap_len++) {
        parse_check(cap_len);
    }
    stress_check(proto_ip);

    ip_fini();
    log_fini();
    return EXIT_SUCCESS;
}
