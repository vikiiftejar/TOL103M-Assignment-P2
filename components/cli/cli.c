#include "cli.h"

#include <string.h>
#include <stdio.h>
#include <inttypes.h>   // for PRIu32

#include <utility.h>
#include <serial_io.h>
#include <lownet.h>     // for lownet_get_device_id(), lownet_get_time()
#include <chat.h>       // for shout_command, tell_command
#include <ping.h>       // for ping_command

command_fun_t find_command(const char* command, const command_t* commands, size_t n)
{
    /*
        Loop Invariant:
        0 <= i < n
        forall x | 0 <= x < i : commands[i].name != command
    */
    for (size_t i = 0; i < n; ++i)
        if (!strcmp(command, commands[i].name))
            return commands[i].fun;

    return NULL;
}

// Print device ID
void id_command(char* args)
{
    // id + null terminator
    char buffer[ID_WIDTH + 1];
    format_id(buffer, lownet_get_device_id());
    serial_write_line(buffer);
}

// Print network time
void date_command(char* args) {
    lownet_time_t now = lownet_get_time();

    if (now.seconds == 0 && now.parts == 0) {
        serial_write_line("Network time is not available.");
        return;
    }

    // convert parts (1/256 s) to tenths
    uint32_t tenths = (now.parts * 10) / 256;

    char buffer[64];
    snprintf(buffer, sizeof(buffer),
             "%" PRIu32 ".%01" PRIu32 " sec since the course started.",
             now.seconds, tenths);

    serial_write_line(buffer);
}
