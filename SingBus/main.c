#include "main.h"

/**
 * BEGIN MAIN MENU
 */

uint16_t mainMenu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
	switch (section_index) {
		case 0:
		return NUM_MAIN_MENU_ITEMS;
		default:
		return 0;
	}
}

int16_t mainMenu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
	return MENU_CELL_BASIC_HEADER_HEIGHT;
}

void mainMenu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  // Determine which section we're working with
	switch (section_index) {
		case 0:
      // Draw title text in the section header
		menu_cell_basic_header_draw(ctx, cell_layer, "SingBus Main Menu");
		break;
	}
}

void mainMenu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  // Determine which section we're going to draw in
	switch (cell_index->section) {
		case 0:
      // Use the row to specify which item we'll draw
		switch (cell_index->row) {
			case 0:
				menu_cell_basic_draw(ctx, cell_layer, "Nearby Bus Stops", NULL, NULL);
				break;
			case 1:
				menu_cell_basic_draw(ctx, cell_layer, "Favourites", NULL, NULL);
				break;
			case 2:
				menu_cell_basic_draw(ctx, cell_layer, "How To Use", "/ About The App", NULL);
				break;
		}
		break;
	}
}

void mainMenu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {

  // Use the row to specify which item will receive the select action
	switch (cell_index->row) {
		case 0:
			fetchBusStops();
			break;
		case 1:
			showFavourites();
			break;
		case 2:
			showInstructions();
	}

}

