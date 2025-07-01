#include "serial_commands.h"
#include "../../conf/global_config.h"
#include <HardwareSerial.h>
#include <WString.h>

#define MAX_COMDLINE_SIZE 80
#define MAX_WORDS 6
#define BACKSPACE_CHAR 0x08
#define ESCAPE_CHAR '\x1b'
#define CSI_CHAR '['
#define LEFT_ARROW_CHAR 'D'
#define RIGHT_ARROW_CHAR 'C'
#define DELETE_KEY_1_CHAR '3'
#define DELETE_KEY_2_CHAR '~'
#define PRINT_CHAR_START 32
#define PRINT_CHAR_END 126

namespace serial_console
{
    bool global_disable_serial_console = false;

    /**
     * @brief Redraws the characters from the current cursor position to the end of the buffer on the serial console.
     *
     * This function is used to update the terminal display when characters are inserted or deleted
     * in the middle of the input buffer. It:
     *   - Saves the current cursor position
     *   - Clears the line from the cursor to the end
     *   - Prints characters from 'cur' to 'len'
     *   - Restores the original cursor position
     *
     * @param cur Current cursor position within the buffer
     * @param len Current length of the buffer (number of characters entered)
     * @param buf Character buffer containing the input string
     */
    static inline void redraw(const int cur, const int len, char *buf) {
        if (!temporary_config.remote_echo)
            return;

        Serial.print("\x1b[s");
        Serial.print("\x1b[K");

        // Reprint characters from cur to end
        for (int i = cur; i < len; i++) {
            Serial.print((char)buf[i]);
        }

        Serial.print(" \x1b[u");
    }

    /*
     * read_string_until: Non-blocking replacement for Serial.readStringUntil()..
     * With delimeter '\n' acts as 'read line'.
     * If input (line) size exceeds provided buffer's size, that entire input (until next delimeter) is silently discarded.
     * Serial.available() can return true without \n being ever in the buffer, so we don't trust it here.
     */

    bool read_string_until(char delimiter, char *result, int max_len)
    {
        static int index = 0;
        static int cur = 0;
        int c;         // read character, -1 if none
        int cnt = 100; // limit on amount of iterations in one go; we're supposed to be non-blocking!

        while ((c = Serial.read()) != -1 && cnt > 0)
        {
            --cnt;

            // backspaceF
            if (c == BACKSPACE_CHAR && cur > 0) {
                    // shift characters left from cursor position
                    memmove(&result[cur - 1], &result[cur], index - cur);
                    index--;
                    cur--;
                    // move cursor left on terminal and redraw updated string
                    if(temporary_config.remote_echo) Serial.print("\x1b[D");
                    redraw(cur, index, result);
            // handle ANSI escape sequences (arrow keys, delete key)
            } else if (c == ESCAPE_CHAR) {

                if ((c = Serial.read()) == -1)
                    break;

                // Expect '[' character
                if (c != CSI_CHAR)
                    continue;

                if ((c = Serial.read()) == -1)
                    break;

                // Left arrow key
                if (c == LEFT_ARROW_CHAR && cur > 0) {
                    // move cursor left on terminal
                    if(temporary_config.remote_echo) Serial.print("\x1b[D");
                    cur--;
                // Right arrow key
                } else if(c == RIGHT_ARROW_CHAR && cur < index) {
                    // move cursor right on terminal
                    if(temporary_config.remote_echo) Serial.print("\x1b[C");
                    cur++;
                // Delete key
                } else if(c == DELETE_KEY_1_CHAR) {
                    if ((c = Serial.read()) == -1)
                        break;
                    if (c == DELETE_KEY_2_CHAR && cur < index) {
                        memmove(&result[cur], &result[cur + 1], index - cur - 1);
                        index--;
                        redraw(cur, index, result);
                    }
                }

            // Handle printable characters
            } else if (c >= PRINT_CHAR_START && c <= PRINT_CHAR_END) {
                // Append character at the end
                if (index < max_len - 1 && cur == index) {
                    if(temporary_config.remote_echo) Serial.print((char)c);
                    result[index++] = c;
                    cur++;
                // Insert character in the middl
                } else if (index < max_len - 1) {
                    memmove(&result[cur + 1], &result[cur], index - cur);
                    result[cur] = c;
                    index++;
                    if(temporary_config.remote_echo) Serial.print((char)c);
                    cur++;
                    redraw(cur, index, result);
                } else if (c == delimiter) { // got delimeter: flush buffer quietly, restart collection.
                    index = 0;
                    cur = 0;
                    return false;
                }
            // delimiter was found
            } else if (c == delimiter) {

                if(temporary_config.remote_echo) Serial.println();
                result[index] = '\0'; // Null-terminate the string
                index = 0;
                cur = 0;
                return true; // Success: Delimiter found
            }
        }

        return false; // still waiting for delimeter
    }

    // split input into words.
    // assumes input ends with '\n', also ends parsing if found MAX_WORDS already.
    int tokenize(String results[], char *input)
    {
        int index = 0;
        int word_count = 0;
        String word = "";
        do
        {
            if (input[index] == '\t' || (input[index] == ' ' && (index <= 0 || input[index - 1] != '\\')) || input[index] == '\n' || input[index] == '\r' || input[index] == '\0')
            {
                if (word.length() > 0)
                {
                    results[word_count] = word;
                    ++word_count;
                    if (word_count >= MAX_WORDS)
                    {
                        return word_count;
                    }
                    word = "";
                }

                if (input[index] == '\n' || input[index] == '\0')
                {
                    return word_count;
                }
                index++;
            }
            else
            {
                if (input[index] != '\\')
                {
                    word += input[index];
                }

                index++;
            }
        } while (1);
    }

    void greet()
    {
        Serial.println("CYD-Klipper " REPO_VERSION);
        Serial.println("Type 'help' for serial console command list");
        Serial.print("> ");
    }

    bool verify_arg_count(int got, int expected)
    {
        if (got != expected)
        {
            Serial.printf("Command expects %d argument%s, %d given.\n", expected - 1, expected == 2 ? "" : "s", got - 1);
            return false;
        }
        return true;
    }

    // main "engine": non-blockingly read until newline found, parse, execute.
    void run()
    {
        if (global_disable_serial_console)
            return;

        static char cmdline[MAX_COMDLINE_SIZE + 1] = {0};
        if (!read_string_until('\n', cmdline, MAX_COMDLINE_SIZE))
            return;

        String argv[MAX_WORDS];
        int argc = tokenize(argv, cmdline);

        if (argc > 0)
        {
            do
            {
                int cmd_id = find_command(argv[0]);
                if (cmd_id == -1)
                    break;
                if (!verify_arg_count(argc, commandHandlers[cmd_id].argc))
                    break;
                commandHandlers[cmd_id].function(argv);
            } while (0);
        }

        Serial.print("> ");
    }

}