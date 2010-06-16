#!/usr/bin/perl -w

use strict;
use Data::Dumper;

die "no input file\n" unless $ARGV[0];

sub parse_doc_api {
    my ($doc_api_file, $hierarchy_id) = @_;
    
    $hierarchy_id = 1 unless defined $hierarchy_id && $hierarchy_id > 0;
    
    my %data;
    
    open INPUT, "<$doc_api_file" || die "Can't open '$doc_api_file': $!\n";
    my $n = 0;
    my $hierarchy_n = 0;
    my $section = '';
    while(<INPUT>) {
        $n++;
        chomp;
        
        die "invalid file format (1)\n" if $n == 1 && $_ !~ /^BlueZ D-Bus \S+ API description$/;
        
        /^\s*$/ && next;
        
        # On-fly patches:
        # audio-api.txt
        s/void (PropertyChanged\(string name, variant value\))$/$1/;
        s/(string State)$/$1 [readonly]/;
        s/void (AnswerRequested\(\))/$1/;
        s/^p(roperties\tstring State \[readonly\])$/P$1/;
        s/ ( \[(readonly|readwrite)\])$/$1/;
        s/void (Ring\(string number\))$/$1/;
        s/void (CallTerminated\(\))$/$1/;
        s/void (CallStarted\(\))$/$1/;
        s/void (CallEnded\(\))$/$1/;
        s/^p(roperties\tboolean Connected \[readonly\])$/P$1/;
        
        if (/^(.+) hierarchy$/) {
            my $hierarchy = $1;
            $section = 'hierarchy';
            $hierarchy_n++;
            undef %data if $hierarchy_n == $hierarchy_id;
            last if $hierarchy_n == $hierarchy_id+1;
            $data{'hierarchy'} = $hierarchy;
        } elsif (/^Service\s*(.+)$/) {
            my $service = $1;
            die "invalid file format (2)\n" unless $section eq 'hierarchy';
            die "invalid service: $service\n" unless $service eq 'org.bluez';
            $data{'service'} = $service;
        } elsif (/^Interface\s*(.+)$/) {
            my $intf = $1;
            die "invalid file format (3)\n" unless $section eq 'hierarchy';
            die "invalid interface: $intf\n" unless $intf =~ /^org\.bluez/;
            $data{'intf'} = $intf;
            
            $data{$intf} = undef;
            $data{$intf}{'methods'} = undef;
            $data{$intf}{'signals'} = undef;
            $data{$intf}{'properties'} = undef;
        } elsif (/^Object path\s*(.+)/) {
            my $obj_path = $1;
            die "invalid file format (4)\n" unless $section eq 'hierarchy';
            $data{'objectPath'} = $obj_path if $obj_path =~ /^[A-Za-z0-9\/]+$/;
        } elsif (/^Methods/) {
            die "invalid file format (5)\n" unless $section eq 'hierarchy';
            $section = 'methods';
            s/Methods/       /;
        } elsif (/^Signals/) {
            die "invalid file format (6)\n" unless $section eq 'methods';
            $section = 'signals';
            s/Signals/       /;
        } elsif(/^Properties/) {
            die "invalid file format (7)\n" unless $section eq 'signals' || $section eq 'methods';
            $section = 'properties';
            s/Properties/          /;
        }
        
        if (defined $section && $section eq 'methods' && (/^\s+((\S+) (\w+)\((.*)\))$/ || /^\s+((\S+) (\w+)\((.*,))$/)) {
            my $decl = $1;
            my $ret = $2;
            my $name = $3;
            my $args = $4;

            if ($decl =~ /,$/) {
                my $add_str = <INPUT>;
                if ($add_str =~ /\s+(.+)\)$/) {
                    $decl .= " $1)";
                    $args .= " $1";
                }  else {
                    die "invalid file format (8)\n";
                }
            }

            $data{$data{'intf'}}{'methods'}{$name}{'decl'} = $decl;
            $data{$data{'intf'}}{'methods'}{$name}{'ret'} = $ret;
            @{$data{$data{'intf'}}{'methods'}{$name}{'args'}} = map {type => (split / /, $_)[0], name => (split / /, $_)[1]}, (split /, /, $args);
        } elsif (defined $section && $section eq 'signals' && (/^\s+((\w+)\((.*)\))$/ || /^\s+((\w+)\((.*,))$/)) {
            my $decl = $1;
            my $name = $2;
            my $args = $3;
            
            if ($decl =~ /,$/) {
                my $add_str = <INPUT>;
                if ($add_str =~ /\s+(.+)\)$/) {
                    $decl .= " $1)";
                    $args .= " $1";
                }  else {
                    die "invalid file format (9)\n";
                }
            }

            $data{$data{'intf'}}{'signals'}{$name}{'decl'} = $decl;
            @{$data{$data{'intf'}}{'signals'}{$name}{'args'}} = map {type => (split / /, $_)[0], name => (split / /, $_)[1]}, (split /, /, $args);
        } elsif (defined $section && $section eq 'properties' && /^\s+((\S+) (\w+) \[(readonly|readwrite)\])$/) {
            my $decl = $1;
            my $type = $2;
            my $name = $3;
            my $mode = $4;
            
            $data{$data{'intf'}}{'properties'}{$name}{'decl'} = $decl;
            $data{$data{'intf'}}{'properties'}{$name}{'type'} = $type;
            $data{$data{'intf'}}{'properties'}{$name}{'mode'} = $mode;
        }
    }

    return \%data;
}

