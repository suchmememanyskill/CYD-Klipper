#include "printer_integration.hpp"

unsigned char current_printer_index = 0;
BasePrinter* registered_printers;

PrinterData* printer_data_copy;
char* state_message_copy;
char* print_filename_copy;
char* popup_message_copy = NULL;

bool available_data_message;
bool available_state_message;
bool available_popup_message;

BasePrinter::BasePrinter(unsigned char index)
 {
    config_index = index;
    // TODO: Fetch printer config and global config
}

PrinterData* BasePrinter::CopyPrinterData()
{
    available_data_message = true;
    available_state_message = printer_data.state != printer_data_copy->state;

    memcpy(printer_data_copy, &printer_data, sizeof(PrinterData));
    printer_data_copy->state_message = state_message_copy;
    printer_data_copy->print_filename = print_filename_copy;
    strcpy(state_message_copy, printer_data.state_message);
    strcpy(print_filename_copy, printer_data.print_filename);
}

void initialize_printers()
{
    printer_data_copy = (PrinterData*)malloc(sizeof(PrinterData));
    state_message_copy = (char*)malloc(256);
    print_filename_copy = (char*)malloc(256);
}

BasePrinter* get_current_printer()
{
    return get_printer(current_printer_index);
}

BasePrinter* get_printer(int idx)
{
    return registered_printers + idx;
}

void store_available_popup_message(const char *message)
{
    if (message != NULL && (!get_current_printer()->global_config->disable_m117_messaging) && (popup_message_copy == NULL || strcmp(popup_message_copy, message)))
    {
        if (popup_message_copy != NULL)
        {
            free(popup_message_copy);
        }

        popup_message_copy = (char*)malloc(strlen(message) + 1);
        strcpy(popup_message_copy, message);
        available_popup_message = true;
    }
}

#define DATA_PRINTER_STATE 1
#define DATA_PRINTER_DATA 2
#define DATA_PRINTER_TEMP_PRESET 3
#define DATA_PRINTER_MINIMAL 4
#define DATA_PRINTER_POPUP 5

void send_available_popup_message()
{
    if (available_popup_message)
    {
        available_data_message = false;
        lv_msg_send(DATA_PRINTER_POPUP, popup_message_copy);
    }
}

void send_available_data_message()
{
    if (available_data_message)
    {
        available_data_message = false;
        lv_msg_send(DATA_PRINTER_DATA, get_current_printer());
    }
}

void send_available_state_message()
{
    if (available_state_message)
    {
        available_state_message = false;
        lv_msg_send(DATA_PRINTER_STATE, get_current_printer());
    }
}