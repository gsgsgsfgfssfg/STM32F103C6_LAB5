#include "cli.h"

static char cli_buffer[CLI_BUFFER_SIZE];
static uint8_t cli_length;
static uint8_t cli_line_ready;

#define BACKSPACE 0x08
#define ENTER 0x0D

typedef enum
{
    CLI_CMD_INVALID,
    CLI_CMD_CLEAR,
    CLI_CMD_GOTO,
    CLI_CMD_PRINT,
    CLI_CMD_HELP
} CLI_CommandType;

typedef struct
{
    CLI_CommandType type;

    uint8_t row;
    uint8_t col;

    char text[CLI_BUFFER_SIZE];
} CLI_Command;

static const char* skipSpaces(const char *p)
{
    while (*p == ' '){
        p++;
    }

    return p;
}

static uint8_t parseNumber(const char **p, uint8_t *value)
{
    uint16_t result = 0;
    uint8_t digit_found = 0;

    *p = skipSpaces(*p);

    while (**p >= '0' && **p <= '9'){
        result = result * 10 + (**p - '0');
        digit_found = 1;

        if (result > 255) return 0;

        (*p)++;
    }

    if (!digit_found) return 0;

    *value = (uint8_t)result;
    return 1;
}
uint8_t CLI_Parse(const char *input, CLI_Command *cmd)
{
    const char *p = input;

    // Reset parsed command
    cmd->type = CLI_CMD_INVALID;
    cmd->row = 0;
    cmd->col = 0;
    cmd->text[0] = '\0';

    p = skipSpaces(p);

    // Empty command
    if (*p == '\0') return 0;

    // Command: clear
    if (strncmp(p, "clear", 5) == 0 && (p[5] == '\0' || p[5] == ' ')) {
        p += 5;
        p = skipSpaces(p);

        if (*p != '\0') return 0;

        cmd->type = CLI_CMD_CLEAR;
        return 1;
    }

    // Command: help
    if (strncmp(p, "help", 4) == 0 && (p[4] == '\0' || p[4] == ' ')) {
        p += 4;
        p = skipSpaces(p);

        if (*p != '\0') return 0;

        cmd->type = CLI_CMD_HELP;
        return 1;
    }

    // Command: goto ROW COL
    if (strncmp(p, "goto", 4) == 0 && p[4] == ' ') {
        p += 4;
        if (!parseNumber(&p, &cmd->col)) return 0;
        if (!parseNumber(&p, &cmd->row)) return 0;

        p = skipSpaces(p);

        // Reject extra arguments
        if (*p != '\0') return 0;

        // LCD is 16x2
        if (cmd->row > 1 || cmd->col > 15)
            return 0;

        cmd->type = CLI_CMD_GOTO;
        return 1;
    }

    // Command: print "TEXT"
    if (strncmp(p, "print", 5) == 0 && p[5] == ' ') {
        p += 5;
        p = skipSpaces(p);

        // Opening quotation mark
        if (*p != '"') return 0;
        p++;

        uint8_t i = 0;

        // Read text until closing quotation mark
        while (*p != '"' && *p != '\0') {
            if (i >= CLI_BUFFER_SIZE - 1) return 0;
            cmd->text[i++] = *p++;
        }

        // Closing quotation mark must exist
        if (*p != '"') return 0;

        cmd->text[i] = '\0';
        p++;
        p = skipSpaces(p);

        // Reject extra characters after quotation
        if (*p != '\0') return 0;

        cmd->type = CLI_CMD_PRINT;
        return 1;
    }

    // Unknown command
    return 0;
}
void CLI_Execute(CLI_Command *cmd)
{
    switch (cmd->type)
    {
        case CLI_CMD_CLEAR:
            lcd_clear();
            break;

        case CLI_CMD_GOTO:
            lcd_goto(cmd->col, cmd->row);
            break;

        case CLI_CMD_PRINT:
            lcd_print(cmd->text);
            break;

        case CLI_CMD_HELP:
            // TODO: Send help text to UART
            break;

        default:
            // Invalid command
            break;
    }
}

static uint8_t isPrintable(uint8_t val){
	return (val >= 32) && (val <= 126);
}

void CLI_Init(void){
	cli_buffer[0] = '\0';
	cli_length = 0;
	cli_line_ready = 0;
}
void CLI_ProcessByte(uint8_t byte)
{
    if (cli_line_ready) return;

    if (byte == BACKSPACE)
    {
        // TODO: Remove the last character
    	if (cli_length > 0) {
    		cli_length--;
    		cli_buffer[cli_length] = '\0';
    	}
        return;
    }

    if (byte == ENTER)
    {
        // TODO: Terminate the string
    	cli_buffer[cli_length] = '\0';
        // TODO: Mark the line as ready
    	cli_line_ready = 1;
        return;
    }

    // TODO: Check if the character is printable
    if (!isPrintable(byte)) return;
    // TODO: Check buffer capacity
    if (cli_length >= CLI_BUFFER_SIZE-1) return;
    // TODO: Append the character
    cli_buffer[cli_length] = (char)byte;
    cli_length++;
    cli_buffer[cli_length] = '\0';
}
void CLI_Process(void)
{
    if (!cli_line_ready) return;

    // cli_buffer now contains a complete command

    // TODO: Parse command
    CLI_Command cmd;
    // TODO: Execute command
    if (CLI_Parse(cli_buffer, &cmd)) CLI_Execute(&cmd);
    else{
        const char error[] = "Syntax error\r\n";

        //HAL_UART_Transmit(&huart2, (uint8_t *)error, sizeof(error) - 1, 100);
    }
    // TODO: Reset input buffer
    cli_buffer[0] = '\0';
    cli_length = 0;
    cli_line_ready = 0;
}
