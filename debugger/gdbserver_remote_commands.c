#include "gdbserver_remote_commands.h"
#include "gdbserver.h"

#include <stddef.h>

static uint8_t remote_command_help(void)
{
    const struct remote_command_entry_t *entry;

    gdbserver_send_remote_console_output("Supported commands:\n");

    for (entry = remote_commands; entry->name; entry++) {
        gdbserver_send_remote_console_output(entry->name);
        gdbserver_send_remote_console_output("\n");
    }

    return 0;
}

static uint8_t remote_command_reset(void)
{
    return gdbserver_reset_via_remote_command();
}

const struct remote_command_entry_t remote_commands[] = {
    { "help", remote_command_help },
    { "reset", remote_command_reset },
    { NULL, NULL }
};
