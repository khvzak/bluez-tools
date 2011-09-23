/*
 *
 *  bluez-tools - a set of tools to manage bluetooth devices for linux
 *
 *  Copyright (C) 2010-2011  Alexander Orlenko <zxteam@gmail.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "lib/dbus-common.h"
#include "lib/helpers.h"
#include "lib/bluez-api.h"

static gboolean need_unregister = TRUE;
static GMainLoop *mainloop = NULL;

static GHashTable *pin_hash_table = NULL;
static gchar *pin_arg = NULL;

static void read_pin_file(const gchar *filename, GHashTable *pin_hash_table, gboolean first_run)
{
	g_assert(filename != NULL && strlen(filename) > 0);
	g_assert(pin_hash_table != NULL);

	GStatBuf sbuf;
	memset(&sbuf, 0, sizeof(sbuf));
	if (g_stat(filename, &sbuf) != 0) {
		if (first_run) {
			g_printerr("%s: %s\n", filename, strerror(errno));
			exit(EXIT_FAILURE);
		} else {
			return;
		}
	}
	if (!S_ISREG(sbuf.st_mode)) {
		if (first_run) {
			g_printerr("%s: It's not a regular file\n", filename);
			exit(EXIT_FAILURE);
		} else {
			return;
		}
	}
	if (sbuf.st_mode & S_IROTH) {
		if (first_run)
			g_print("Warning! %s is world readable!\n", filename);
	}

	FILE *fp = g_fopen(filename, "r");
	if (!fp) {
		if (first_run) {
			g_printerr("%s: %s\n", filename, strerror(errno));
			exit(EXIT_FAILURE);
		} else {
			return;
		}
	}

	g_hash_table_remove_all(pin_hash_table);

	gchar *line = NULL;
	size_t len = 0;
	ssize_t read;
	guint n = 0;
	GRegex *regex = g_regex_new("^(\\S+)\\s+(\\S+)$", 0, 0, NULL);

	while ((read = getline(&line, &len, fp)) != -1) {
		n++;

		if (g_regex_match_simple("^\\s*(#|$)", line, 0, 0))
			continue;

		GMatchInfo *match_info;
		if (g_regex_match(regex, line, 0, &match_info)) {
			gchar **t = g_match_info_fetch_all(match_info);
			/* Convert MAC to upper case */
			if (g_regex_match_simple("^([0-9a-fA-F]{2}(:|$)){6}$", t[1], 0, 0))
				g_hash_table_insert(pin_hash_table, g_ascii_strup(t[1], -1), g_strdup(t[2]));
			else
				g_hash_table_insert(pin_hash_table, g_strdup(t[1]), g_strdup(t[2]));
			g_strfreev(t);
		} else {
			if (first_run)
				g_print("%d: Invalid line (ignored)\n", n);
		}
		g_match_info_free(match_info);
	}

	g_regex_unref(regex);
	if (line)
		g_free(line);
	fclose(fp);

	first_run = FALSE;

	return;
}

static void signal_handler(int sig)
{
	g_message("%s received", sig == SIGTERM ? "SIGTERM" : (sig == SIGUSR1 ? "SIGUSR1" : "SIGINT"));

	if (sig == SIGUSR1 && pin_arg) {
		/* Re-read PIN's file */
		g_print("Re-reading PIN's file\n");
		read_pin_file(pin_arg, pin_hash_table, FALSE);
	} else if (sig == SIGTERM || sig == SIGINT) {


		if (g_main_loop_is_running(mainloop))
			g_main_loop_quit(mainloop);
	}
}

static void agent_released(Agent *agent, gpointer data)
{
	g_assert(data != NULL);
	GMainLoop *mainloop = data;

	need_unregister = FALSE;

	if (g_main_loop_is_running(mainloop))
		g_main_loop_quit(mainloop);

	g_print("Agent released\n");
}

static gchar *adapter_arg = NULL;
static gchar *capability_arg = NULL;
static gboolean daemon_arg = FALSE;

static GOptionEntry entries[] = {
	{"adapter", 'a', 0, G_OPTION_ARG_STRING, &adapter_arg, "Adapter Name or MAC", "<name|mac>"},
	{"capability", 'c', 0, G_OPTION_ARG_STRING, &capability_arg, "Agent capability", "<capability>"},
	{"pin", 'p', 0, G_OPTION_ARG_STRING, &pin_arg, "Path to the PIN's file"},
	{"daemon", 'd', 0, G_OPTION_ARG_NONE, &daemon_arg, "Run in background (as daemon)"},
	{NULL}
};

