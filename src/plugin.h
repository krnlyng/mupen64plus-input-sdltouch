/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-input-sdltouch - plugin.h                                 *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2014 Franz-Josef Anton Friedrich Haider                 *
 *   Copyright (C) 2008-2009 Richard Goedeken                              *
 *   Copyright (C) 2008 Tillin9                                            *
 *   Copyright (C) 2002 Blight                                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include <SDL.h>
#include <SDL_opengles2.h>

#include <string>

#define M64P_PLUGIN_PROTOTYPES 1
#include "m64p_plugin.h"
#include "m64p_config.h"

// Some stuff from n-rage plugin
#define RD_GETSTATUS        0x00        // get status
#define RD_READKEYS         0x01        // read button values
#define RD_READPAK          0x02        // read from controllerpack
#define RD_WRITEPAK         0x03        // write to controllerpack
#define RD_RESETCONTROLLER  0xff        // reset controller
#define RD_READEEPROM       0x04        // read eeprom
#define RD_WRITEEPROM       0x05        // write eeprom

#define PAK_IO_RUMBLE       0xC000      // the address where rumble-commands are sent to

/* global function definitions */
extern void DebugMessage(int level, const char *message, ...);

/* declarations of pointers to Core config functions */
extern ptr_ConfigListSections     ConfigListSections;
extern ptr_ConfigOpenSection      ConfigOpenSection;
extern ptr_ConfigDeleteSection    ConfigDeleteSection;
extern ptr_ConfigListParameters   ConfigListParameters;
extern ptr_ConfigSaveFile         ConfigSaveFile;
extern ptr_ConfigSaveSection      ConfigSaveSection;
extern ptr_ConfigSetParameter     ConfigSetParameter;
extern ptr_ConfigGetParameter     ConfigGetParameter;
extern ptr_ConfigGetParameterHelp ConfigGetParameterHelp;
extern ptr_ConfigSetDefaultInt    ConfigSetDefaultInt;
extern ptr_ConfigSetDefaultFloat  ConfigSetDefaultFloat;
extern ptr_ConfigSetDefaultBool   ConfigSetDefaultBool;
extern ptr_ConfigSetDefaultString ConfigSetDefaultString;
extern ptr_ConfigGetParamInt      ConfigGetParamInt;
extern ptr_ConfigGetParamFloat    ConfigGetParamFloat;
extern ptr_ConfigGetParamBool     ConfigGetParamBool;
extern ptr_ConfigGetParamString   ConfigGetParamString;

extern ptr_ConfigGetSharedDataFilepath ConfigGetSharedDataFilepath;
extern ptr_ConfigGetUserConfigPath     ConfigGetUserConfigPath;
extern ptr_ConfigGetUserDataPath       ConfigGetUserDataPath;
extern ptr_ConfigGetUserCachePath      ConfigGetUserCachePath;

struct _color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};

struct _pos {
    unsigned int x;
    unsigned int y;
};

struct _slot_data {
    unsigned int x;
    unsigned int y;
    bool pressed;
    bool released;
};

enum _ba_id {
    BUTTON_A=0,
    BUTTON_B,
    BUTTON_L,
    BUTTON_R,
    BUTTON_Z,
    BUTTON_START,
    BUTTON_HB,
    BUTTON_CRIGHT,
    BUTTON_CDOWN,
    BUTTON_CLEFT,
    BUTTON_CUP,
    BUTTON_DRIGHT,
    BUTTON_DDOWN,
    BUTTON_DLEFT,
    BUTTON_DUP,
    JOYSTICK_AXIS,
    NUM_BUTTONS_AND_AXES
};

struct _bainfo {
    _ba_id id;
    std::string name;
    std::string nice_name;
    struct _pos pos;
    float radius;
    struct _color col;
    bool pressed;
};

#define BUTTON_POLYGON_SIZE 8

struct _ba_drawdata {
    GLuint vbo;
    GLuint cbo, cabo;
    float vertices[2 * BUTTON_POLYGON_SIZE];
    float colors[4 * BUTTON_POLYGON_SIZE];
    float colors_active[4 * BUTTON_POLYGON_SIZE];
};

#endif // __PLUGIN_H__

