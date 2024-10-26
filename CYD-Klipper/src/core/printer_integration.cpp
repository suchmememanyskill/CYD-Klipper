#include "printer_integration.hpp"
#include "lv_setup.h"
#include "screen_driver.h"

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

PrinterData* BasePrinter::AnnouncePrinterData()
{
    char* old_state_message = printer_data_copy->state_message;
    char* old_print_filename = printer_data_copy->print_filename;
    char* old_popup_message = printer_data_copy->print_filename;

    memcpy(printer_data_copy, &printer_data, sizeof(PrinterData));

    if (old_state_message != printer_data_copy->state_message)
    {
        free(old_state_message);
    }

    if (old_print_filename != printer_data_copy->print_filename)
    {
        free(old_print_filename);
    }

    if (printer_data.state != printer_data_copy->state)
    {
        lv_msg_send(DATA_PRINTER_STATE, get_current_printer());
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

int get_current_printer_index()
{
    return current_printer_index;
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

PrinterDataMinimal* get_printer_data_minimal(int idx)
{
    return &(minimal_data_copy[idx]);
}

void BasePrinter::save_printer_config()
{
    // TODO
}


void add_printer()
{

}

void set_current_printer(int idx)
{
    //set_printer_config_index(index);
    set_color_scheme();
    set_invert_display();
}