int main(int argc, char *argv[])
{
	GError *error = NULL;
	GOptionContext *context;

	/* Query current locale */
	setlocale(LC_CTYPE, "");

	g_type_init();
	dbus_init();

	context = g_option_context_new(" - a bluetooth agent");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_summary(context, "Version "PACKAGE_VERSION);
	g_option_context_set_description(context,
			"`capability` can be one of:\n"
			"   DisplayOnly\n"
			"   DisplayYesNo (default)\n"
			"   KeyboardOnly\n"
			"   NoInputNoOutput\n\n"
			//"Report bugs to <"PACKAGE_BUGREPORT">."
			"Project home page <"PACKAGE_URL">."
			);

	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		g_print("%s: %s\n", g_get_prgname(), error->message);
		g_print("Try `%s --help` for more information.\n", g_get_prgname());
		exit(EXIT_FAILURE);
	}

	if (capability_arg) {
		if (
				g_strcmp0(capability_arg, "DisplayOnly") != 0 &&
				g_strcmp0(capability_arg, "DisplayYesNo") != 0 &&
				g_strcmp0(capability_arg, "KeyboardOnly") != 0 &&
				g_strcmp0(capability_arg, "NoInputNoOutput") != 0
				) {
			g_print("%s: Invalid capability: %s\n", g_get_prgname(), capability_arg);
			g_print("Try `%s --help` for more information.\n", g_get_prgname());
			exit(EXIT_FAILURE);
		}
	} else {
		capability_arg = "DisplayYesNo"; // default value
	}

	g_option_context_free(context);

	if (!dbus_system_connect(&error)) {
		g_printerr("Couldn't connect to DBus system bus: %s\n", error->message);
		exit(EXIT_FAILURE);
	}

	/* Check, that bluetooth daemon is running */
	if (!intf_supported(BLUEZ_DBUS_NAME, MANAGER_DBUS_PATH, MANAGER_DBUS_INTERFACE)) {
		g_printerr("%s: bluez service is not found\n", g_get_prgname());
		g_printerr("Did you forget to run bluetoothd?\n");
		exit(EXIT_FAILURE);
	}

	/* Read PIN's file */
	if (pin_arg) {
		pin_hash_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
		read_pin_file(pin_arg, pin_hash_table, TRUE);
	}

	mainloop = g_main_loop_new(NULL, FALSE);

	Manager *manager = g_object_new(MANAGER_TYPE, NULL);

	Adapter *adapter = find_adapter(adapter_arg, &error);
	exit_if_error(error);

	Agent *agent = g_object_new(AGENT_TYPE, "PinHashTable", pin_hash_table, "Interactive", !daemon_arg, NULL);

	adapter_register_agent(adapter, AGENT_DBUS_PATH, capability_arg, &error);
	exit_if_error(error);
	g_print("Agent registered\n");

	g_signal_connect(agent, "AgentReleased", G_CALLBACK(agent_released), mainloop);

	if (daemon_arg) {
		pid_t pid, sid;

		/* Fork off the parent process */
		pid = fork();
		if (pid < 0)
			exit(EXIT_FAILURE);
		/* Ok, terminate parent proccess */
		if (pid > 0)
			exit(EXIT_SUCCESS);

		/* Create a new SID for the child process */
		sid = setsid();
		if (sid < 0)
			exit(EXIT_FAILURE);

		/* Close out the standard file descriptors */
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
	}

	/* Add SIGTERM/SIGINT/SIGUSR1 handlers */
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signal_handler;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);

	g_main_loop_run(mainloop);

	if (need_unregister) {
		adapter_unregister_agent(adapter, AGENT_DBUS_PATH, &error);
		exit_if_error(error);

		/* Waiting for AgentReleased signal */
		g_main_loop_run(mainloop);
	}

	g_main_loop_unref(mainloop);

	g_object_unref(agent);
	g_object_unref(adapter);
	g_object_unref(manager);

	dbus_disconnect();

	exit(EXIT_SUCCESS);
}