my $data = parse_doc_api($ARGV[0], $ARGV[1]);

my $HEADER = <<EOH;
/*
 *
 *  bluez-tools - a set of tools to manage bluetooth devices for linux
 *
 *  Copyright (C) 2010  Alexander Orlenko <zxteam\@gmail.com>
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
EOH

sub get_g_type {
    my $bluez_type = shift;
    my $g_type;
    
    $g_type = 'void ' if $bluez_type eq 'void';
    $g_type = 'gchar *' if $bluez_type eq 'object' || $bluez_type eq 'string';
    $g_type = 'GHashTable *' if $bluez_type eq 'dict';
    $g_type = 'GValue *' if $bluez_type eq 'variant';
    $g_type = 'guint8 ' if $bluez_type eq 'uint8';
    $g_type = 'gboolean ' if $bluez_type eq 'boolean';
    $g_type = 'gint32' if $bluez_type eq 'int32';
    
    die "unknown bluez type (1): $bluez_type\n" unless defined $g_type;
    
    return $g_type;
}

sub get_g_type_name {
    my $bluez_type = shift;
    my $g_type_name;
    
    $g_type_name = 'DBUS_TYPE_G_OBJECT_PATH' if $bluez_type eq 'object';
    $g_type_name = 'G_TYPE_STRING' if $bluez_type eq 'string';
    $g_type_name = 'G_TYPE_VALUE' if $bluez_type eq 'variant';
    $g_type_name = 'DBUS_TYPE_G_STRING_VARIANT_HASHTABLE' if $bluez_type eq 'dict';
    
    die "unknown bluez type (2): $bluez_type\n" unless defined $g_type_name;
    
    return $g_type_name;
}

sub generate_header {
    my $node = shift;

    my $HEADER_TEMPLATE = <<EOT;
#ifndef __{\$OBJECT}_H
#define __{\$OBJECT}_H

#include <glib-object.h>

/*
 * Type macros
 */
#define {\$OBJECT}_TYPE				({\$object}_get_type())
#define {\$OBJECT}(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), {\$OBJECT}_TYPE, {\$Object}))
#define {\$OBJECT}_IS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), {\$OBJECT}_TYPE))
#define {\$OBJECT}_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), {\$OBJECT}_TYPE, {\$Object}Class))
#define {\$OBJECT}_IS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), {\$OBJECT}_TYPE))
#define {\$OBJECT}_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), {\$OBJECT}_TYPE, {\$Object}Class))

