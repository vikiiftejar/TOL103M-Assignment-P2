#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <esp_log.h>

#include <string.h>
#include <stdlib.h>    // for strtol()

#include <lownet.h>
#include <serial_io.h>
#include <utility.h>   // for util_printable()

#include "chat.h"

#define TAG "CHAT"

void chat_init()
{
    if (lownet_register_protocol(LOWNET_PROTOCOL_CHAT, chat_receive) != 0)
    {
        ESP_LOGE(TAG, "Error registering CHAT protocol");
    }
}

// Broadcast command: /shout MSG
void shout_command(char* args)
{
    if (args == NULL) {
        serial_write_line("Usage: /shout MSG");
        return;
    }
    chat_shout(args);
}

// Direct message command: /tell ID MSG
void tell_command(char* args)
{
    if (args == NULL) {
        serial_write_line("Usage: /tell ID MSG");
        return;
    }

    char* id_str = strtok(args, " ");
    char* msg = strtok(NULL, "\n");

    if (id_str == NULL || msg == NULL) {
        serial_write_line("Usage: /tell ID MSG");
        return;
    }

    uint8_t id = (uint8_t)strtol(id_str, NULL, 0);
    chat_tell(msg, id);
}

// Handle incoming chat frames
void chat_receive(const lownet_frame_t* frame)
{
    char buffer[256];

    if (frame->destination == lownet_get_device_id()) {
        // Direct message
        snprintf(buffer, sizeof(buffer), "[DM from %d]: %.*s",
                 frame->source, frame->length, (char*)frame->payload);
        serial_write_line(buffer);
    }
    else if (frame->destination == 0xFF) {
        // Broadcast shout
        snprintf(buffer, sizeof(buffer), "[Shout from %d]: %.*s",
                 frame->source, frame->length, (char*)frame->payload);
        serial_write_line(buffer);
    }
}

// Send a broadcast chat
void chat_shout(const char* message)
{
    char clean[MSG_BUFFER_LENGTH];
    int j = 0;

    for (int i = 0; message[i] != '\0' && j < MSG_BUFFER_LENGTH - 1; i++) {
        if (util_printable(message[i])) {
            clean[j++] = message[i];
        }
    }
    clean[j] = '\0';

    lownet_frame_t frame;
    frame.protocol = LOWNET_PROTOCOL_CHAT;
    frame.destination = 0xFF;   // broadcast
    frame.length = strlen(clean);
    memcpy(frame.payload, clean, frame.length);

    lownet_send(&frame);

    // Feedback for user
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "You shouted: %s", clean);
    serial_write_line(buffer);
}

// Send a direct chat message
void chat_tell(const char* message, uint8_t destination)
{
    char clean[MSG_BUFFER_LENGTH];
    int j = 0;

    for (int i = 0; message[i] != '\0' && j < MSG_BUFFER_LENGTH - 1; i++) {
        if (util_printable(message[i])) {
            clean[j++] = message[i];
        }
    }
    clean[j] = '\0';

    lownet_frame_t frame;
    frame.protocol = LOWNET_PROTOCOL_CHAT;
    frame.destination = destination;
    frame.length = strlen(clean);
    memcpy(frame.payload, clean, frame.length);

    lownet_send(&frame);

    // Feedback for user
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "You told 0x%02X: %s", destination, clean);
    serial_write_line(buffer);
}