void mainMenu_load(Window *window){

	Layer *windowLayer = window_get_root_layer(window);
	GRect bounds = layer_get_frame(windowLayer);

	mainMenuLayer = menu_layer_create(bounds);
	menu_layer_set_callbacks(mainMenuLayer, NULL, (MenuLayerCallbacks) {
		.get_num_rows = mainMenu_get_num_rows_callback,
		.get_header_height = mainMenu_get_header_height_callback,
		.draw_row = mainMenu_draw_row_callback,
		.draw_header = mainMenu_draw_header_callback,
		.select_click = mainMenu_select_callback
	});
	#ifdef PBL_COLOR
		menu_layer_set_normal_colors(mainMenuLayer, GColorCobaltBlue, GColorWhite);
		menu_layer_set_highlight_colors(mainMenuLayer, GColorCeleste, GColorDukeBlue);
	#else
		//menu_layer_set_normal_colors(mainMenuLayer, GColorWhite, GColorBlack);
	#endif

	
	layer_add_child(windowLayer, menu_layer_get_layer(mainMenuLayer));

	GRect messageBounds = (GRect) {
		.origin = (GPoint) {.x = 15, .y = (bounds.size.h / 2 - 30)},
		.size = (GSize) {.w = (bounds.size.w - 30), .h = 60}
	};
	messageLayer = text_layer_create(messageBounds);
	text_layer_set_text(messageLayer, "Connecting to phone...");
	text_layer_set_font(messageLayer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(messageLayer, GTextAlignmentCenter);
	#ifdef PBL_COLOR
		text_layer_set_background_color(messageLayer, GColorJaegerGreen);
		text_layer_set_text_color(messageLayer, GColorWhite);
	#endif
	layer_add_child(windowLayer, text_layer_get_layer(messageLayer));
	
	// App Logging!
	APP_LOG(APP_LOG_LEVEL_INFO, "[UI] Main Menu pushed");
}

void mainMenu_unload(Window *window){
	menu_layer_destroy(mainMenuLayer);
}

/**
 * END MAIN MENU
 */

/**
 * BEGIN BUS STOP FETCHER
 */

void fetchBusStops_OutSent(DictionaryIterator *sent, void *context){
	APP_LOG(APP_LOG_LEVEL_INFO,"[PBJS] Message delivered - Fetch Bus Stops");
}

void fetchBusStops_OutFailed(DictionaryIterator *failed, AppMessageResult reason, void *context){
	APP_LOG(APP_LOG_LEVEL_INFO, "[PBJS] Message delivery failed - Fetch Bus Stops");	
}

void fetchBusStops_InReceived(DictionaryIterator *iterator, void *context){
	char busStopString[255];
	char busStops[NUM_NEAREST_BUS_STOPS][128];
	char split[128];
	static BusStop busStop;
	unsigned int i, j;
	int k, l;
	int busStopNum = 0;
	int flipper = 0;


	Tuple *foundBusStops = dict_find(iterator, 2);
	if(foundBusStops){
		strcpy(busStopString, foundBusStops->value->cstring);
		APP_LOG(APP_LOG_LEVEL_INFO, "[PBJS] Message received - Nearest Bus Stops");

		// Split string into multiple bus stops
		for(i = 0; i < strlen(busStopString); ++i){
			j = 0;
			memset(split, 0, 128);
			while(busStopString[i] != ',' && i < strlen(busStopString)){
				split[j] = busStopString[i];
				j++;
				i++;
			}

			strcpy(busStops[busStopNum++], split);
		}

		l = 0;

		// Split bus stop text into id, name and distance
		for(k = 0; k < busStopNum; ++k){
			for(i = 0; i < strlen(busStops[k]); ++i){
				j = 0;
				memset(split, 0, 128);
				while(busStops[k][i] != '|' && i < strlen(busStops[k])){
					split[j] = busStops[k][i];
					j++;
					i++;
				}
				
				switch (flipper){
					case 0:
						strcpy(busStop.id, split);
						flipper++;
						break;
					case 1:
						strcpy(busStop.name, split);
						flipper++;
						break;
					case 2:
						strcpy(busStop.distance, split);
						flipper = 0;
						break;
				}
			}

			// Save bus stop info
			savedBusStops[l++] = busStop;
			//APP_LOG(APP_LOG_LEVEL_DEBUG, "[BUS STOP] Bus Stop saved into slot %i - #%s, %s away", l - 1, savedBusStops[l-1].id, savedBusStops[l-1].distance);
		}

		numSavedBusStops = l;
	}

	APP_LOG(APP_LOG_LEVEL_INFO, "[BUS STOP] All bus stops saved, creating menu");
	
	// Push the window
	window_stack_push(busStopSelectionWindow, true);
}

void fetchBusStops_InFailed(AppMessageResult reason, void *context){
	APP_LOG(APP_LOG_LEVEL_ERROR, "%i", reason);
}

void fetchBusStops(){

	app_message_register_outbox_sent(fetchBusStops_OutSent);
	app_message_register_outbox_failed(fetchBusStops_OutFailed);
	app_message_register_inbox_received(fetchBusStops_InReceived);
	app_message_register_inbox_dropped(fetchBusStops_InFailed);

	app_message_open(255, 128);

	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	Tuplet tuple = TupletInteger(1, 69);
	dict_write_tuplet(iter, &tuple);
	dict_write_end(iter);
	app_message_outbox_send();
	APP_LOG(APP_LOG_LEVEL_INFO,"[PBJS] Sending Message - Fetch Bus Stops");
}

/**
 * END BUS STOP FETCHER
 */

/**
 * BEGIN BUS STOP MENU
 */

uint16_t busStopMenu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
	switch (section_index) {
		case 0:
		return numSavedBusStops;
		default:
		return 0;
	}
}

int16_t busStopMenu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
	return MENU_CELL_BASIC_HEADER_HEIGHT;
}

void busStopMenu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  // Determine which section we're working with
	switch (section_index) {
		case 0:
      // Draw title text in the section header
		menu_cell_basic_header_draw(ctx, cell_layer, "Select Bus Stop");
		break;
	}
}

void busStopMenu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
	BusStop busStop;
  // Determine which section we're going to draw in
	switch (cell_index->section) {
		case 0:
	      	// Use the row to specify which item we'll draw
			busStop = savedBusStops[cell_index->row];
			menu_cell_basic_draw(ctx, cell_layer, busStop.name, busStop.distance, NULL);
			break;
	}
}

void busStopMenu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {

  // Use the row to specify which item will receive the select action
	selectedBusStopIndex = cell_index->row;
	fetchBusTimings(savedBusStops[selectedBusStopIndex].id);

}

