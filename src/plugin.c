 /*
 * Keyinator's Mass Actions
 *
 * Copyright (c) 2018-2018 Immanuel "Keyinator" von Neumann
 */

#ifdef _WIN32
#pragma warning (disable : 4100)
#include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "teamspeak/public_errors.h"
#include "teamspeak/public_errors_rare.h"
#include "teamspeak/public_definitions.h"
#include "teamspeak/public_rare_definitions.h"
#include "teamspeak/clientlib_publicdefinitions.h"
#include "ts3_functions.h"
#include "plugin.h"

static struct TS3Functions ts3Functions;

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); (dest)[destSize-1] = '\0'; }
#endif

#define PLUGIN_API_VERSION 22

#define PATH_BUFSIZE 512
#define COMMAND_BUFSIZE 128
#define INFODATA_BUFSIZE 128
#define SERVERINFO_BUFSIZE 256
#define CHANNELINFO_BUFSIZE 512
#define RETURNCODE_BUFSIZE 128

static char* pluginID = NULL;

#ifdef _WIN32
/* Helper function to convert wchar_T to Utf-8 encoded strings on Windows */
static int wcharToUtf8(const wchar_t* str, char** result) {
	int outlen = WideCharToMultiByte(CP_UTF8, 0, str, -1, 0, 0, 0, 0);
	*result = (char*)malloc(outlen);
	if(WideCharToMultiByte(CP_UTF8, 0, str, -1, *result, outlen, 0, 0) == 0) {
		*result = NULL;
		return -1;
	}
	return 0;
}
#endif

/*********************************** Required functions ************************************/
/*
 * If any of these required functions is not implemented, TS3 will refuse to load the plugin
 */

/* Unique name identifying this plugin */
const char* ts3plugin_name() {
#ifdef _WIN32
	/* TeamSpeak expects UTF-8 encoded characters. Following demonstrates a possibility how to convert UTF-16 wchar_t into UTF-8. */
	static char* result = NULL;  /* Static variable so it's allocated only once */
	if(!result) {
		const wchar_t* name = L"Keyinator's MassActions";
		if(wcharToUtf8(name, &result) == -1) {  /* Convert name into UTF-8 encoded result */
			result = "Keyinator's MassActions";  /* Conversion failed, fallback here */
		}
	}
	return result;
#else
	return "Keyinator's MassActions";
#endif
}

/* Plugin version */
const char* ts3plugin_version() {
    return "1.3";
}

/* Plugin API version. Must be the same as the clients API major version, else the plugin fails to load. */
int ts3plugin_apiVersion() {
	return PLUGIN_API_VERSION;
}

/* Plugin author */
const char* ts3plugin_author() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
    return "Keyinator";
}

/* Plugin description */
const char* ts3plugin_description() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
    return "This plugin offers a variety of mass-actions to execute";
}

/* Set TeamSpeak 3 callback functions */
void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    ts3Functions = funcs;
}

/*
 * Custom code called right after loading the plugin. Returns 0 on success, 1 on failure.
 * If the function returns 1 on failure, the plugin will be unloaded again.
 */
int ts3plugin_init() {
    char appPath[PATH_BUFSIZE];
    char resourcesPath[PATH_BUFSIZE];
    char configPath[PATH_BUFSIZE];
	char pluginPath[PATH_BUFSIZE];

    /* Your plugin init code here */
    printf("PLUGIN: init\n");

    /* Example on how to query application, resources and configuration paths from client */
    /* Note: Console client returns empty string for app and resources path */
    ts3Functions.getAppPath(appPath, PATH_BUFSIZE);
    ts3Functions.getResourcesPath(resourcesPath, PATH_BUFSIZE);
    ts3Functions.getConfigPath(configPath, PATH_BUFSIZE);
	ts3Functions.getPluginPath(pluginPath, PATH_BUFSIZE, pluginID);

	printf("PLUGIN: App path: %s\nResources path: %s\nConfig path: %s\nPlugin path: %s\n", appPath, resourcesPath, configPath, pluginPath);

    return 0;  /* 0 = success, 1 = failure, -2 = failure but client will not show a "failed to load" warning */
	/* -2 is a very special case and should only be used if a plugin displays a dialog (e.g. overlay) asking the user to disable
	 * the plugin again, avoiding the show another dialog by the client telling the user the plugin failed to load.
	 * For normal case, if a plugin really failed to load because of an error, the correct return value is 1. */
}

