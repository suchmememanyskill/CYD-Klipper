#include "serial_commands.h"
#include "../../conf/global_config.h"
#include <HardwareSerial.h>
#include <WString.h>

#define MAX_COMDLINE_SIZE 255
#define MAX_WORDS 6

namespace serial_console
{

    /*
     * read_string_until: Non-blocking replacement for Serial.readStringUntil()..
     * With delimeter '\n' acts as 'read line'.
     * If input (line) size exceeds provided buffer's size, that entire input (until next delimeter) is silently discarded.
     * Serial.available() can return true without \n being ever in the buffer, so we don't trust it here.
     */

    bool read_string_until(char delimiter, char *result, int max_len)
    {
        static int index = 0;
        int c;         // read character, -1 if none
        int cnt = 100; // limit on amount of iterations in one go; we're supposed to be non-blocking!

        while ((c = Serial.read()) != -1 && cnt > 0)
        {
            --cnt;

            // backspace
            if (c == 8)
            {
                if (index > 0)
                {
                    if(temporary_config.remote_echo) Serial.print("\x08 \x08"); // overwrite last character with space and move cursor 1 back.
                    index--;
                }
                continue;
            }

            if(temporary_config.remote_echo) Serial.print((char)c); // echo

            // Buffer overflow handling:
            // start treating current buffer as invalid:
            // - stop collecting more data
            // - return false on delimeter, flushing everything collected,
            // - restart collection from scratch after delimeter,
            // - return control as normal.

            if (index >= max_len - 1)
            {
                if (c == delimiter) // got delimeter: flush buffer quietly, restart collection.
                {
                    index = 0;
                    return false;
                }
                else
                    continue; // discard any data past the end of the buffer, keep reading
            }

            result[index++] = c;

            // delimiter was found
            if (c == delimiter)
            {
                result[index] = '\0'; // Null-terminate the string
                index = 0;
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
            if (input[index] == '\t' || input[index] == ' ' || input[index] == '\n' || input[index] == '\r' || input[index] == '\0')
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
                word += input[index];
                index++;
            }
        } while (1);
    }

    void greet()
    {
        Serial.println("CYB-Klipper " REPO_VERSION);
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
        static char cmdline[MAX_COMDLINE_SIZE + 1];
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