void busStopMenu_long_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {

  // Use the row to specify which item will receive the select action
	selectedBusStopIndex = cell_index->row;
	static char str[128];
	strcpy(str, savedBusStops[selectedBusStopIndex].id);
	strcat(str, ",");
	strcat(str, savedBusStops[selectedBusStopIndex].name);
	addFavourite(str);
	vibes_short_pulse();

}

void busStopMenu_load(Window *window){

	Layer *windowLayer = window_get_root_layer(window);
	GRect bounds = layer_get_frame(windowLayer);

	busStopMenuLayer = menu_layer_create(bounds);
	menu_layer_set_callbacks(busStopMenuLayer, NULL, (MenuLayerCallbacks) {
		.get_num_rows = busStopMenu_get_num_rows_callback,
		.get_header_height = busStopMenu_get_header_height_callback,
		.draw_row = busStopMenu_draw_row_callback,
		.draw_header = busStopMenu_draw_header_callback,
		.select_click = busStopMenu_select_callback,
		.select_long_click = busStopMenu_long_select_callback
	});
	#ifdef PBL_COLOR
		menu_layer_set_normal_colors(busStopMenuLayer, GColorJaegerGreen, GColorWhite);
		menu_layer_set_highlight_colors(busStopMenuLayer, GColorMintGreen, GColorDarkGreen);
	#endif

	menu_layer_set_click_config_onto_window(busStopMenuLayer, window);
	layer_add_child(windowLayer, menu_layer_get_layer(busStopMenuLayer));
	
	// App Logging!
	APP_LOG(APP_LOG_LEVEL_INFO, "[UI] Bus Stop Selection Menu pushed");
}

void busStopMenu_unload(Window *window){
	menu_layer_destroy(busStopMenuLayer);
}

/**
 * END BUS STOP MENU
 */

/**
 * START BUS TIMING FETCHER
 */

void fetchBusTimings_OutSent(DictionaryIterator *sent, void *context){
	APP_LOG(APP_LOG_LEVEL_INFO,"[PBJS] Message delivered - Fetch Bus Timings");
}

void fetchBusTimings_OutFailed(DictionaryIterator *failed, AppMessageResult reason, void *context){
	APP_LOG(APP_LOG_LEVEL_INFO, "[PBJS] Message delivery failed - Fetch Bus Timings");	
}

void fetchBusTimings_InReceived(DictionaryIterator *iterator, void *context){
	char busTimingString[255];
	static char busTimings[32][128];
	char split[128];
	static BusTiming busTiming;
	unsigned int i, j;
	int k, l;
	int busNum = 0;
	int flipper = 0;
	bool isUpdate = false;

	for(k = 0; k < 32; ++k){
		memset(busTimings[k], 0, 128);
	}

	Tuple *foundBusTimings = dict_find(iterator, 4);
	if(!foundBusTimings){
		foundBusTimings = dict_find(iterator, 6);
		isUpdate = true;
	}
	if(foundBusTimings){
		strcpy(busTimingString, foundBusTimings->value->cstring);
		if(isUpdate){
			APP_LOG(APP_LOG_LEVEL_INFO, "[PBJS] Message received - Update Bus Timings");
		}else{
			APP_LOG(APP_LOG_LEVEL_INFO, "[PBJS] Message received - Bus Timings");
		}

		// Split string into multiple buses
		for(i = 0; i < strlen(busTimingString); ++i){
			j = 0;
			memset(split, 0, 128);
			while(busTimingString[i] != ',' && i < strlen(busTimingString)){
				split[j] = busTimingString[i];
				j++;
				i++;
			}
			strcpy(busTimings[busNum++], split);
		}

		l = 0;

		// Split bus text into id, timing, load
		for(k = 0; k < busNum; ++k){
			for(i = 0; i < strlen(busTimings[k]); ++i){
				j = 0;
				memset(split, 0, 128);
				while(busTimings[k][i] != '|' && i < strlen(busTimings[k])){
					split[j] = busTimings[k][i];
					j++;
					i++;
				}
				
				switch (flipper){
					case 0:
						if(!isUpdate) strcpy(busTiming.id, split);
						else strcpy(savedBusTimings[l].id, split);
						flipper++;
						break;
					case 1:
						if(!isUpdate) strcpy(busTiming.timing, split);
						else strcpy(savedBusTimings[l].timing, split);
						flipper++;
						break;
					case 2:
						if(!isUpdate) busTiming.load = split[0] - 48;
						else savedBusTimings[l].load = split[0] - 48;
						flipper = 0;
						break;
				}
			}

			if(!isUpdate){
				// Save bus stop info
				savedBusTimings[l++] = busTiming;
			}else l++;
			//APP_LOG(APP_LOG_LEVEL_DEBUG, "[BUS TIMING] Bus Timing saved into slot %i - %s, %s", l - 1, savedBusTimings[l-1].id, savedBusTimings[l-1].timing);
		}

		numSavedBusTimings = l;
	}

	if(!isUpdate){
		APP_LOG(APP_LOG_LEVEL_INFO, "[BUS TIMING] All bus timings saved, displaying timings");
		// Push the window
		window_stack_push(busTimingWindow, true);
	}else{
		APP_LOG(APP_LOG_LEVEL_INFO, "[BUS TIMING] All bus timings saved, updating timings");
		busTimingUpdate_updateDisplay();
	}
}

