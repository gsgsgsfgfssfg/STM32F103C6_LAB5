#include "cli.h"

extern UART_HandleTypeDef huart2;

static char cli_buffer[CLI_BUFFER_SIZE];
static volatile uint8_t cli_length;
static volatile uint8_t cli_line_ready;
static volatile CLI_State_t cli_state;

#define BACKSPACE 0x08
#define ENTER 0x0D

static void CLI_Transmit(const char *data, uint16_t length){
    uint32_t start_tick = HAL_GetTick();

    while (huart2.gState != HAL_UART_STATE_READY && (HAL_GetTick() - start_tick) < 100U) {
    }

    if (huart2.gState == HAL_UART_STATE_READY) {
        HAL_UART_Transmit(&huart2, (uint8_t *)data, length, 100);
    }
}

typedef struct{
    CLI_CommandType type;

    uint8_t row;
    uint8_t col;

    char text[CLI_BUFFER_SIZE];
} CLI_Command;

static const char* skipSpaces(const char *p){
    while (*p == ' '){
        p++;
    }

    return p;
}

static uint8_t parseNumber(const char **p, uint8_t *value){
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
uint8_t CLI_Parse(const char *input, CLI_Command *cmd){
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

    // Command: goto COL ROW
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
void CLI_Execute(CLI_Command *cmd){
	Protocol_Frame_t frame;
    switch (cmd->type){
        case CLI_CMD_CLEAR:
            //lcd_clear();
        	Protocol_InitFrame(&frame,CMD_CLEAR,0,0,NULL);
        	Communication_Submit(&frame);
            break;

        case CLI_CMD_GOTO:{
            //lcd_goto(cmd->col, cmd->row);
        	uint8_t data[2] = {cmd->col,cmd->row};
        	Protocol_InitFrame(&frame,CMD_GOTO,0,2,data);
        	Communication_Submit(&frame);
            break;
        }
        case CLI_CMD_PRINT:
            //lcd_print(cmd->text);
        	Protocol_InitFrame(&frame,CMD_PRINT,0,strlen(cmd->text),(uint8_t*)cmd->text);
        	Communication_Submit(&frame);
            break;

        case CLI_CMD_HELP:{
            // TODO: Send help text to UART
        	const char msg1[] = "- clear: clear the LCD screen\r\n";
        	const char msg2[] = "- goto [col] [row]: go to the position on LCD screen\r\n";
        	const char msg3[] = "- print [text]: print the text to the LCD screen \r\n";
            	CLI_Transmit(msg1, sizeof(msg1) - 1);
            	CLI_Transmit(msg2, sizeof(msg2) - 1);
            	CLI_Transmit(msg3, sizeof(msg3) - 1);
            break;
        }
        default:{
            // Invalid command
	        const char msg[] = "Not found command\r\n";
            CLI_Transmit(msg, sizeof(msg) - 1);
            break;
        }
    }
}

static uint8_t isPrintable(uint8_t val){
	return (val >= 32) && (val <= 126);
}

void CLI_Init(void){
	cli_buffer[0] = '\0';
	cli_length = 0;
	cli_line_ready = 0;
	cli_state = CLI_STATE_IDLE;
}
void CLI_ProcessByte(uint8_t byte){
	if (cli_state == CLI_STATE_BUSY) return;
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
void CLI_Process(void){
	if (cli_state == CLI_STATE_BUSY){
		Communication_Result_t comm_res = Communication_GetResult();
		if (comm_res == COMM_RESULT_PENDING) return;
		cli_state = CLI_STATE_IDLE;
		if (comm_res == COMM_RESULT_SUCCESS){
	        const char msg[] = "Done!\r\n";
            	CLI_Transmit(msg, sizeof(msg) - 1);
		} else {
	        const char msg[] = "Fail!\r\n";
            	CLI_Transmit(msg, sizeof(msg) - 1);
		}
		return;
	}

    if (!cli_line_ready) return;
    CLI_Command cmd;
    if (CLI_Parse(cli_buffer, &cmd) == 1) {
    	if (cmd.type != CLI_CMD_HELP) cli_state = CLI_STATE_BUSY;
    	CLI_Execute(&cmd);
    }
    else{
        const char error[] = "Syntax error\r\n";
        CLI_Transmit(error, sizeof(error) - 1);
    }

    cli_buffer[0] = '\0';
    cli_length = 0;
    cli_line_ready = 0;
}