/* Custom code called right before the plugin is unloaded */
void ts3plugin_shutdown() {
    /* Your plugin cleanup code here */
    printf("PLUGIN: shutdown\n");

	/*
	 * Note:
	 * If your plugin implements a settings dialog, it must be closed and deleted here, else the
	 * TeamSpeak client will most likely crash (DLL removed but dialog from DLL code still open).
	 */

	/* Free pluginID if we registered it */
	if(pluginID) {
		free(pluginID);
		pluginID = NULL;
	}
}

/****************************** Optional functions ********************************/
/*
 * Following functions are optional, if not needed you don't need to implement them.
 */

/* Tell client if plugin offers a configuration window. If this function is not implemented, it's an assumed "does not offer" (PLUGIN_OFFERS_NO_CONFIGURE). */
int ts3plugin_offersConfigure() {
	printf("PLUGIN: offersConfigure\n");
	/*
	 * Return values:
	 * PLUGIN_OFFERS_NO_CONFIGURE         - Plugin does not implement ts3plugin_configure
	 * PLUGIN_OFFERS_CONFIGURE_NEW_THREAD - Plugin does implement ts3plugin_configure and requests to run this function in an own thread
	 * PLUGIN_OFFERS_CONFIGURE_QT_THREAD  - Plugin does implement ts3plugin_configure and requests to run this function in the Qt GUI thread
	 */
	return PLUGIN_OFFERS_NO_CONFIGURE;  /* In this case ts3plugin_configure does not need to be implemented */
}

/* Plugin might offer a configuration window. If ts3plugin_offersConfigure returns 0, this function does not need to be implemented. */
void ts3plugin_configure(void* handle, void* qParentWidget) {
    printf("PLUGIN: configure\n");
}

/*
 * If the plugin wants to use error return codes, plugin commands, hotkeys or menu items, it needs to register a command ID. This function will be
 * automatically called after the plugin was initialized. This function is optional. If you don't use these features, this function can be omitted.
 * Note the passed pluginID parameter is no longer valid after calling this function, so you must copy it and store it in the plugin.
 */
void ts3plugin_registerPluginID(const char* id) {
	const size_t sz = strlen(id) + 1;
	pluginID = (char*)malloc(sz * sizeof(char));
	_strcpy(pluginID, sz, id);  /* The id buffer will invalidate after exiting this function */
	printf("PLUGIN: registerPluginID: %s\n", pluginID);
}

/* Required to release the memory for parameter "data" allocated in ts3plugin_infoData and ts3plugin_initMenus */
void ts3plugin_freeMemory(void* data) {
	free(data);
}

/*
 * Plugin requests to be always automatically loaded by the TeamSpeak 3 client unless
 * the user manually disabled it in the plugin dialog.
 * This function is optional. If missing, no autoload is assumed.
 */
int ts3plugin_requestAutoload() {
	return 0;  /* 1 = request autoloaded, 0 = do not request autoload */
}

/* Helper function to create a menu item */
static struct PluginMenuItem* createMenuItem(enum PluginMenuType type, int id, const char* text, const char* icon) {
	struct PluginMenuItem* menuItem = (struct PluginMenuItem*)malloc(sizeof(struct PluginMenuItem));
	menuItem->type = type;
	menuItem->id = id;
	_strcpy(menuItem->text, PLUGIN_MENU_BUFSZ, text);
	_strcpy(menuItem->icon, PLUGIN_MENU_BUFSZ, icon);
	return menuItem;
}

