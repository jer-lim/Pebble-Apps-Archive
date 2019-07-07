#ifndef MAIN_H
#define MAIN_H

#include <pebble.h>

/**
 * MAIN ITEMS
 */
Window *mainWindow;

void handle_init(void);
void handle_deinit(void);

/**
 * MAIN MENU
 */
#define NUM_MAIN_MENU_ITEMS 3
MenuLayer *mainMenuLayer;

uint16_t mainMenu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
int16_t mainMenu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
void mainMenu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data);
void mainMenu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data);
void mainMenu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data);
void mainMenu_load(Window *window);
void mainMenu_unload(Window *window);

/**
 * BUS STOP FETCHER
 */
#define NUM_NEAREST_BUS_STOPS 6

typedef struct BusStop {
	char id[6];
	char name[128];
	char distance[32];
} BusStop;

void fetchBusStops_OutSent(DictionaryIterator *sent, void *context);
void fetchBusStops_OutFailed(DictionaryIterator *failed, AppMessageResult reason, void *context);
void fetchBusStops_InReceived(DictionaryIterator *iterator, void *context);
void fetchBusStops_InFailed(AppMessageResult reason, void *context);
void fetchBusStops();

/**
 * BUS STOP CHOOSER
 */

Window *busStopSelectionWindow;
MenuLayer *busStopMenuLayer;
int selectedBusStopIndex;

uint16_t busStopMenu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
int16_t busStopMenu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
void busStopMenu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data);
void busStopMenu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data);
void busStopMenu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data);
void busStopMenu_long_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data);
void busStopMenu_load(Window *window);
void busStopMenu_unload(Window *window);

/**
 * BUS TIMING FETCHER
 */

typedef struct BusTiming {
	char id[8];
	char timing[10];
	int load;
	TextLayer *idTextLayer;
	TextLayer *timingTextLayer;
	TextLayer *loadTextLayer;
} BusTiming;

void fetchBusTimings_OutSent(DictionaryIterator *sent, void *context);
void fetchBusTimings_OutFailed(DictionaryIterator *failed, AppMessageResult reason, void *context);
void fetchBusTimings_InReceived(DictionaryIterator *iterator, void *context);
void fetchBusTimings_InFailed(AppMessageResult reason, void *context);
void fetchBusTimings(char *busStopId);

/**
 * BUS TIMING DISPLAY
 */

#define BUS_TIMING_ROW_HEIGHT 36;
#define BUS_LOAD_ROW_HEIGHT 20;
#define BUS_TIMING_HEADER_HEIGHT 16;

Window *busTimingWindow;
ScrollLayer *busTimingScrollLayer;
TextLayer *headerTextLayer;

void busTimingDisplay_load(Window *window);
void busTimingDisplay_unload(Window *window);

/**
 * BUS TIMING UPDATER
 */

AppTimer *busTimingUpdateTimer;

void busTimingUpdate_updateDisplay();
void busTimingUpdate_update();
void busTimingUpdate_start();
void busTimingUpdate_end();

/**
 * FAVOURITES
 */

void addFavourite(char *busStopStr);
void showFavourites();

/**
 * HOW TO USE
 */

Window *instructionsWindow;
ScrollLayer *instructionsScrollLayer;
TextLayer *instructionsTextLayer;

void showInstructions();
void showInstructions_load(Window *window);
void showInstructions_unload(Window *window);

/**
 * MESSAGE BOX
 */

TextLayer *messageLayer;

/**
 * APP READY
 */
void awaitAppReady_InReceived(DictionaryIterator *iterator, void *context);
void awaitAppReady_InFailed(AppMessageResult reason, void *context);

/**
 * GLOBAL STUFF
 */

BusStop savedBusStops[NUM_NEAREST_BUS_STOPS];
int numSavedBusStops;

BusTiming savedBusTimings[32];
int numSavedBusTimings;

#endif