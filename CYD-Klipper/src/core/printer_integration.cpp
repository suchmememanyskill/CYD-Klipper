#include "printer_integration.hpp"
#include "lv_setup.h"
#include "screen_driver.h"
#include <HardwareSerial.h>

static char blank[] = { '\0' };
static unsigned char current_printer_index = 0;
static unsigned char last_announced_printer_index = 0;
static unsigned char total_printers;
static BasePrinter** registered_printers;
static PrinterDataMinimal* minimal_data_copy;
static PrinterData* printer_data_copy;

BasePrinter::BasePrinter(unsigned char index)
{
    config_index = index;
    printer_config = &global_config.printer_config[index];
    memset(&printer_data, 0, sizeof(PrinterData));
    // TODO: Fetch printer config and global config
}

PrinterData* BasePrinter::AnnouncePrinterData()
{
    char* old_state_message = printer_data_copy->state_message;
    char* old_print_filename = printer_data_copy->print_filename;
    char* old_popup_message = printer_data_copy->popup_message;
    PrinterState old_state = printer_data_copy->state;
    bool no_free = current_printer_index != last_announced_printer_index;

    last_announced_printer_index = current_printer_index;
    memcpy(printer_data_copy, &printer_data, sizeof(PrinterData));

    if (printer_data_copy->state_message == NULL)
    {
        printer_data_copy->state_message = blank;
    }

    if (printer_data_copy->print_filename == NULL)
    {
        printer_data_copy->print_filename = blank;
    }

    if (printer_data_copy->popup_message == NULL)
    {
        printer_data_copy->popup_message = blank;
    }
    
    if (old_state_message != printer_data_copy->state_message && old_state_message != NULL && old_state_message != blank && !no_free)
    {
        LOG_F(("Freeing state message '%s' (%x)\n", old_state_message, old_state_message));
        free(old_state_message);
    }

    if (old_print_filename != printer_data_copy->print_filename && old_print_filename != NULL && old_print_filename != blank && !no_free)
    {
        LOG_F(("Freeing print filename '%s' (%x)\n", old_print_filename, old_print_filename));
        free(old_print_filename);
    }

    if (old_state != printer_data_copy->state)
    {
        lv_msg_send(DATA_PRINTER_STATE, get_current_printer());
    }

    if (old_popup_message != printer_data_copy->popup_message)
    {
        if (old_popup_message != NULL && old_popup_message != blank && !no_free)
        {
            LOG_F(("Freeing popup message '%s' (%x)\n", old_popup_message, old_popup_message));
            free(old_popup_message);
        }

        if (printer_data_copy->popup_message != NULL && printer_data_copy->popup_message != blank)
        {
            lv_msg_send(DATA_PRINTER_POPUP, get_current_printer());
        }
    }

    lv_msg_send(DATA_PRINTER_DATA, get_current_printer());
    return printer_data_copy;
}

void initialize_printers(BasePrinter** printers, unsigned char total)
{
    LOG_F(("Initializing %d printers\n", total))
    printer_data_copy = (PrinterData*)malloc(sizeof(PrinterData));
    minimal_data_copy = (PrinterDataMinimal*)malloc(sizeof(PrinterDataMinimal) *  total);
    memset(printer_data_copy, 0, sizeof(PrinterData));
    memset(minimal_data_copy, 0, sizeof(PrinterDataMinimal) *  total);
    registered_printers = printers;
    total_printers = total;
}

BasePrinter* get_current_printer()
{
    return get_printer(current_printer_index);
}

BasePrinter* get_printer(int idx)
{
    return registered_printers[idx];
}

bool BasePrinter::supports_feature(PrinterFeatures feature)
{
    return supported_features & feature == feature;
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
    memcpy(minimal_data_copy, printer_data, sizeof(PrinterDataMinimal) * total_printers);
    lv_msg_send(DATA_PRINTER_MINIMAL, get_current_printer());
}

PrinterDataMinimal* get_printer_data_minimal(int idx)
{
    return &(minimal_data_copy[idx]);
}

void set_current_printer(int idx)
{
    current_printer_index = idx;
    global_config_set_printer(idx);
    set_color_scheme();
    set_invert_display();
}