/* Some makros to make the code to create menu items a bit more readable */
#define BEGIN_CREATE_MENUS(x) const size_t sz = x + 1; size_t n = 0; *menuItems = (struct PluginMenuItem**)malloc(sizeof(struct PluginMenuItem*) * sz);
#define CREATE_MENU_ITEM(a, b, c, d) (*menuItems)[n++] = createMenuItem(a, b, c, d);
#define END_CREATE_MENUS (*menuItems)[n++] = NULL; assert(n == sz);

/*
 * Menu IDs for this plugin. Pass these IDs when creating a menuitem to the TS3 client. When the menu item is triggered,
 * ts3plugin_onMenuItemEvent will be called passing the menu ID of the triggered menu item.
 * These IDs are freely choosable by the plugin author. It's not really needed to use an enum, it just looks prettier.
 */
enum {
	MENU_ID_GLOBAL_1,
	MENU_ID_GLOBAL_2,
	MENU_ID_GLOBAL_3,
	MENU_ID_GLOBAL_4,
	MENU_ID_GLOBAL_5,
	MENU_ID_GLOBAL_6,
	MENU_ID_GLOBAL_7,
	MENU_ID_GLOBAL_8,
	MENU_ID_GLOBAL_9,
	MENU_ID_GLOBAL_10,
	MENU_ID_GLOBAL_11,
	MENU_ID_GLOBAL_12,
	MENU_ID_GLOBAL_13,
	MENU_ID_GLOBAL_14,
	MENU_ID_GLOBAL_15,
	MENU_ID_GLOBAL_16,
	MENU_ID_GLOBAL_17,
	MENU_ID_GLOBAL_18,
	MENU_ID_GLOBAL_19,
	MENU_ID_GLOBAL_20,
	MENU_ID_GLOBAL_21,
	MENU_ID_GLOBAL_22,
	MENU_ID_GLOBAL_23,
	MENU_ID_GLOBAL_24,
	MENU_ID_GLOBAL_25,
	MENU_ID_GLOBAL_26,
	MENU_ID_GLOBAL_27,
	MENU_ID_GLOBAL_28,
	MENU_ID_CHANNEL_1,
	MENU_ID_CHANNEL_2,
	MENU_ID_CHANNEL_3,
	MENU_ID_CHANNEL_4,
	MENU_ID_CHANNEL_5,
	MENU_ID_CHANNEL_6,
	MENU_ID_CHANNEL_7,
	MENU_ID_CHANNEL_8,
	MENU_ID_CHANNEL_9,
	MENU_ID_CHANNEL_10,
	MENU_ID_CHANNEL_11,
	MENU_ID_CHANNEL_12,
	MENU_ID_CHANNEL_13,
	MENU_ID_CHANNEL_14,
	MENU_ID_CLIENT_1,
	MENU_ID_CLIENT_2
};

/*
 * Initialize plugin menus.
 * This function is called after ts3plugin_init and ts3plugin_registerPluginID. A pluginID is required for plugin menus to work.
 * Both ts3plugin_registerPluginID and ts3plugin_freeMemory must be implemented to use menus.
 * If plugin menus are not used by a plugin, do not implement this function or return NULL.
 */
