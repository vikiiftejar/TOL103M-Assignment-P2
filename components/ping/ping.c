#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>     // strtol
#include <inttypes.h>   // PRIu64

#include <esp_log.h>

#include "ping.h"
#include "serial_io.h"
#include "utility.h"
#include "lownet.h"

#define TAG "PING"

void ping_init() {
    if (lownet_register_protocol(LOWNET_PROTOCOL_PING, ping_receive) != 0) {
        ESP_LOGE(TAG, "Error registering PING protocol");
    }
}

// Handle /ping ID command
void ping_command(char* args) {
    if (args == NULL) {
        serial_write_line("Usage: /ping ID");
        return;
    }

    // Parse ID (supports decimal or hex like 0xF0)
    uint8_t id = (uint8_t)strtol(args, NULL, 0);
    ping(id);
}

// Send a ping packet
void ping(uint8_t node) {
    uint8_t payload[11];

    // Get current network time
    lownet_time_t now = lownet_get_time();

    // timestamp_out
    memcpy(payload, &now, sizeof(lownet_time_t));

    // timestamp_back = zero
    lownet_time_t zero = {0, 0};
    memcpy(payload + 5, &zero, sizeof(lownet_time_t));

    // origin = my node ID
    payload[10] = lownet_get_device_id();

    // Build frame
    lownet_frame_t frame;
    frame.protocol = LOWNET_PROTOCOL_PING;
    frame.destination = node;
    frame.length = sizeof(payload);
    memcpy(frame.payload, payload, sizeof(payload));

    lownet_send(&frame);

    char buf[64];
    snprintf(buf, sizeof(buf), "Ping sent to 0x%02X", node);
    serial_write_line(buf);
}

// Receive ping or pong
void ping_receive(const lownet_frame_t* frame) {
    if (frame->length < 11) {
        return; // invalid
    }

    uint8_t origin = frame->payload[10];
    lownet_time_t timestamp_out;
    lownet_time_t timestamp_back;

    memcpy(&timestamp_out, frame->payload, 5);
    memcpy(&timestamp_back, frame->payload + 5, 5);

    if (origin == lownet_get_device_id()) {
        // This is our pong reply → calculate RTT
        lownet_time_t now = lownet_get_time();

        // convert to 64-bit ms
        uint64_t t_out = (uint64_t)timestamp_out.seconds * 1000 +
                         ((uint64_t)timestamp_out.parts * 1000 / 256);

        uint64_t t_now = (uint64_t)now.seconds * 1000 +
                         ((uint64_t)now.parts * 1000 / 256);

        uint64_t rtt = t_now - t_out;

        char buf[64];
        snprintf(buf, sizeof(buf), "Ping reply from 0x%02X: %" PRIu64 " ms",
                 frame->source, rtt);
        serial_write_line(buf);

    } else {
        // This is someone else’s ping → reply with pong
        uint8_t reply_payload[11];
        memcpy(reply_payload, frame->payload, frame->length);

        // Fill timestamp_back with my current time
        lownet_time_t now = lownet_get_time();
        memcpy(reply_payload + 5, &now, 5);

        lownet_frame_t reply;
        reply.protocol = LOWNET_PROTOCOL_PING;
        reply.destination = origin;   // always back to origin
        reply.length = frame->length;
        memcpy(reply.payload, reply_payload, reply.length);

        lownet_send(&reply);

        char buf[64];
        snprintf(buf, sizeof(buf), "Pong sent to 0x%02X", origin);
        serial_write_line(buf);
    }
}