typedef struct _{\$Object} {\$Object};
typedef struct _{\$Object}Class {\$Object}Class;
typedef struct _{\$Object}Private {\$Object}Private;

struct _{\$Object} {
	GObject parent_instance;

	/*< private >*/
	{\$Object}Private *priv;
};

struct _{\$Object}Class {
	GObjectClass parent_class;
};

/*
 * Method definitions
 */
{METHOD_DEFS}

#endif /* __{\$OBJECT}_H */
EOT
    
    my $intf = $node->{'intf'};
    my $obj = (split /\./, $intf)[-1];
    my $obj_lc = lc $obj;
    my $obj_uc = uc $obj;
    
    my $method_defs = "";
    for my $method (sort keys %{$node->{$intf}{'methods'}}) {
        my @a = $method =~ /([A-Z]+[a-z]*)/g;
	my %m = %{$node->{$intf}{'methods'}{$method}};
        
	my $in_args = join ', const', (map get_g_type($_->{'type'}).$_->{'name'}, @{$m{'args'}});
        my $method_def =
	get_g_type($m{'ret'}) . "${obj_lc}_" . (join '_', (map lc $_, @a)) . "($obj *self" .
        ($in_args eq "" ? "" : ", const $in_args") . ", GError **error = NULL);";
	
        $method_defs .= "$method_def\n";
    }
    chomp $method_defs;
    
    my $output = $HEADER . "\n" . $HEADER_TEMPLATE;
    $output =~ s/{METHOD_DEFS}/$method_defs/;
    $output =~ s/{\$OBJECT}/$obj_uc/g;
    $output =~ s/{\$Object}/$obj/g;
    $output =~ s/{\$object}/$obj_lc/g;
    $output .= "\n";
    
    return $output;
}

sub generate_source {
    my $node = shift;
    
    my $SOURCE_TEMPLATE = <<EOT;
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "dbus-common.h"
#include "marshallers.h"
#include "{\$object}.h"

{BLUEZ_DBUS_OBJECT_DEFS}

#define {\$OBJECT}_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), {\$OBJECT}_TYPE, {\$Object}Private))

struct _{\$Object}Private {
	DBusGProxy *dbus_g_proxy;
};

G_DEFINE_TYPE({\$Object}, {\$object}, G_TYPE_OBJECT);

enum {
	PROP_0,

	{ENUM_PROPERTIES}
};

{IF_SIGNALS}
enum {
	{ENUM_SIGNALS},

	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};
{FI_SIGNALS}

static void {\$object}_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void {\$object}_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

{IF_SIGNALS}
{SIGNALS_HANDLERS_DEF}
{FI_SIGNALS}

static void {\$object}_class_init({\$Object}Class *klass)
{
	g_type_class_add_private(klass, sizeof({\$Object}Private));

	/* Properties registration */
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GParamSpec *pspec;

	gobject_class->get_property = {\$object}_get_property;
	gobject_class->set_property = {\$object}_set_property;

	{PROPERTIES_REGISTRATION}

        {IF_SIGNALS}
	/* Signals registation */
        {SIGNALS_REGISTRATION}
        {FI_SIGNALS}
}

static void {\$object}_init({\$Object} *self)
{
	self->priv = MANAGER_GET_PRIVATE(self);

	g_assert(conn != NULL);

	{IF_INIT}
	self->priv->dbus_g_proxy = dbus_g_proxy_new_for_name(conn, BLUEZ_DBUS_NAME, BLUEZ_DBUS_{\$OBJECT}_PATH, BLUEZ_DBUS_{\$OBJECT}_INTERFACE);

	g_assert(self->priv->dbus_g_proxy != NULL);

	{IF_SIGNALS}
        /* DBUS signals connection */

	{SIGNALS_CONNECTION}
        {FI_SIGNALS}
	{FI_INIT}
}

{IF_POST_INIT}
static void {\$object}_post_init({\$Object} *self)
{
	g_assert(self->priv->dbus_g_proxy != NULL);

        {IF_SIGNALS}
	/* DBUS signals connection */

	{SIGNALS_CONNECTION}
        {FI_SIGNALS}
}
{FI_POST_INIT}

static void {\$object}_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	{\$Object} *self = {\$OBJECT}(object);

	GHashTable *properties = {\$object}_get_properties(self, NULL);
	if (properties == NULL) {
	    return;
	}

	switch (property_id) {
	{GET_PROPERTIES}

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}

	g_hash_table_unref(properties);
}