void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon) {
	/*
	 * Create the menus
	 * There are three types of menu items:
	 * - PLUGIN_MENU_TYPE_CLIENT:  Client context menu
	 * - PLUGIN_MENU_TYPE_CHANNEL: Channel context menu
	 * - PLUGIN_MENU_TYPE_GLOBAL:  "Plugins" menu in menu bar of main window
	 *
	 * Menu IDs are used to identify the menu item when ts3plugin_onMenuItemEvent is called
	 *
	 * The menu text is required, max length is 128 characters
	 *
	 * The icon is optional, max length is 128 characters. When not using icons, just pass an empty string.
	 * Icons are loaded from a subdirectory in the TeamSpeak client plugins folder. The subdirectory must be named like the
	 * plugin filename, without dll/so/dylib suffix
	 * e.g. for "test_plugin.dll", icon "1.png" is loaded from <TeamSpeak 3 Client install dir>\plugins\test_plugin\1.png
	 */

	BEGIN_CREATE_MENUS(7);  /* IMPORTANT: Number of menu items must be correct! */
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_1, "[MOVING]","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_2, "Move all clients into own channel","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_25,"","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_3, "[KICKING]","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_4, "=[clients in channel]","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_5, "==[from channel]","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_6, "everyone (but you)","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_7, "everyone","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_8, "==[from server]","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_9, "everyone (but you)","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_10, "everyone","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_11, "=[clients in server]","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_12, "==[from channel]","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_13, "everyone (but you)","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_14, "everyone","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_15, "==[from server]","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_16, "everyone (but you)","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_16, "everyone","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_26,"","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_28, "[TALKPOWER]","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_18, "Give everyone talkpower","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_19, "Take everyones talkpower","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_27,"","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_20, "[MISC]","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_21, "ACTIVATE FOR THIS SESSION","");
	ts3Functions.setPluginMenuEnabled(pluginID, 20, 1);
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_22, "DEACTIVATE FOR THIS SESSION","");
	ts3Functions.setPluginMenuEnabled(pluginID, 21, 0);
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_23, "Delete every channel","");
	ts3Functions.setPluginMenuEnabled(pluginID, 22, 0);
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_24, "Delete every empty channel","");
	ts3Functions.setPluginMenuEnabled(pluginID, 23, 0);

	/* CHANNEL - CHANNEL - CHANNEL - CHANNEL - CHANNEL - CHANNEL - CHANNEL - CHANNEL - CHANNEL - CHANNEL - CHANNEL - CHANNEL - CHANNEL */

	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_1, "[MOVING]","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_11, "=[from this channel]", "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_2, "to your channel", "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_12, "=[to this channel]", "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_13, "your channel","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_14, "whole server", "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_3, "","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_4, "[KICKING]", "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_5, "=[from channel]", "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_6, "everyone (but you)", "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_7, "everyone", "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_8, "=[from server]", "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_9, "everyone (but you)", "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_10, "everyone", "");

	/* CLIENT - CLIENT - CLIENT - CLIENT - CLIENT - CLIENT - CLIENT - CLIENT - CLIENT - CLIENT - CLIENT - CLIENT - CLIENT */

	/*CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CLIENT_1, "Client item 1","");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CLIENT_2, "Client item 2",""); */

	END_CREATE_MENUS;  /* Includes an assert checking if the number of menu items matched */

	/*
	 * Specify an optional icon for the plugin. This icon is used for the plugins submenu within context and main menus
	 * If unused, set menuIcon to NULL
	 */
	*menuIcon = (char*)malloc(PLUGIN_MENU_BUFSZ * sizeof(char));
	_strcpy(*menuIcon, PLUGIN_MENU_BUFSZ, "NULL");

	/*
	 * Menus can be enabled or disabled with: ts3Functions.setPluginMenuEnabled(pluginID, menuID, 0|1);
	 * Test it with plugin command: /test enablemenu <menuID> <0|1>
	 * Menus are enabled by default. Please note that shown menus will not automatically enable or disable when calling this function to
	 * ensure Qt menus are not modified by any thread other the UI thread. The enabled or disable state will change the next time a
	 * menu is displayed.
	 */
	/* For example, this would disable MENU_ID_GLOBAL_2: */
	/* ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_GLOBAL_2, 0); */

	/* All memory allocated in this function will be automatically released by the TeamSpeak client later by calling ts3plugin_freeMemory */
}

/************************** TeamSpeak callbacks ***************************/
/*
 * Following functions are optional, feel free to remove unused callbacks.
 * See the clientlib documentation for details on each function.
 */

/* Clientlib */