void fetchBusTimings_InFailed(AppMessageResult reason, void *context){
	APP_LOG(APP_LOG_LEVEL_ERROR, "%i", reason);
}

void fetchBusTimings(char *busStopId){
	app_message_register_outbox_sent(fetchBusTimings_OutSent);
	app_message_register_outbox_failed(fetchBusTimings_OutFailed);
	app_message_register_inbox_received(fetchBusTimings_InReceived);
	app_message_register_inbox_dropped(fetchBusTimings_InFailed);

	app_message_open(255, 128);

	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	Tuplet tuple = TupletCString(3, busStopId);
	dict_write_tuplet(iter, &tuple);
	dict_write_end(iter);
	app_message_outbox_send();
	APP_LOG(APP_LOG_LEVEL_INFO,"[PBJS] Sending Message - Fetch Bus Timings for %s", busStopId);
}

/**
 * END BUS TIMING FETCHER
 */

/**
 * START BUS TIMING DISPLAY
 */

void busTimingDisplay_load(Window *window){

	Layer *windowLayer = window_get_root_layer(window);
	GRect bounds = layer_get_frame(windowLayer);

	int i = 0;
	TextLayer *tlayer;
	GSize currentContentSize;
	int rowHeight = BUS_TIMING_ROW_HEIGHT;
	int headerHeight = BUS_TIMING_HEADER_HEIGHT;
	int loadHeight = BUS_LOAD_ROW_HEIGHT;

	GRect rowBounds = (GRect) {
		.origin = (GPoint) {.x = 0, .y = headerHeight},
		.size = (GSize) {.w = 144, .h = rowHeight}
	};

	GRect rowTimingBounds = (GRect) {
		.origin = (GPoint) {.x = 84, .y = headerHeight},
		.size = (GSize) {.w = 60, .h = rowHeight}
	};

	GRect rowLoadBounds = (GRect) {
		.origin = (GPoint) {.x = (144/2 - loadHeight/2), .y = (headerHeight + (rowHeight - loadHeight)/2)},
		.size = (GSize) {.w = loadHeight, .h = loadHeight}
	};

	busTimingScrollLayer = scroll_layer_create(bounds);
	scroll_layer_set_content_size(busTimingScrollLayer, (GSize) {
		.w = 144,
		.h = headerHeight
	});

	scroll_layer_set_click_config_onto_window(busTimingScrollLayer, window);
	layer_add_child(windowLayer, scroll_layer_get_layer(busTimingScrollLayer));

	window_set_background_color(window, GColorBlack);

	// Add header
	GRect headerTlayerBounds = (GRect) {
		.origin = (GPoint) {.x = 0, .y = 0},
		.size = (GSize) {.w = 144, .h = headerHeight}
	};
	tlayer = text_layer_create(headerTlayerBounds);
	static char tempStr[128];
	strcpy(tempStr, " ");
	strcat(tempStr, savedBusStops[selectedBusStopIndex].name);
	text_layer_set_text(tlayer, tempStr);
	#ifdef PBL_COLOR
		text_layer_set_background_color(tlayer, GColorCobaltBlue);
		text_layer_set_text_color(tlayer, GColorWhite);
	#endif
	scroll_layer_add_child(busTimingScrollLayer, text_layer_get_layer(tlayer));
	headerTextLayer = tlayer;
	// End header

	// Populate list
	for(i = 0; i < numSavedBusTimings; ++i){

		// Create bus # display
		savedBusTimings[i].idTextLayer = text_layer_create(rowBounds);
		tlayer = savedBusTimings[i].idTextLayer;
		text_layer_set_text(tlayer, savedBusTimings[i].id);
		text_layer_set_font(tlayer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
		#ifdef PBL_COLOR
			if(i % 2 == 1){
				text_layer_set_background_color(tlayer, GColorCobaltBlue);
				text_layer_set_text_color(tlayer, GColorWhite);
			}else{
				text_layer_set_background_color(tlayer, GColorCeleste);
				text_layer_set_text_color(tlayer, GColorDukeBlue);
			}
		#endif
		scroll_layer_add_child(busTimingScrollLayer, text_layer_get_layer(tlayer));
		

		// Create bus timing display
		tlayer = text_layer_create(rowTimingBounds);
		text_layer_set_text(tlayer, savedBusTimings[i].timing);
		text_layer_set_font(tlayer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
		text_layer_set_text_alignment(tlayer, GTextAlignmentRight);
		#ifdef PBL_COLOR
			if(i % 2 == 1){
				text_layer_set_background_color(tlayer, GColorCobaltBlue);
				text_layer_set_text_color(tlayer, GColorWhite);
			}else{
				text_layer_set_background_color(tlayer, GColorCeleste);
				text_layer_set_text_color(tlayer, GColorDukeBlue);
			}
		#endif
		scroll_layer_add_child(busTimingScrollLayer, text_layer_get_layer(tlayer));
		savedBusTimings[i].timingTextLayer = tlayer;

		// Create bus load display
		savedBusTimings[i].loadTextLayer = text_layer_create(rowLoadBounds);
		tlayer = savedBusTimings[i].loadTextLayer;
		#ifdef PBL_COLOR
			if(savedBusTimings[i].load == 1){
				text_layer_set_background_color(tlayer, GColorMalachite);
			}else if(savedBusTimings[i].load == 2){
				text_layer_set_background_color(tlayer, GColorChromeYellow);
			}else{
				text_layer_set_background_color(tlayer, GColorRed);
			}
		#else
			if(savedBusTimings[i].load == 3){
				text_layer_set_background_color(tlayer, GColorBlack);
			}
		#endif
		scroll_layer_add_child(busTimingScrollLayer, text_layer_get_layer(tlayer));
		
		rowBounds.origin.y += BUS_TIMING_ROW_HEIGHT;
		rowTimingBounds.origin.y += BUS_TIMING_ROW_HEIGHT;
		rowLoadBounds.origin.y += BUS_TIMING_ROW_HEIGHT;
		currentContentSize = scroll_layer_get_content_size(busTimingScrollLayer);
		currentContentSize.h += BUS_TIMING_ROW_HEIGHT;
		scroll_layer_set_content_size(busTimingScrollLayer, currentContentSize);

	}
	
	// App Logging!
	APP_LOG(APP_LOG_LEVEL_INFO, "[UI] Bus Timing Display pushed");

	for(i = 0; i < numSavedBusTimings; ++i){
		APP_LOG(APP_LOG_LEVEL_DEBUG, "%p", (void*)savedBusTimings[i].timingTextLayer);
	}

	busTimingUpdate_start();
}

void busTimingDisplay_unload(Window *window){
	int i;
	for(i = 0; i < numSavedBusTimings; ++i){
		text_layer_destroy(savedBusTimings[i].idTextLayer);
		text_layer_destroy(savedBusTimings[i].timingTextLayer);
		text_layer_destroy(savedBusTimings[i].loadTextLayer);

	}
	scroll_layer_destroy(busTimingScrollLayer);
	busTimingUpdate_end();
}

/**
 * END BUS TIMING DISPLAY
 */

/**
 * START BUS TIMING UPDATER
 */

void busTimingUpdate_updateDisplay(){
	int i;
	layer_mark_dirty(scroll_layer_get_layer(busTimingScrollLayer));
	for(i = 0; i < numSavedBusTimings; ++i){
		//text_layer_set_text(savedBusTimings[i].timingTextLayer, savedBusTimings[i].timing);
		#ifdef PBL_COLOR
			if(savedBusTimings[i].load == 1){
				text_layer_set_background_color(savedBusTimings[i].loadTextLayer, GColorMalachite);
			}else if(savedBusTimings[i].load == 2){
				text_layer_set_background_color(savedBusTimings[i].loadTextLayer, GColorChromeYellow);
			}else{
				text_layer_set_background_color(savedBusTimings[i].loadTextLayer, GColorRed);
			}
		#endif
	}
}

void busTimingUpdate_update() {
	app_message_register_outbox_sent(fetchBusTimings_OutSent);
	app_message_register_outbox_failed(fetchBusTimings_OutFailed);
	app_message_register_inbox_received(fetchBusTimings_InReceived);
	app_message_register_inbox_dropped(fetchBusTimings_InFailed);

	app_message_open(255, 128);

	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	Tuplet tuple = TupletCString(5, savedBusStops[selectedBusStopIndex].id);
	dict_write_tuplet(iter, &tuple);
	dict_write_end(iter);
	app_message_outbox_send();
	APP_LOG(APP_LOG_LEVEL_INFO,"[PBJS] Sending Message - Update Bus Timings for %s", savedBusStops[selectedBusStopIndex].id);
	
	busTimingUpdateTimer = app_timer_register(60000, busTimingUpdate_update, NULL);
}

void busTimingUpdate_start() {
	busTimingUpdateTimer = app_timer_register(60000, busTimingUpdate_update, NULL);
	APP_LOG(APP_LOG_LEVEL_INFO, "[BUS TIMING] Bus Timing update loop started");
}

void busTimingUpdate_end() {
	app_timer_cancel(busTimingUpdateTimer);
	busTimingUpdateTimer = NULL;
	APP_LOG(APP_LOG_LEVEL_INFO, "[BUS TIMING] Bus Timing update loop stopped");
}

/**
 * END BUS TIMING UPDATER
 */

/**
 * BEGIN FAVOURITES
 */

void addFavourite(char *busStopStr){
	app_message_open(255, 128);

	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	Tuplet tuple = TupletCString(7, busStopStr);
	dict_write_tuplet(iter, &tuple);
	dict_write_end(iter);
	app_message_outbox_send();
	APP_LOG(APP_LOG_LEVEL_INFO,"[PBJS] Sending Message - Adding bus stop as favourite: %s", busStopStr);
}

void showFavourites(){

	app_message_register_outbox_sent(fetchBusStops_OutSent);
	app_message_register_outbox_failed(fetchBusStops_OutFailed);
	app_message_register_inbox_received(fetchBusStops_InReceived);
	app_message_register_inbox_dropped(fetchBusStops_InFailed);

	app_message_open(255, 128);

	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	Tuplet tuple = TupletInteger(9, 69);
	dict_write_tuplet(iter, &tuple);
	dict_write_end(iter);
	app_message_outbox_send();
	APP_LOG(APP_LOG_LEVEL_INFO,"[PBJS] Sending Message - Show favourites (masquerading as fetch bus stops)");
}

/**
 * END FAVOURITES
 */

/**
 * BEGIN INSTRUCTIONS
 */

void showInstructions(){
	window_stack_push(instructionsWindow, true);
}

void showInstructions_load(Window *window){
	Layer *windowLayer = window_get_root_layer(window);
	GRect bounds = layer_get_frame(windowLayer);

	GRect contentBounds = (GRect) {
		.origin = (GPoint) {.x = 0, .y = 0},
		.size = (GSize) {.w = (bounds.size.w), .h = 800}
	};

	instructionsScrollLayer = scroll_layer_create(bounds);
	scroll_layer_set_content_size(instructionsScrollLayer, (GSize) {
		.w = 144,
		.h = 800
	});

	scroll_layer_set_click_config_onto_window(instructionsScrollLayer, window);
	layer_add_child(windowLayer, scroll_layer_get_layer(instructionsScrollLayer));

	window_set_background_color(window, GColorBlack);

	instructionsTextLayer = text_layer_create(contentBounds);
	text_layer_set_text(instructionsTextLayer, "Basic Usage:\n- Selecting Nearby Bus Stops and then a specific bus stop will display all the buses at that stop. Use up/down to scroll the list.\n- Long press the select button in the bus stop selection menu to save the bus stop as a Favourite. A vibration signals that the bus stop has been saved.\n- In the Favourites menu, long pressing the select button removes the bus stop as a Favourite.\n- The bus timings update automatically every minute.\n\nBus Load Indication:\nGreen - Seating space available\nOrange - No seats available\nRed / Black (OG Pebble) - Bus has little standing space\n\nAbout The App:\nDeveloped by Jeremy Lim (http://jerl.im) in Nov 2015. Visit the Pebble Store page for updates.");
	text_layer_set_font(instructionsTextLayer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	#ifdef PBL_COLOR
		text_layer_set_background_color(instructionsTextLayer, GColorJaegerGreen);
		text_layer_set_text_color(instructionsTextLayer, GColorWhite);
	#endif
	scroll_layer_add_child(instructionsScrollLayer, text_layer_get_layer(instructionsTextLayer));
}

void showInstructions_unload(Window *window){
	scroll_layer_destroy(instructionsScrollLayer);
	text_layer_destroy(instructionsTextLayer);
}

/**
 * END INSTRUCTIONS
 */

/**
 * BEGIN APP READY
 */

void awaitAppReady_InReceived(DictionaryIterator *iterator, void *context){
	Tuple *appReady = dict_find(iterator, 99);
	if(appReady){
		APP_LOG(APP_LOG_LEVEL_INFO, "[PBJS] Message received - App ready");

		text_layer_destroy(messageLayer);
		menu_layer_set_click_config_onto_window(mainMenuLayer, mainWindow);
	}
}

void awaitAppReady_InFailed(AppMessageResult reason, void *context){
	APP_LOG(APP_LOG_LEVEL_ERROR, "%i", reason);
}

/**
 * END APP READY
 */

/**
 * BEGIN MAIN INIT STUFF
 */

void handle_init(void) {

	// Create a window
	mainWindow = window_create();
	window_set_window_handlers(mainWindow, (WindowHandlers) {
		.load = mainMenu_load,
		.unload = mainMenu_unload
	});

	busStopSelectionWindow = window_create();
	window_set_window_handlers(busStopSelectionWindow, (WindowHandlers) {
		.load = busStopMenu_load,
		.unload = busStopMenu_unload
	});

	busTimingWindow = window_create();
	window_set_window_handlers(busTimingWindow, (WindowHandlers) {
		.load = busTimingDisplay_load,
		.unload = busTimingDisplay_unload
	});

	instructionsWindow = window_create();
	window_set_window_handlers(instructionsWindow, (WindowHandlers) {
		.load = showInstructions_load,
		.unload = showInstructions_unload
	});

	app_message_register_inbox_received(awaitAppReady_InReceived);
	app_message_register_inbox_dropped(awaitAppReady_InFailed);
	app_message_open(255, 128);

	// Push the window
	window_stack_push(mainWindow, true);
}

void handle_deinit(void) {
	
	// Destroy the window
	window_destroy(mainWindow);
	window_destroy(busStopSelectionWindow);
	window_destroy(busTimingWindow);
}

int main(void) {
	handle_init();
	app_event_loop();
	handle_deinit();
}

/**
 * END MAIN INIT STUFF
 */