static void {\$object}_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	{\$Object} *self = {\$OBJECT}(object);

	switch (property_id) {
	{SET_PROPERTIES}
	
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/* Methods */
{METHODS}

{IF_SIGNALS}
/* Signals handlers */
{SIGNALS_HANDLERS}
{FI_SIGNALS}
EOT

    my $intf = $node->{'intf'};
    my $obj = (split /\./, $intf)[-1];
    my $obj_lc = lc $obj;
    my $obj_uc = uc $obj;

    my $bluez_dbus_object_defs = "";
    $bluez_dbus_object_defs .= "#define BLUEZ_DBUS_{\$OBJECT}_PATH \"". $node->{'objectPath'} . "\"\n"  if defined $node->{'objectPath'};
    $bluez_dbus_object_defs .= "#define BLUEZ_DBUS_{\$OBJECT}_INTERFACE \"" . $node->{'intf'} . "\"";

    my $enum_signals = "";
    my $signals_handlers_def = "";
    my $signals_registration = "";
    my $signals_connection = "";
    my $signals_handlers = "";
    for my $signal (sort keys %{$node->{$intf}{'signals'}}) {
        my @a = $signal =~ /([A-Z]+[a-z]*)/g;
        my %s = %{$node->{$intf}{'signals'}{$signal}};
        
        my $enum = join '_', (map uc $_, @a);
        my $handler_name = (join '_', (map lc $_, @a)) . "_handler";
        my $handler = "static void $handler_name(DBusGProxy *dbus_g_proxy, ";
        for my $arg (@{$s{'args'}}) {
            $handler .= "const " . get_g_type($arg->{'type'}) . $arg->{'name'} . ", ";
        }
        $handler .= "gpointer data)";
        
        $signals_registration .=
        "\tsignals[$enum] = g_signal_new(\"$signal\",\n" .
        "\t\t\tG_TYPE_FROM_CLASS(gobject_class),\n" .
        "\t\t\tG_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,\n" .
        "\t\t\t0, NULL, NULL,\n";
        my $arg_t = join '_', map($_->{'type'}, @{$s{'args'}});
        if ($arg_t eq '') {
            $signals_registration .=
            "\t\t\tg_cclosure_marshal_VOID__VOID,\n".
            "\t\t\tG_TYPE_NONE, 0);\n\n";
        } elsif ($arg_t eq 'object' || $arg_t eq 'string') {
            $signals_registration .=
            "\t\t\tg_cclosure_marshal_VOID__STRING,\n".
            "\t\t\tG_TYPE_NONE, 1, G_TYPE_STRING);\n\n";
        } elsif ($arg_t eq 'string_variant' || $arg_t eq 'string_dict') {
            $signals_registration .=
            "\t\t\tg_cclosure_bluez_marshal_VOID__STRING_BOXED,\n".
            "\t\t\tG_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_VALUE);\n\n";
        #} elsif ($arg_t eq 'uint8_boolean_int32_string') { # control api skipped
        #    $signals_registration .=
        #    "\t\t\tg_cclosure_bluez_marshal_VOID__STRING_BOXED,\n".
        #    "\t\t\tG_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_VALUE);\n\n";
        } else {
            die "unknown arguments: $arg_t\n";
        }
        
        $signals_connection .=
        "\t/* " . $s{'decl'} . " */\n" .
        "\tdbus_g_proxy_add_signal(self->priv->dbus_g_proxy, \"$signal\", " . (join ', ', (map get_g_type_name($_->{'type'}), @{$s{'args'}})) . ", G_TYPE_INVALID);\n" .
        "\tdbus_g_proxy_connect_signal(self->priv->dbus_g_proxy, \"$signal\", G_CALLBACK($handler_name), self, NULL);\n\n";
        
        my $args = join ', ', map($_->{'name'}, @{$s{'args'}});
        $signals_handlers .= "$handler\n" .
        "{\n" .
        "\t{\$Object} *self = {\$OBJECT}(data);\n" .
        "\tg_signal_emit(self, signals[$enum], 0" . ($args eq '' ? '' : ", $args") . ");\n" .
        "}\n\n";
        
        $enum_signals .= "\t$enum,\n";
        $signals_handlers_def .= "$handler;\n";
    }
    $enum_signals =~ s/^\t(.+?),\s+$/$1/s;
    $signals_handlers_def =~ s/\s+$//s;
    $signals_registration =~ s/^\t(.+?)\s+$/$1/s;
    $signals_connection =~ s/^\t(.+?)\s+$/$1/s;
    $signals_handlers =~ s/\s+$//s;
    
    my $enum_properties = "";
    my $properties_registration = "";
    my $get_properties = "";
    my $set_properties = "";
    unless (defined $node->{'objectPath'}) {
        $enum_properties .= "\tPROP_DBUS_OBJECT_PATH, /* readwrite, construct only */\n";
        $properties_registration .=
        "\t/* object DBusObjectPath [readwrite, construct only] */\n" .
        "\tpspec = g_param_spec_string(\"DBusObjectPath\", \"dbus_object_path\", \"Adapter D-Bus object path\", NULL, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);\n" .
        "\tg_object_class_install_property(gobject_class, PROP_DBUS_OBJECT_PATH, pspec);\n\n";
        
        $get_properties .=
        "\tcase PROP_DBUS_OBJECT_PATH:\n" .
        "\t\tg_value_set_string(value, g_strdup(dbus_g_proxy_get_path(self->priv->dbus_g_proxy)));\n" .
        "\t\tbreak;\n\n";
        
        $set_properties .=
        "\tcase PROP_DBUS_OBJECT_PATH:\n" .
        "\t{\n" .
        "\t\tconst gchar *dbus_object_path = g_value_get_string(value);\n" .
        "\t\tg_assert(dbus_object_path != NULL);\n" .
        "\t\tg_assert(self->priv->dbus_g_proxy == NULL);\n" .
        "\t\tself->priv->dbus_g_proxy = dbus_g_proxy_new_for_name(conn, BLUEZ_DBUS_NAME, dbus_object_path, BLUEZ_DBUS_{\$OBJECT}_INTERFACE);\n" .
        "\t\t{\$object}_post_init(self);\n" .
        "\t}\n" .
        "\t\tbreak;\n\n";
    }
    for my $property (sort keys %{$node->{$intf}{'properties'}}) {
        my @a = $property =~ /([A-Z]+[a-z]*)/g;
        my %p = %{$node->{$intf}{'properties'}{$property}};
        
        my $enum = "PROP_" . (join '_', (map uc $_, @a));
        
        $enum_properties .= "\t$enum, /* $p{'mode'} */\n";
        $properties_registration .= "\t/* $p{'decl'} */\n";
        if ($p{'type'} eq 'string') {
            $properties_registration .= "\tpspec = g_param_spec_string(\"$property\", NULL, NULL, NULL, " . ($p{'mode'} eq 'readonly' ? 'G_PARAM_READABLE' : 'G_PARAM_READWRITE') . ");\n";
        } elsif ($p{'type'} eq 'array{object}' || $p{'type'} eq 'array{string}') {
            $properties_registration .= "\tpspec = g_param_spec_boxed(\"$property\", NULL, NULL, G_TYPE_PTR_ARRAY, " . ($p{'mode'} eq 'readonly' ? 'G_PARAM_READABLE' : 'G_PARAM_READWRITE') . ");\n";
        } elsif ($p{'type'} eq 'uint32') {
            $properties_registration .= "\tpspec = g_param_spec_uint(\"$property\", NULL, NULL, 0, 4294967295, 0, " . ($p{'mode'} eq 'readonly' ? 'G_PARAM_READABLE' : 'G_PARAM_READWRITE') . ");\n";
        } elsif ($p{'type'} eq 'boolean') {
            $properties_registration .= "\tpspec = g_param_spec_boolean(\"$property\", NULL, NULL, FALSE, " . ($p{'mode'} eq 'readonly' ? 'G_PARAM_READABLE' : 'G_PARAM_READWRITE') . ");\n";
        } else {
            die "unknown type: " . $p{'type'} . "\n";
        }
        $properties_registration .= "\tg_object_class_install_property(gobject_class, $enum, pspec);\n\n";
        
        if ($p{'type'} eq 'array{object}') {
            $get_properties .=
            "\tcase $enum:\n" .
            "\t\tg_value_set_boxed(value, g_value_dup_boxed(g_hash_table_lookup(properties, \"$property\")));\n" .
            "\t\tbreak;\n\n";
	    
	    if ($p{'mode'} eq 'readwrite') {
		$set_properties .=
		"case $enum:\n" .
		"\t\tNONE;\n" .
		"\t\tbreak;\n\n"
	    }
        } else {
            die "unknown type: $p{'type'}\n";
        }
    }
    $enum_properties =~ s/^\t(.+?), (\/\* .+? \*\/)\s+$/$1 $2/s;
    $properties_registration =~ s/^\t(.+?)\s+$/$1/s;
    $get_properties =~ s/^\t(.+?)\s+$/$1/s;
    $set_properties =~ s/^\t(.+?)\s+$/$1/s;
    
    my $output = $HEADER . "\n" . $SOURCE_TEMPLATE;
    if (defined $node->{'objectPath'}) {
        $output =~ s/\{IF_INIT\}\s+(.+?)\s+\{FI_INIT\}/$1/gs;
        $output =~ s/\s+\{IF_POST_INIT\}.+?\{FI_POST_INIT\}\s+/\n\n/gs;
    } else {
        $output =~ s/\{IF_POST_INIT\}\s+(.+?)\s+\{FI_POST_INIT\}/$1/gs;
        $output =~ s/\s+\{IF_INIT\}.+?\{FI_INIT\}\s+/\n/gs;
    }
    if (scalar keys %{$node->{$intf}{'signals'}} > 0) {
        $output =~ s/\{IF_SIGNALS\}\s+(.+?)\s+\{FI_SIGNALS\}/$1/gs;
    } else {
        $output =~ s/\s+\{IF_SIGNALS\}.+?\{FI_SIGNALS\}\s+/\n\n/gs;
    }
    $output =~ s/{BLUEZ_DBUS_OBJECT_DEFS}/$bluez_dbus_object_defs/;
    $output =~ s/{ENUM_SIGNALS}/$enum_signals/;
    $output =~ s/{SIGNALS_HANDLERS_DEF}/$signals_handlers_def/;
    $output =~ s/{SIGNALS_REGISTRATION}/$signals_registration/;
    $output =~ s/{SIGNALS_CONNECTION}/$signals_connection/;
    $output =~ s/{SIGNALS_HANDLERS}/$signals_handlers/;
    $output =~ s/{ENUM_PROPERTIES}/$enum_properties/;
    $output =~ s/{PROPERTIES_REGISTRATION}/$properties_registration/;
    $output =~ s/{GET_PROPERTIES}/$get_properties/;
    $output =~ s/{SET_PROPERTIES}/$set_properties/;
    $output =~ s/{\$OBJECT}/$obj_uc/g;
    $output =~ s/{\$Object}/$obj/g;
    $output =~ s/{\$object}/$obj_lc/g;
    
    # Some formatting fixes
    $output =~ s/\s+?(\t*\})/\n$1/g;
    $output =~ s/(switch \(\w+\) \{\n)\s+?(\t+default:)/$1$2/s;
    $output =~ s/\s+$/\n\n/;
    
    return $output;
}

#print Dumper($data);
print generate_source($data);
