#include "printer_integration.hpp"

unsigned char current_printer_index = 0;
unsigned char total_printers;
BasePrinter* registered_printers;
PrinterDataMinimal* minimal_data_copy;
PrinterData* printer_data_copy;

BasePrinter::BasePrinter(unsigned char index)
{
    config_index = index;

    printer_data.state_message = (char*)malloc(1);
    printer_data.print_filename = (char*)malloc(1);
    printer_data.popup_message = (char*)malloc(1);
    *printer_data.state_message = '\0';
    *printer_data.print_filename = '\0';
    *printer_data.popup_message = '\0';

    // TODO: Fetch printer config and global config
}

#define DATA_PRINTER_STATE 1
#define DATA_PRINTER_DATA 2
#define DATA_PRINTER_TEMP_PRESET 3
#define DATA_PRINTER_MINIMAL 4
#define DATA_PRINTER_POPUP 5

PrinterData* BasePrinter::AnnouncePrinterData()
{
    char* old_state_message = printer_data_copy->state_message;
    char* old_print_filename = printer_data_copy->print_filename;
    char* old_popup_message = printer_data_copy->print_filename;

    memcpy(printer_data_copy, &printer_data, sizeof(PrinterData));

    if (old_state_message != printer_data_copy->state_message)
    {
        free(old_state_message);
        lv_msg_send(DATA_PRINTER_STATE, get_current_printer());
    }
    else if (printer_data.state != printer_data_copy->state)
    {
        lv_msg_send(DATA_PRINTER_STATE, get_current_printer());
    }

    if (old_print_filename != printer_data_copy->print_filename)
    {
        free(old_print_filename);
    }

    if (old_popup_message != printer_data_copy->popup_message)
    {
        free(old_popup_message);
        lv_msg_send(DATA_PRINTER_POPUP, get_current_printer());
    }

    lv_msg_send(DATA_PRINTER_DATA, get_current_printer());
}

void initialize_printers()
{
    printer_data_copy = (PrinterData*)malloc(sizeof(PrinterData));
}

BasePrinter* get_current_printer()
{
    return get_printer(current_printer_index);
}

BasePrinter* get_printer(int idx)
{
    return registered_printers + idx;
}

PrinterData* get_current_printer_data()
{
    return printer_data_copy;
}

unsigned int get_printer_count()
{
    return total_printers;
}

void announce_printer_data_minimal(PrinterDataMinimal* printer_data)
{
    memcpy(printer_data_copy, printer_data, sizeof(PrinterDataMinimal) * total_printers);
    lv_msg_send(DATA_PRINTER_MINIMAL, get_current_printer());
}