void ts3plugin_onMenuItemEvent(uint64 serverConnectionHandlerID, enum PluginMenuType type, int menuItemID, uint64 selectedItemID) {
	printf("PLUGIN: onMenuItemEvent: serverConnectionHandlerID=%llu, type=%d, menuItemID=%d, selectedItemID=%llu\n", (long long unsigned int)serverConnectionHandlerID, type, menuItemID, (long long unsigned int)selectedItemID);
	switch(type) {
		case PLUGIN_MENU_TYPE_GLOBAL:
			/* Global menu item was triggered. selectedItemID is unused and set to zero. */
			switch(menuItemID) {
				case MENU_ID_GLOBAL_2: {
					/* Menu global 1 was triggered */
					anyID *ClientList;
					ts3Functions.getClientList(serverConnectionHandlerID, &ClientList);

					anyID myID;
					ts3Functions.getClientID(serverConnectionHandlerID, &myID);

					uint64 myChannel;
					ts3Functions.getChannelOfClient(serverConnectionHandlerID, myID, &myChannel);

					for (int c = 0; ClientList[c]; c++)
					{
						uint64 clientChannel;
						ts3Functions.getChannelOfClient(serverConnectionHandlerID, ClientList[c], &clientChannel);

						if (clientChannel != myChannel)
						{
							ts3Functions.requestClientMove(serverConnectionHandlerID, ClientList[c], myChannel, "", 0);
						}
					}
					break;
				}
				case MENU_ID_GLOBAL_6: {
					/* Menu global 2 was triggered */
					anyID *ClientList;
					ts3Functions.getClientList(serverConnectionHandlerID, &ClientList);

					anyID myID;
					ts3Functions.getClientID(serverConnectionHandlerID, &myID);

					uint64 myChannel;
					ts3Functions.getChannelOfClient(serverConnectionHandlerID, myID, &myChannel);

					for (int c = 0; ClientList[c]; c++)
					{
						uint64 clientChannel;
						ts3Functions.getChannelOfClient(serverConnectionHandlerID, ClientList[c], &clientChannel);

						if ((clientChannel == myChannel) && (ClientList[c] != myID))
						{
							ts3Functions.requestClientKickFromChannel(serverConnectionHandlerID, ClientList[c], "", NULL);
							
						}
					}
					break;
				}
				case MENU_ID_GLOBAL_7: {
					/* Menu global 3 was triggered */
					anyID *ClientList;
					ts3Functions.getClientList(serverConnectionHandlerID, &ClientList);

					anyID myID;
					ts3Functions.getClientID(serverConnectionHandlerID, &myID);

					uint64 myChannel;
					ts3Functions.getChannelOfClient(serverConnectionHandlerID, myID, &myChannel);

					for (int c = 0; ClientList[c]; c++)
					{
						uint64 clientChannel;
						ts3Functions.getChannelOfClient(serverConnectionHandlerID, ClientList[c], &clientChannel);

						if (clientChannel == myChannel)
						{
							ts3Functions.requestClientKickFromChannel(serverConnectionHandlerID, ClientList[c], "", NULL);
						}
					}
					break;
				}
				case MENU_ID_GLOBAL_9: {
					/* Menu global 4 was triggered */
					anyID *ClientList;
					ts3Functions.getClientList(serverConnectionHandlerID, &ClientList);

					anyID myID;
					ts3Functions.getClientID(serverConnectionHandlerID, &myID);

					uint64 myChannel;
					ts3Functions.getChannelOfClient(serverConnectionHandlerID, myID, &myChannel);

					for (int c = 0; ClientList[c]; c++)
					{
						uint64 clientChannel;
						ts3Functions.getChannelOfClient(serverConnectionHandlerID, ClientList[c], &clientChannel);

						if ((clientChannel == myChannel) && (ClientList[c] != myID))
						{
							ts3Functions.requestClientKickFromServer(serverConnectionHandlerID, ClientList[c], "", NULL);

						}
					}
					break;
				}
				case MENU_ID_GLOBAL_10:
					/* Menu global 5 was triggered */
					anyID *ClientList;
					ts3Functions.getClientList(serverConnectionHandlerID, &ClientList);

					anyID myID;
					ts3Functions.getClientID(serverConnectionHandlerID, &myID);

					uint64 myChannel;
					ts3Functions.getChannelOfClient(serverConnectionHandlerID, myID, &myChannel);

					for (int c = 0; ClientList[c]; c++)
					{
						uint64 clientChannel;
						ts3Functions.getChannelOfClient(serverConnectionHandlerID, ClientList[c], &clientChannel);

						if ((clientChannel == myChannel) && (ClientList[c] != myID))
						{
							ts3Functions.requestClientKickFromServer(serverConnectionHandlerID, ClientList[c], "", NULL);

						}
					}
					ts3Functions.requestClientKickFromServer(serverConnectionHandlerID, myID, "", NULL);
					break;
										/*RTEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE*/
				case MENU_ID_GLOBAL_13: {
					/* Menu global 2 was triggered */
					anyID *ClientList;
					ts3Functions.getClientList(serverConnectionHandlerID, &ClientList);

					anyID myID;
					ts3Functions.getClientID(serverConnectionHandlerID, &myID);

					uint64 myChannel;
					ts3Functions.getChannelOfClient(serverConnectionHandlerID, myID, &myChannel);

					for (int c = 0; ClientList[c]; c++)
					{
						uint64 clientChannel;
						ts3Functions.getChannelOfClient(serverConnectionHandlerID, ClientList[c], &clientChannel);

						if (ClientList[c] != myID) {
							ts3Functions.requestClientKickFromChannel(serverConnectionHandlerID, ClientList[c], "", NULL);
						}

					}
					break;
				}
				case MENU_ID_GLOBAL_14: {
					/* Menu global 3 was triggered */
					anyID *ClientList;
					ts3Functions.getClientList(serverConnectionHandlerID, &ClientList);

					anyID myID;
					ts3Functions.getClientID(serverConnectionHandlerID, &myID);

					uint64 myChannel;
					ts3Functions.getChannelOfClient(serverConnectionHandlerID, myID, &myChannel);

					for (int c = 0; ClientList[c]; c++)
					{
						uint64 clientChannel;
						ts3Functions.getChannelOfClient(serverConnectionHandlerID, ClientList[c], &clientChannel);

						ts3Functions.requestClientKickFromChannel(serverConnectionHandlerID, ClientList[c], "", NULL);
					}
					break;
				}
				case MENU_ID_GLOBAL_16: {
					/* Menu global 4 was triggered */
					anyID *ClientList;
					ts3Functions.getClientList(serverConnectionHandlerID, &ClientList);

					anyID myID;
					ts3Functions.getClientID(serverConnectionHandlerID, &myID);

					uint64 myChannel;
					ts3Functions.getChannelOfClient(serverConnectionHandlerID, myID, &myChannel);

					for (int c = 0; ClientList[c]; c++)
					{
						uint64 clientChannel;
						ts3Functions.getChannelOfClient(serverConnectionHandlerID, ClientList[c], &clientChannel);

						if (ClientList[c] != myID) {
							ts3Functions.requestClientKickFromServer(serverConnectionHandlerID, ClientList[c], "", NULL);
						}
					}
					break;
				}
				case MENU_ID_GLOBAL_17: {
					/* Menu global 5 was triggered */
					anyID *ClientList;
					ts3Functions.getClientList(serverConnectionHandlerID, &ClientList);

					anyID myID;
					ts3Functions.getClientID(serverConnectionHandlerID, &myID);

					uint64 myChannel;
					ts3Functions.getChannelOfClient(serverConnectionHandlerID, myID, &myChannel);

					for (int c = 0; ClientList[c]; c++)
					{
						uint64 clientChannel;
						ts3Functions.getChannelOfClient(serverConnectionHandlerID, ClientList[c], &clientChannel);

						if (ClientList[c] != myID) {
							ts3Functions.requestClientKickFromServer(serverConnectionHandlerID, ClientList[c], "", NULL);
						}
					}
					ts3Functions.requestClientKickFromServer(serverConnectionHandlerID, myID, "", NULL);
					break;
				}
				case MENU_ID_GLOBAL_18: {
					anyID *ClientList;
					ts3Functions.getClientList(serverConnectionHandlerID, &ClientList);

					anyID myID;
					ts3Functions.getClientID(serverConnectionHandlerID, &myID);

					uint64 myChannel;
					ts3Functions.getChannelOfClient(serverConnectionHandlerID, myID, &myChannel);

					for (int c = 0; ClientList[c]; c++)
					{
						uint64 clientChannel;
						ts3Functions.getChannelOfClient(serverConnectionHandlerID, ClientList[c], &clientChannel);

						if (clientChannel == myChannel)
						{
							ts3Functions.requestClientSetIsTalker(serverConnectionHandlerID, ClientList[c], 1, NULL);
						}
					}
					break;
				}
				case MENU_ID_GLOBAL_19: {
					anyID *ClientList;
					ts3Functions.getClientList(serverConnectionHandlerID, &ClientList);

					anyID myID;
					ts3Functions.getClientID(serverConnectionHandlerID, &myID);

					uint64 myChannel;
					ts3Functions.getChannelOfClient(serverConnectionHandlerID, myID, &myChannel);

					for (int c = 0; ClientList[c]; c++)
					{
						uint64 clientChannel;
						ts3Functions.getChannelOfClient(serverConnectionHandlerID, ClientList[c], &clientChannel);

						if (clientChannel == myChannel)
						{
							ts3Functions.requestClientSetIsTalker(serverConnectionHandlerID, ClientList[c], 0, NULL);
						}
					}
					break;
				}
				case MENU_ID_GLOBAL_21: {
					/* Activate */
					for (int c = 21; c <= 23; c++)
					{
						ts3Functions.setPluginMenuEnabled(pluginID, c, 1);
					}
					ts3Functions.setPluginMenuEnabled(pluginID, 20, 0);
					break;
				}
				case MENU_ID_GLOBAL_22: {
					/* Deactivate */
					for (int c = 21; c <= 23; c++)
					{
						ts3Functions.setPluginMenuEnabled(pluginID, c, 0);
					}
					ts3Functions.setPluginMenuEnabled(pluginID, 20, 1);
					break;
				}
				case MENU_ID_GLOBAL_23: {
					uint64 *ChannelList;
					ts3Functions.getChannelList(serverConnectionHandlerID, &ChannelList);

					for (int c = 0; ChannelList[c]; c++)
					{
						ts3Functions.requestChannelDelete(serverConnectionHandlerID, ChannelList[c], 1, NULL);
					}
					break;
				}
				case MENU_ID_GLOBAL_24: {
					uint64 *ChannelList;
					ts3Functions.getChannelList(serverConnectionHandlerID, &ChannelList);

					for (int c = 0; ChannelList[c]; c++)
					{
						ts3Functions.requestChannelDelete(serverConnectionHandlerID, ChannelList[c], 0, NULL);
					}
					break;
				}
				default:
					break;
			}
		case PLUGIN_MENU_TYPE_CHANNEL:
			/* Channel contextmenu item was triggered. selectedItemID is the channelID of the selected channel */
			switch (menuItemID) {
				case MENU_ID_CHANNEL_2: {
					anyID ClientID;
					ts3Functions.getClientID(serverConnectionHandlerID, &ClientID);

					uint64 ClientChannel;
					ts3Functions.getChannelOfClient(serverConnectionHandlerID, ClientID, &ClientChannel);

					anyID *ChannelClients;
					ts3Functions.getChannelClientList(serverConnectionHandlerID, selectedItemID, &ChannelClients);

					for (int c = 0; ChannelClients[c]; c++) {
						ts3Functions.requestClientMove(serverConnectionHandlerID, ChannelClients[c], ClientChannel, "", NULL);
					}

					break;
				}
				case MENU_ID_CHANNEL_6: {
					anyID ClientID;
					ts3Functions.getClientID(serverConnectionHandlerID, &ClientID);

					anyID *ChannelClients;
					ts3Functions.getChannelClientList(serverConnectionHandlerID, selectedItemID, &ChannelClients);

					for (int c = 0; ChannelClients[c]; c++) {
						if (ChannelClients[c] != ClientID) {
							ts3Functions.requestClientKickFromChannel(serverConnectionHandlerID, ChannelClients[c], "", NULL);
						}
					}

					break;
				}
				case MENU_ID_CHANNEL_7: {

					anyID *ChannelClients;
					ts3Functions.getChannelClientList(serverConnectionHandlerID, selectedItemID, &ChannelClients);

					for (int c = 0; ChannelClients[c]; c++) {
							ts3Functions.requestClientKickFromChannel(serverConnectionHandlerID, ChannelClients[c], "", NULL);
					}

					break;
				}
				case MENU_ID_CHANNEL_9: {
					anyID ClientID;
					ts3Functions.getClientID(serverConnectionHandlerID, &ClientID);

					uint64 ClientChannel;
					ts3Functions.getChannelOfClient(serverConnectionHandlerID, ClientID, &ClientChannel);

					anyID *ChannelClients;
					ts3Functions.getChannelClientList(serverConnectionHandlerID, selectedItemID, &ChannelClients);

					for (int c = 0; ChannelClients[c]; c++) {
						if (ChannelClients[c] != ClientID) {
							ts3Functions.requestClientKickFromServer(serverConnectionHandlerID, ChannelClients[c], "", NULL);
						}
					}

					break;
				}
				case MENU_ID_CHANNEL_10: {
					anyID ClientID;
					ts3Functions.getClientID(serverConnectionHandlerID, &ClientID);

					uint64 ClientChannel;
					ts3Functions.getChannelOfClient(serverConnectionHandlerID, ClientID, &ClientChannel);

					anyID *ChannelClients;
					ts3Functions.getChannelClientList(serverConnectionHandlerID, selectedItemID, &ChannelClients);

					for (int c = 0; ChannelClients[c]; c++) {
						if (ChannelClients[c] != ClientID) {
							ts3Functions.requestClientKickFromServer(serverConnectionHandlerID, ChannelClients[c], "", NULL);
						}
					}

					ts3Functions.requestClientKickFromServer(serverConnectionHandlerID, ClientID, "", NULL);

					break;
				}
				case MENU_ID_CHANNEL_13: {
					anyID ClientID;
					ts3Functions.getClientID(serverConnectionHandlerID, &ClientID);

					uint64 ClientChannel;
					ts3Functions.getChannelOfClient(serverConnectionHandlerID, ClientID, &ClientChannel);

					anyID *ChannelClients;
					ts3Functions.getChannelClientList(serverConnectionHandlerID, ClientChannel, &ChannelClients);

					for (int c = 0; ChannelClients[c]; c++) {
						ts3Functions.requestClientMove(serverConnectionHandlerID, ChannelClients[c], selectedItemID, "", NULL);
					}

					break;
				}
				case MENU_ID_CHANNEL_14: {

					anyID *ServerClients;
					ts3Functions.getClientList(serverConnectionHandlerID, &ServerClients);

					for (int c = 0; ServerClients[c]; c++) {
						uint64 ClientChannel;
						ts3Functions.getChannelOfClient(serverConnectionHandlerID, ServerClients[c], &ClientChannel);

						if (ClientChannel != selectedItemID) {
							ts3Functions.requestClientMove(serverConnectionHandlerID, ServerClients[c], selectedItemID, "", NULL);
						}
					}

					break;
				}
				default:
					break;
			}
			break;
		case PLUGIN_MENU_TYPE_CLIENT:
			/* Client contextmenu item was triggered. selectedItemID is the clientID of the selected client */
			switch (menuItemID) {
			case MENU_ID_CLIENT_1:
				/* Menu client 1 was triggered */
				break;
			case MENU_ID_CLIENT_2:
				/* Menu client 2 was triggered */
				break;
			default:
				break;
			}
			break;
		default:
			break;
	}
}