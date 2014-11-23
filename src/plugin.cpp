/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-input-sdltouch - plugin.cpp                               *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2014 Franz-Josef Anton Friedrich Haider                 *
 *   Copyright (C) 2008-2011 Richard Goedeken                              *
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

#include <SDL.h>

#define M64P_PLUGIN_PROTOTYPES 1
#include "m64p_types.h"
#include "m64p_plugin.h"
#include "m64p_common.h"
#include "m64p_config.h"

#include "plugin.h"
#include "version.h"
#include "osal_dynamiclib.h"

#include <string>
#include <vector>
#include <list>
#include <map>
#include <sstream>
#include <iomanip>
#include <cmath>

/* definitions of pointers to Core config functions */
ptr_ConfigOpenSection      ConfigOpenSection = NULL;
ptr_ConfigDeleteSection    ConfigDeleteSection = NULL;
ptr_ConfigSaveSection      ConfigSaveSection = NULL;
ptr_ConfigListParameters   ConfigListParameters = NULL;
ptr_ConfigSaveFile         ConfigSaveFile = NULL;
ptr_ConfigSetParameter     ConfigSetParameter = NULL;
ptr_ConfigGetParameter     ConfigGetParameter = NULL;
ptr_ConfigGetParameterHelp ConfigGetParameterHelp = NULL;
ptr_ConfigSetDefaultInt    ConfigSetDefaultInt = NULL;
ptr_ConfigSetDefaultFloat  ConfigSetDefaultFloat = NULL;
ptr_ConfigSetDefaultBool   ConfigSetDefaultBool = NULL;
ptr_ConfigSetDefaultString ConfigSetDefaultString = NULL;
ptr_ConfigGetParamInt      ConfigGetParamInt = NULL;
ptr_ConfigGetParamFloat    ConfigGetParamFloat = NULL;
ptr_ConfigGetParamBool     ConfigGetParamBool = NULL;
ptr_ConfigGetParamString   ConfigGetParamString = NULL;

ptr_ConfigGetSharedDataFilepath ConfigGetSharedDataFilepath = NULL;
ptr_ConfigGetUserConfigPath     ConfigGetUserConfigPath = NULL;
ptr_ConfigGetUserDataPath       ConfigGetUserDataPath = NULL;
ptr_ConfigGetUserCachePath      ConfigGetUserCachePath = NULL;

/* static data definitions */
static void (*l_DebugCallback)(void *, int, const char *) = NULL;
static void *l_DebugCallContext = NULL;
static int l_PluginInit = 0;

static m64p_handle input_section;
static m64p_handle video_general_section;

static int screen_width = 0, screen_height = 0;
static float screen_widthf;
static float screen_heightf;
static float aspect_ratio;

static float corr_factor_x;
static float corr_factor_y;

static bool render_joystick = true;
static bool render_dpad = true;
static bool render_ba = true;
static bool hide_buttons = false;
static bool gl_initialized = false;

static GLuint buttons_program;
static GLuint projection_matrix_location;
static GLuint rotation_matrix_location;
static GLfloat projection_matrix[16];

#define POSITION_ATTR 5
#define COLOR_ATTR 6

static int rotate;

static std::map<unsigned int, struct _slot_data> input_slots;
static unsigned int fingerIds[NUM_BUTTONS_AND_AXES];

// default positioning and coloring of the buttons and axes
// default positions are only placeholders and will be overwriten (adjusted to
// the screen, see generate_default_positions)
static std::vector<struct _bainfo> buttons_and_axes {
    {      BUTTON_A,     "BUTTON_A",            "Button A", {804, 430},   7.071f, {  0,   0, 102, 128}, false},
    {      BUTTON_B,     "BUTTON_B",            "Button B", {705, 330},   7.071f, {  0, 102,   0, 128}, false},
    {      BUTTON_L,     "BUTTON_L",            "Button L", { 50,  50},   7.071f, {128, 128, 128, 128}, false},
    {      BUTTON_R,     "BUTTON_R",            "Button R", {804, 230},   7.071f, {128, 128, 128, 128}, false},
    {      BUTTON_Z,     "BUTTON_Z",            "Button Z", {705, 230},   7.071f, {128, 128, 128, 128}, false},
    {  BUTTON_START, "BUTTON_START",        "Button Start", {427, 240},      3.f, {255,   0,   0,  64}, false},
    {     BUTTON_HB,    "BUTTON_HB", "Button hide buttons", {130,  50},   7.071f, { 17,  17,  17, 128}, false},
    {BUTTON_CRIGHT, "BUTTON_CRIGHT",      "Button C-right", {755,  60},      4.f, {204, 255,   0, 128}, false},
    {  BUTTON_CDOWN, "BUTTON_CDOWN",       "Button C-down", {698,  98},      4.f, {204, 255,   0, 128}, false},
    {  BUTTON_CLEFT, "BUTTON_CLEFT",       "Button C-left", {636,  60},      4.f, {204, 255,   0, 128}, false},
    {    BUTTON_CUP,   "BUTTON_CUP",         "Button C-up", {698,  40},      4.f, {204, 255,   0, 128}, false},
    { BUTTON_DRIGHT,"BUTTON_DRIGHT",      "Button D-right", {223, 330},   7.071f, {128, 128, 128, 128}, false},
    {  BUTTON_DDOWN, "BUTTON_DDOWN",       "Button D-down", {124, 430},   7.071f, {128, 128, 128, 128}, false},
    {  BUTTON_DLEFT, "BUTTON_DLEFT",       "Button D-left", { 50, 330},   7.071f, {128, 128, 128, 128}, false},
    {    BUTTON_DUP,   "BUTTON_DUP",         "Button D-up", {124, 230},   7.071f, {128, 128, 128, 128}, false},
    { JOYSTICK_AXIS,"JOYSTICK_AXIS",       "Joystick axis", {124, 355}, 10.6065f, {200, 200, 200, 128}, false}
};

static std::map<_ba_id, struct _ba_drawdata> draw_info {
    {     BUTTON_A, {0, 0, 0, {0}, {0}, {0}}},
    {     BUTTON_B, {0, 0, 0, {0}, {0}, {0}}},
    {     BUTTON_L, {0, 0, 0, {0}, {0}, {0}}},
    {     BUTTON_R, {0, 0, 0, {0}, {0}, {0}}},
    {     BUTTON_Z, {0, 0, 0, {0}, {0}, {0}}},
    { BUTTON_START, {0, 0, 0, {0}, {0}, {0}}},
    {    BUTTON_HB, {0, 0, 0, {0}, {0}, {0}}},
    {BUTTON_CRIGHT, {0, 0, 0, {0}, {0}, {0}}},
    { BUTTON_CDOWN, {0, 0, 0, {0}, {0}, {0}}},
    { BUTTON_CLEFT, {0, 0, 0, {0}, {0}, {0}}},
    {   BUTTON_CUP, {0, 0, 0, {0}, {0}, {0}}},
    {BUTTON_DRIGHT, {0, 0, 0, {0}, {0}, {0}}},
    { BUTTON_DDOWN, {0, 0, 0, {0}, {0}, {0}}},
    { BUTTON_DLEFT, {0, 0, 0, {0}, {0}, {0}}},
    {   BUTTON_DUP, {0, 0, 0, {0}, {0}, {0}}},
    {JOYSTICK_AXIS, {0, 0, 0, {0}, {0}, {0}}},
};

#define GLSL_VERSION "100"

const char *v_shader_str = 
    "#version " GLSL_VERSION "                                          \n"
    "attribute vec4 a_position;                                         \n"
    "attribute vec4 a_color;                                            \n"
    "varying vec4 v_color;                                              \n"
    "uniform mat4 projection_matrix;                                    \n"
    "uniform mat4 rotation_matrix;                                      \n"
    "void main()                                                        \n"
    "{                                                                  \n"
    "  gl_Position = projection_matrix * rotation_matrix * a_position;  \n"
    "  v_color = a_color;                                               \n"
    "}                                                                  \n";

const char *f_shader_str = 
    "#version " GLSL_VERSION "                                          \n"
    "precision mediump float;                                           \n"
    "varying vec4 v_color;                                              \n"
    "void main()                                                        \n"
    "{                                                                  \n"
    "  gl_FragColor = v_color;                                          \n"
    "}                                                                  \n";

static GLuint vertex_shader;
static GLuint fragment_shader;

static int romopen = 0;

/* Global functions */
void DebugMessage(int level, const char *message, ...)
{
  char msgbuf[1024];
  va_list args;

  if (l_DebugCallback == NULL)
      return;

  va_start(args, message);
  vsprintf(msgbuf, message, args);

  (*l_DebugCallback)(l_DebugCallContext, level, msgbuf);

  va_end(args);
}

static void generate_default_positions()
{
    for(unsigned int i=0;i<buttons_and_axes.size();i++)
    {
        if("BUTTON_A" == buttons_and_axes[i].name)
        {
            buttons_and_axes[i].pos.x = (unsigned int)(0.941f * (float)screen_width);
            buttons_and_axes[i].pos.y = (unsigned int)(0.895f * (float)screen_height);
            buttons_and_axes[i].radius = sqrt(screen_width * screen_height) * 0.075f;
        }
        else if("BUTTON_B" == buttons_and_axes[i].name)
        {
            buttons_and_axes[i].pos.x = (unsigned int)(0.825f * (float)screen_width);
            buttons_and_axes[i].pos.y = (unsigned int)(0.688f * (float)screen_height);
            buttons_and_axes[i].radius = sqrt(screen_width * screen_height) * 0.075f;
        }
        else if("BUTTON_L" == buttons_and_axes[i].name)
        {
            buttons_and_axes[i].pos.x = (unsigned int)(0.059f * (float)screen_width);
            buttons_and_axes[i].pos.y = (unsigned int)(0.104f * (float)screen_height);
            buttons_and_axes[i].radius = sqrt(screen_width * screen_height) * 0.075f;
        }
        else if("BUTTON_R" == buttons_and_axes[i].name)
        {
            buttons_and_axes[i].pos.x = (unsigned int)(0.941f * (float)screen_width);
            buttons_and_axes[i].pos.y = (unsigned int)(0.479f * (float)screen_height);
            buttons_and_axes[i].radius = sqrt(screen_width * screen_height) * 0.075f;
        }
        else if("BUTTON_Z" == buttons_and_axes[i].name)
        {
            buttons_and_axes[i].pos.x = (unsigned int)(0.825f * (float)screen_width);
            buttons_and_axes[i].pos.y = (unsigned int)(0.479f * (float)screen_height);
            buttons_and_axes[i].radius = sqrt(screen_width * screen_height) * 0.075f;
        }
        else if("BUTTON_START" == buttons_and_axes[i].name)
        {
            buttons_and_axes[i].pos.x = (unsigned int)(0.5f * (float)screen_width);
            buttons_and_axes[i].pos.y = (unsigned int)(0.895f * (float)screen_height);
            buttons_and_axes[i].radius = sqrt(screen_width * screen_height) * 0.03375f;
        }
        else if("BUTTON_HB" == buttons_and_axes[i].name)
        {
            buttons_and_axes[i].pos.x = (unsigned int)(0.152f * (float)screen_width);
            buttons_and_axes[i].pos.y = (unsigned int)(0.104f * (float)screen_height);
            buttons_and_axes[i].radius = sqrt(screen_width * screen_height) * 0.075f;
        }
        else if("BUTTON_CRIGHT" == buttons_and_axes[i].name)
        {
            buttons_and_axes[i].pos.x = (unsigned int)(0.884f * (float)screen_width);
            buttons_and_axes[i].pos.y = (unsigned int)(0.125f * (float)screen_height);
            buttons_and_axes[i].radius = sqrt(screen_width * screen_height) * 0.0375f;
        }
        else if("BUTTON_CDOWN" == buttons_and_axes[i].name)
        {
            buttons_and_axes[i].pos.x = (unsigned int)(0.817f * (float)screen_width);
            buttons_and_axes[i].pos.y = (unsigned int)(0.204f * (float)screen_height);
            buttons_and_axes[i].radius = sqrt(screen_width * screen_height) * 0.0375f;
        }
        else if("BUTTON_CLEFT" == buttons_and_axes[i].name)
        {
            buttons_and_axes[i].pos.x = (unsigned int)(0.745f * (float)screen_width);
            buttons_and_axes[i].pos.y = (unsigned int)(0.125f * (float)screen_height);
            buttons_and_axes[i].radius = sqrt(screen_width * screen_height) * 0.0375f;
        }
        else if("BUTTON_CUP" == buttons_and_axes[i].name)
        {
            buttons_and_axes[i].pos.x = (unsigned int)(0.817f * (float)screen_width);
            buttons_and_axes[i].pos.y = (unsigned int)(0.083f * (float)screen_height);
            buttons_and_axes[i].radius = sqrt(screen_width * screen_height) * 0.0375f;
        }
        else if("BUTTON_DRIGHT" == buttons_and_axes[i].name)
        {
            buttons_and_axes[i].pos.x = (unsigned int)(0.231f * (float)screen_width);
            buttons_and_axes[i].pos.y = (unsigned int)(0.739f * (float)screen_height);
            buttons_and_axes[i].radius = sqrt(screen_width * screen_height) * 0.075f;
        }
        else if("BUTTON_DDOWN" == buttons_and_axes[i].name)
        {
            buttons_and_axes[i].pos.x = (unsigned int)(0.145f * (float)screen_width);
            buttons_and_axes[i].pos.y = (unsigned int)(0.895f * (float)screen_height);
            buttons_and_axes[i].radius = sqrt(screen_width * screen_height) * 0.075f;
        }
        else if("BUTTON_DLEFT" == buttons_and_axes[i].name)
        {
            buttons_and_axes[i].pos.x = (unsigned int)(0.059f * (float)screen_width);
            buttons_and_axes[i].pos.y = (unsigned int)(0.739f * (float)screen_height);
            buttons_and_axes[i].radius = sqrt(screen_width * screen_height) * 0.075f;
        }
        else if("BUTTON_DUP" == buttons_and_axes[i].name)
        {
            buttons_and_axes[i].pos.x = (unsigned int)(0.145f * (float)screen_width);
            buttons_and_axes[i].pos.y = (unsigned int)(0.583f * (float)screen_height);
            buttons_and_axes[i].radius = sqrt(screen_width * screen_height) * 0.075f;
        }
        else if("JOYSTICK_AXIS" == buttons_and_axes[i].name)
        {
            buttons_and_axes[i].pos.x = (unsigned int)(0.145f * (float)screen_width);
            buttons_and_axes[i].pos.y = (unsigned int)(0.739f * (float)screen_height);
            buttons_and_axes[i].radius = sqrt(screen_width * screen_height) * 0.1125f;
        }
    }
}

static void set_default_cfg_values()
{
    for(unsigned int i=0;i<buttons_and_axes.size();i++)
    {
        std::string name = buttons_and_axes[i].name;
        std::string nice_name = buttons_and_axes[i].nice_name;
        struct _pos pos = buttons_and_axes[i].pos;
        float radius = buttons_and_axes[i].radius;
        struct _color col = buttons_and_axes[i].col;
        std::stringstream sscolor, ssalpha;

        sscolor << "#";
        sscolor << std::setfill('0') << std::setw(2) << std::hex << (unsigned int)col.r;
        sscolor << std::setfill('0') << std::setw(2) << std::hex << (unsigned int)col.g;
        sscolor << std::setfill('0') << std::setw(2) << std::hex << (unsigned int)col.b;

        ssalpha << std::setfill('0') << std::setw(2) << std::hex << (unsigned int)col.a;

        ConfigSetDefaultInt(input_section, (name + "_X").c_str(), pos.x, (nice_name + " x coordinate").c_str());
        ConfigSetDefaultInt(input_section, (name + "_Y").c_str(), pos.y, (nice_name + " y coordinate").c_str());
        ConfigSetDefaultFloat(input_section, (name + "_RADIUS").c_str(), radius, (nice_name + " radius").c_str());
        ConfigSetDefaultString(input_section, (name + "_COLOR").c_str(), sscolor.str().c_str(), (nice_name + " HTML style color value").c_str());
        ConfigSetDefaultString(input_section, (name + "_A").c_str(), ssalpha.str().c_str(), (nice_name + " alpha value for specifying transparency").c_str());
    }

    ConfigSetDefaultBool(input_section, "render_joystick", true, "render the joystick axis on screen, can be turned of, for example for games which don't use the joystick");
    ConfigSetDefaultBool(input_section, "render_dpad", true, "render the dpad on screen, can be turned of, for example for games which don't use the dpad");
}

static void load_configuration()
{
    for(unsigned int i=0;i<buttons_and_axes.size();i++)
    {
        std::string name = buttons_and_axes[i].name;
        std::string color, alpha;

        buttons_and_axes[i].pos.x = ConfigGetParamInt(input_section, (name + "_X").c_str());
        buttons_and_axes[i].pos.y = ConfigGetParamInt(input_section, (name + "_Y").c_str());
        buttons_and_axes[i].radius = ConfigGetParamFloat(input_section, (name + "_RADIUS").c_str());

        color = ConfigGetParamString(input_section, (name + "_COLOR").c_str());
        if(7 == color.length() && '#' == color[0])
        {
            buttons_and_axes[i].col.r = stoul(color.substr(1,2), nullptr, 16);
            buttons_and_axes[i].col.g = stoul(color.substr(3,2), nullptr, 16);
            buttons_and_axes[i].col.b = stoul(color.substr(5,2), nullptr, 16);
        }
        else
        {
            DebugMessage(M64MSG_ERROR, "malformed color string for %s correct example: #FF00FF", buttons_and_axes[i].nice_name.c_str());
        }

        alpha = ConfigGetParamString(input_section, (name + "_A").c_str());
        if(2 == alpha.length())
        {
            buttons_and_axes[i].col.a = stoul(alpha, nullptr, 16);
        }
        else
        {
            DebugMessage(M64MSG_ERROR, "malformed alpha string for %s correct example: A7", buttons_and_axes[i].nice_name.c_str());
        }
    }

    render_joystick = ConfigGetParamBool(input_section, "render_joystick");
    render_dpad = ConfigGetParamBool(input_section, "render_dpad");
}

static void setup_button(_ba_id id, float x, float y, float radius, float color[4])
{
    for(unsigned int j=1,i=0,k=0;j<=BUTTON_POLYGON_SIZE;j++,i+=2,k+=4)
    {
        if(1 == rotate)
        {
            draw_info[id].vertices[i] = (screen_width - x)/aspect_ratio + radius * cos((2.f*M_PI * static_cast<float>(j))/static_cast<float>(BUTTON_POLYGON_SIZE) - M_PI/2.f);
            draw_info[id].vertices[i+1] = -y*aspect_ratio + radius * sin((2.f*M_PI * static_cast<float>(j))/static_cast<float>(BUTTON_POLYGON_SIZE) - M_PI/2.f);
        }
        else if(2 == rotate)
        {
            draw_info[id].vertices[i] = (x - screen_width) + radius * cos((2.f*M_PI * static_cast<float>(j))/static_cast<float>(BUTTON_POLYGON_SIZE) - M_PI/2.f);
            draw_info[id].vertices[i+1] = (y - screen_height) + radius * sin((2.f*M_PI * static_cast<float>(j))/static_cast<float>(BUTTON_POLYGON_SIZE) - M_PI/2.f);
        }
        else if(3 == rotate)
        {
            draw_info[id].vertices[i] = - x/aspect_ratio + radius * cos((2.f*M_PI * static_cast<float>(j))/static_cast<float>(BUTTON_POLYGON_SIZE) - M_PI/2.f);
            draw_info[id].vertices[i+1] = (screen_height - y)*aspect_ratio + radius * sin((2.f*M_PI * static_cast<float>(j))/static_cast<float>(BUTTON_POLYGON_SIZE) - M_PI/2.f);
        }
        else
        {
            draw_info[id].vertices[i] = x + radius * cos((2.f*M_PI * static_cast<float>(j))/static_cast<float>(BUTTON_POLYGON_SIZE) - M_PI/2.f);
            draw_info[id].vertices[i+1] = y + radius * sin((2.f*M_PI * static_cast<float>(j))/static_cast<float>(BUTTON_POLYGON_SIZE) - M_PI/2.f);
        }
        draw_info[id].colors[k] = color[0];
        draw_info[id].colors[k+1] = color[1];
        draw_info[id].colors[k+2] = color[2];
        draw_info[id].colors[k+3] = color[3];
        draw_info[id].colors_active[k] = color[0];
        draw_info[id].colors_active[k+1] = color[1];
        draw_info[id].colors_active[k+2] = color[2];
        draw_info[id].colors_active[k+3] = 1.f;
    }
}

static void ortho_matrix(GLfloat mat[16], float left, float right, float bottom, float top, float near, float far)
{
    /* first column */
    mat[0] = 2.0f / (right - left);
    mat[1] = 0;
    mat[2] = 0;
    mat[3] = 0;
    /* second column */
    mat[4] = 0;
    mat[5] = 2.0f / (top - bottom);
    mat[6] = 0;
    mat[7] = 0;
    /* third column */
    mat[8] = 0;
    mat[9] = 0;
    mat[10] = -2.0f / (far - near);
    mat[11] = 0;
    /* fourth column */
    mat[12] = - (right + left) / (right - left);
    mat[13] = - (top + bottom) / (top - bottom);
    mat[14] = - (far + near) / (far - near);
    mat[15] = 1;
}

static void set_rotation_matrix(GLuint loc, int rotate)
{
    GLfloat mat[16];

    /* first setup everything which is the same everytime */
    /* (X, X, 0, 0)
     * (X, X, 0, 0)
     * (0, 0, 1, 0)
     * (0, 0, 0, 1)
     */

    //mat[0] =  cos(angle);
    //mat[1] =  sin(angle);
    mat[2] = 0;
    mat[3] = 0;

    //mat[4] = -sin(angle);
    //mat[5] =  cos(angle);
    mat[6] = 0;
    mat[7] = 0;

    mat[8] = 0;
    mat[9] = 0;
    mat[10] = 1;
    mat[11] = 0;

    mat[12] = 0;
    mat[13] = 0;
    mat[14] = 0;
    mat[15] = 1;

    /* now set the actual rotation */
    if(1 == rotate) // 90 degree
    {
        mat[0] =  0;
        mat[1] =  1;
        mat[4] = -1;
        mat[5] =  0;
    }
    else if(2 == rotate) // 180 degree
    {
        mat[0] = -1;
        mat[1] =  0;
        mat[4] =  0;
        mat[5] =  -1;
    }
    else if(3 == rotate) // 270 degree
    {
        mat[0] =  0;
        mat[1] = -1;
        mat[4] =  1;
        mat[5] =  0;
    }
    else /* 0 degree, also fallback if input is wrong) */
    {
        mat[0] =  1;
        mat[1] =  0;
        mat[4] =  0;
        mat[5] =  1;
    }

    glUniformMatrix4fv(loc, 1, GL_FALSE, mat);
}

static bool init_gl()
{
    GLint linked;

    // compile the shaders
    GLint success;
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, (const char**) &v_shader_str, NULL);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        DebugMessage(M64MSG_ERROR, "failed to compile the vertex shader");
        return false;
    }

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, (const char**) &f_shader_str, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        DebugMessage(M64MSG_ERROR, "failed to compile the fragment shader");
        return false;
    }

    // Create the program object.
    buttons_program = glCreateProgram();

    if (buttons_program == 0) {
        DebugMessage(M64MSG_ERROR, "glCreateProgram returned 0 (no valid opengl context?)");
        return false;
    }

    glAttachShader(buttons_program, vertex_shader);
    glAttachShader(buttons_program, fragment_shader);

    // Bind artributes.
    glBindAttribLocation(buttons_program, POSITION_ATTR, "a_position");
    glBindAttribLocation(buttons_program, COLOR_ATTR, "a_color");

    // Link the program and check linking result.
    glLinkProgram(buttons_program);
    glGetProgramiv(buttons_program, GL_LINK_STATUS, &linked);
    if(!linked)
    {
        GLint infoLen = 0;

        glGetProgramiv(buttons_program, GL_INFO_LOG_LENGTH, &infoLen);

        if(infoLen > 1)
        {
            char* infoLog = (char*)malloc(sizeof(char) * infoLen);

            glGetProgramInfoLog(buttons_program, infoLen, NULL, infoLog);
            DebugMessage(M64MSG_ERROR, "failed linking program:\n%s\n", infoLog);
            free(infoLog);
        }

        glDeleteProgram(buttons_program);
        return false;
    }

    glUseProgram(buttons_program);

    projection_matrix_location = glGetUniformLocation(buttons_program, "projection_matrix");

    ortho_matrix(projection_matrix, 0.0f, screen_widthf, screen_heightf, 0.0f, -1.0f, 1.0f);

    glUniformMatrix4fv(projection_matrix_location, 1, GL_FALSE, &projection_matrix[0]);

    rotation_matrix_location = glGetUniformLocation(buttons_program, "rotation_matrix");
    set_rotation_matrix(rotation_matrix_location, rotate);

    for(auto it=buttons_and_axes.begin();it!=buttons_and_axes.end();it++)
    {
        float color[4];

        glGenBuffers(1, &draw_info[it->id].vbo);
        glGenBuffers(1, &draw_info[it->id].cbo);
        glGenBuffers(1, &draw_info[it->id].cabo);

        color[0] = static_cast<float>(it->col.r)/255.f;
        color[1] = static_cast<float>(it->col.g)/255.f;
        color[2] = static_cast<float>(it->col.b)/255.f;
        color[3] = static_cast<float>(it->col.a)/255.f;

        setup_button(it->id,it->pos.x,it->pos.y,it->radius,color);

        glBindBuffer(GL_ARRAY_BUFFER, draw_info[it->id].vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(draw_info[it->id].vertices), draw_info[it->id].vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, draw_info[it->id].cbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(draw_info[it->id].colors), draw_info[it->id].colors, GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, draw_info[it->id].cabo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(draw_info[it->id].colors_active), draw_info[it->id].colors_active, GL_STATIC_DRAW);
    }

    gl_initialized = true;
    return true;
}

static void render_button(_ba_id id, bool active)
{
    glBindBuffer(GL_ARRAY_BUFFER, draw_info[id].vbo);
    glVertexAttribPointer(POSITION_ATTR, 2, GL_FLOAT, GL_FALSE, 0, 0);

    if(active) glBindBuffer(GL_ARRAY_BUFFER, draw_info[id].cabo);
    else glBindBuffer(GL_ARRAY_BUFFER, draw_info[id].cbo);
    glVertexAttribPointer(COLOR_ATTR, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_TRIANGLE_FAN, 0, BUTTON_POLYGON_SIZE);
}

static float distance(float x1, float y1, float x2, float y2)
{
    return sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
}

static float norm(float x, float y)
{
    return sqrt(x*x + y*y);
}

static void set_n64_button(int id, bool on, BUTTONS *Keys)
{
    if(BUTTON_A == id)
    {
        Keys->A_BUTTON = (int)on;
    }
    else if(BUTTON_B == id)
    {
        Keys->B_BUTTON = (int)on;
    }
    else if(BUTTON_L == id)
    {
        Keys->L_TRIG = (int)on;
    }
    else if(BUTTON_R == id)
    {
        Keys->R_TRIG = (int)on;
    }
    else if(BUTTON_Z == id)
    {
        Keys->Z_TRIG = (int)on;
    }
    else if(BUTTON_START == id)
    {
        Keys->START_BUTTON = (int)on;
    }
    else if(BUTTON_HB == id)
    {
        hide_buttons = !hide_buttons;
    }
    else if(BUTTON_CRIGHT == id)
    {
        Keys->R_CBUTTON = (int)on;
    }
    else if(BUTTON_CDOWN == id)
    {
        Keys->D_CBUTTON = (int)on;
    }
    else if(BUTTON_CLEFT == id)
    {
        Keys->L_CBUTTON = (int)on;
    }
    else if(BUTTON_CUP == id)
    {
        Keys->U_CBUTTON = (int)on;
    }
    else if(BUTTON_CDOWN == id)
    {
        Keys->D_CBUTTON = (int)on;
    }
    else if(BUTTON_DRIGHT == id)
    {
        Keys->R_DPAD = (int)on;
    }
    else if(BUTTON_DDOWN == id)
    {
        Keys->D_DPAD = (int)on;
    }
    else if(BUTTON_DLEFT == id)
    {
        Keys->L_DPAD = (int)on;
    }
    else if(BUTTON_DUP == id)
    {
        Keys->U_DPAD = (int)on;
    }
    else if(JOYSTICK_AXIS == id)
    {
        if(!on)
        {
            Keys->X_AXIS = 0;
            Keys->Y_AXIS = 0;
        }
    }
    else
    {
        DebugMessage(M64MSG_ERROR, "invalid button id %d", id);
    }
}

static std::list<_ba_id> get_buttons_pressed_by_fingerId(unsigned int fingerId)
{
    std::list<_ba_id> ret;
    for(unsigned int i=0;i<NUM_BUTTONS_AND_AXES;i++)
    {
        if(fingerIds[i] == fingerId)
        {
            ret.push_back((_ba_id)i);
        }
    }
    return ret;
}

static void press_button(_bainfo &ba, unsigned int finger_id, BUTTONS *Keys)
{
    set_n64_button(ba.id, true, Keys);
    ba.pressed = true;
    fingerIds[ba.id] = finger_id;
}

static void unpress_button(unsigned int finger_id, BUTTONS *Keys)
{
    std::list<_ba_id> buttons = get_buttons_pressed_by_fingerId(finger_id);
    for(auto it=buttons.begin();it!=buttons.end();it++)
    {
        set_n64_button(*it, false, Keys);
        fingerIds[*it] = -1;
        for(auto it2=buttons_and_axes.begin();it2!=buttons_and_axes.end();it2++)
        {
            if(it2->id == *it)
            {
                it2->pressed = false;

                if(BUTTON_HB == it2->id)
                {
                    render_ba = !render_ba;
                }
            }
        }
    }
}

static void process_sdl_events()
{
    SDL_Event event;

    while(SDL_PollEvent(&event))
    {
        if(SDL_FINGERDOWN == event.type)
        {
            input_slots[event.tfinger.fingerId].pressed = true;
            input_slots[event.tfinger.fingerId].released = false;
        }
        else if(SDL_FINGERUP == event.type)
        {
            input_slots[event.tfinger.fingerId].pressed = false;
            input_slots[event.tfinger.fingerId].released = true;
        }

        if(SDL_FINGERDOWN == event.type || SDL_FINGERUP == event.type || SDL_FINGERMOTION == event.type)
        {
            if(1 == rotate)
            {
                input_slots[event.tfinger.fingerId].x = screen_width - static_cast<unsigned int>(static_cast<float>(event.tfinger.y)*aspect_ratio);
                input_slots[event.tfinger.fingerId].y = static_cast<unsigned int>(static_cast<float>(event.tfinger.x)/aspect_ratio);
            }
            else if(2 == rotate)
            {
                input_slots[event.tfinger.fingerId].x = screen_width - event.tfinger.x;
                input_slots[event.tfinger.fingerId].y = screen_height - event.tfinger.y;
            }
            else if(3 == rotate)
            {
                input_slots[event.tfinger.fingerId].x = static_cast<unsigned int>(static_cast<float>(event.tfinger.y)*aspect_ratio);
                input_slots[event.tfinger.fingerId].y = screen_height - static_cast<unsigned int>(static_cast<float>(event.tfinger.x)/aspect_ratio);
            }
            else
            {
                input_slots[event.tfinger.fingerId].x = event.tfinger.x;
                input_slots[event.tfinger.fingerId].y = event.tfinger.y;
            }
        }
    }
}

/* Mupen64Plus plugin functions */
EXPORT m64p_error CALL PluginStartup(m64p_dynlib_handle CoreLibHandle, void *Context,
                                   void (*DebugCallback)(void *, int, const char *))
{
    ptr_CoreGetAPIVersions CoreAPIVersionFunc;
    
    //int i, 
    int ConfigAPIVersion, DebugAPIVersion, VidextAPIVersion;

    if (l_PluginInit)
        return M64ERR_ALREADY_INIT;

    /* first thing is to set the callback function for debug info */
    l_DebugCallback = DebugCallback;
    l_DebugCallContext = Context;

    /* attach and call the CoreGetAPIVersions function, check Config API version for compatibility */
    CoreAPIVersionFunc = (ptr_CoreGetAPIVersions) osal_dynlib_getproc(CoreLibHandle, "CoreGetAPIVersions");
    if (CoreAPIVersionFunc == NULL)
    {
        DebugMessage(M64MSG_ERROR, "Core emulator broken; no CoreAPIVersionFunc() function found.");
        return M64ERR_INCOMPATIBLE;
    }
    
    (*CoreAPIVersionFunc)(&ConfigAPIVersion, &DebugAPIVersion, &VidextAPIVersion, NULL);
    if ((ConfigAPIVersion & 0xffff0000) != (CONFIG_API_VERSION & 0xffff0000) || ConfigAPIVersion < CONFIG_API_VERSION)
    {
        DebugMessage(M64MSG_ERROR, "Emulator core Config API (v%i.%i.%i) incompatible with plugin (v%i.%i.%i)",
                VERSION_PRINTF_SPLIT(ConfigAPIVersion), VERSION_PRINTF_SPLIT(CONFIG_API_VERSION));
        return M64ERR_INCOMPATIBLE;
    }

    /* Get the core config function pointers from the library handle */
    ConfigOpenSection = (ptr_ConfigOpenSection) osal_dynlib_getproc(CoreLibHandle, "ConfigOpenSection");
    ConfigDeleteSection = (ptr_ConfigDeleteSection) osal_dynlib_getproc(CoreLibHandle, "ConfigDeleteSection");
    ConfigSaveFile = (ptr_ConfigSaveFile) osal_dynlib_getproc(CoreLibHandle, "ConfigSaveFile");
    ConfigSaveSection = (ptr_ConfigSaveSection) osal_dynlib_getproc(CoreLibHandle, "ConfigSaveSection");
    ConfigListParameters = (ptr_ConfigListParameters) osal_dynlib_getproc(CoreLibHandle, "ConfigListParameters");
    ConfigSetParameter = (ptr_ConfigSetParameter) osal_dynlib_getproc(CoreLibHandle, "ConfigSetParameter");
    ConfigGetParameter = (ptr_ConfigGetParameter) osal_dynlib_getproc(CoreLibHandle, "ConfigGetParameter");
    ConfigSetDefaultInt = (ptr_ConfigSetDefaultInt) osal_dynlib_getproc(CoreLibHandle, "ConfigSetDefaultInt");
    ConfigSetDefaultFloat = (ptr_ConfigSetDefaultFloat) osal_dynlib_getproc(CoreLibHandle, "ConfigSetDefaultFloat");
    ConfigSetDefaultBool = (ptr_ConfigSetDefaultBool) osal_dynlib_getproc(CoreLibHandle, "ConfigSetDefaultBool");
    ConfigSetDefaultString = (ptr_ConfigSetDefaultString) osal_dynlib_getproc(CoreLibHandle, "ConfigSetDefaultString");
    ConfigGetParamInt = (ptr_ConfigGetParamInt) osal_dynlib_getproc(CoreLibHandle, "ConfigGetParamInt");
    ConfigGetParamFloat = (ptr_ConfigGetParamFloat) osal_dynlib_getproc(CoreLibHandle, "ConfigGetParamFloat");
    ConfigGetParamBool = (ptr_ConfigGetParamBool) osal_dynlib_getproc(CoreLibHandle, "ConfigGetParamBool");
    ConfigGetParamString = (ptr_ConfigGetParamString) osal_dynlib_getproc(CoreLibHandle, "ConfigGetParamString");

    ConfigGetSharedDataFilepath = (ptr_ConfigGetSharedDataFilepath) osal_dynlib_getproc(CoreLibHandle, "ConfigGetSharedDataFilepath");
    ConfigGetUserConfigPath = (ptr_ConfigGetUserConfigPath) osal_dynlib_getproc(CoreLibHandle, "ConfigGetUserConfigPath");
    ConfigGetUserDataPath = (ptr_ConfigGetUserDataPath) osal_dynlib_getproc(CoreLibHandle, "ConfigGetUserDataPath");
    ConfigGetUserCachePath = (ptr_ConfigGetUserCachePath) osal_dynlib_getproc(CoreLibHandle, "ConfigGetUserCachePath");

    if (!ConfigOpenSection || !ConfigDeleteSection || !ConfigSaveFile || !ConfigSaveSection || !ConfigSetParameter || !ConfigGetParameter ||
        !ConfigSetDefaultInt || !ConfigSetDefaultFloat || !ConfigSetDefaultBool || !ConfigSetDefaultString ||
        !ConfigGetParamInt   || !ConfigGetParamFloat   || !ConfigGetParamBool   || !ConfigGetParamString ||
        !ConfigGetSharedDataFilepath || !ConfigGetUserConfigPath || !ConfigGetUserDataPath || !ConfigGetUserCachePath)
    {
        DebugMessage(M64MSG_ERROR, "Couldn't connect to Core configuration functions");
        return M64ERR_INCOMPATIBLE;
    }

    if (ConfigOpenSection("Video-General", &video_general_section) != M64ERR_SUCCESS)
    {
        DebugMessage(M64MSG_ERROR, "Couldn't open the Video-General configuration section.");
        return M64ERR_FILES;
    }

    if (ConfigOpenSection("Input-Touch", &input_section) != M64ERR_SUCCESS)
    {
        DebugMessage(M64MSG_ERROR, "Couldn't open the Input-Touch configuration section.");
        return M64ERR_FILES;
    }

    screen_height = ConfigGetParamInt(video_general_section, "ScreenHeight");
    screen_width = ConfigGetParamInt(video_general_section, "ScreenWidth");

    rotate = ConfigGetParamInt(video_general_section, "Rotate");

    screen_widthf = static_cast<float>(screen_width);
    screen_heightf = static_cast<float>(screen_height);
    aspect_ratio = screen_widthf / screen_heightf;

    if(1 == rotate)
    {
        corr_factor_x = 1.0f;
        corr_factor_y = aspect_ratio;
    }
    else if(2 == rotate)
    {
        corr_factor_x = aspect_ratio;
        corr_factor_y = 1.0f;
    }
    else if(3 == rotate)
    {
        corr_factor_x = 1.0f;
        corr_factor_y = aspect_ratio;
    }
    else
    {
        corr_factor_x = aspect_ratio;
        corr_factor_y = 1.0f;
    }

    generate_default_positions();

#if _DEBUG
    for(unsigned int i=0;i<buttons_and_axes.size();i++)
    {
        DebugMessage(M64MSG_INFO, "placed %s at (%d,%d) radius: %f", buttons_and_axes[i].name.c_str(), buttons_and_axes[i].pos.x, buttons_and_axes[i].pos.y, buttons_and_axes[i].radius);
    }
#endif

    set_default_cfg_values();

    load_configuration();

    l_PluginInit = 1;
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL PluginShutdown(void)
{
    if (!l_PluginInit)
        return M64ERR_NOT_INIT;

    /* reset some local variables */
    l_DebugCallback = NULL;
    l_DebugCallContext = NULL;

    glDeleteProgram(buttons_program);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    for(auto it=buttons_and_axes.begin();it!=buttons_and_axes.end();it++)
    {
        glDeleteBuffers(1, &draw_info[it->id].vbo);
        glDeleteBuffers(1, &draw_info[it->id].cbo);
        glDeleteBuffers(1, &draw_info[it->id].cabo);
    }

    l_PluginInit = 0;
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL PluginGetVersion(m64p_plugin_type *PluginType, int *PluginVersion, int *APIVersion, const char **PluginNamePtr, int *Capabilities)
{
    /* set version info */
    if (PluginType != NULL)
        *PluginType = M64PLUGIN_INPUT;

    if (PluginVersion != NULL)
        *PluginVersion = PLUGIN_VERSION;

    if (APIVersion != NULL)
        *APIVersion = INPUT_PLUGIN_API_VERSION;
    
    if (PluginNamePtr != NULL)
        *PluginNamePtr = PLUGIN_NAME;

    if (Capabilities != NULL)
    {
        *Capabilities = 0;
    }
                    
    return M64ERR_SUCCESS;
}

#if 0
static unsigned char DataCRC( unsigned char *Data, int iLenght )
{
    unsigned char Remainder = Data[0];

    int iByte = 1;
    unsigned char bBit = 0;

    while( iByte <= iLenght )
    {
        int HighBit = ((Remainder & 0x80) != 0);
        Remainder = Remainder << 1;

        Remainder += ( iByte < iLenght && Data[iByte] & (0x80 >> bBit )) ? 1 : 0;

        Remainder ^= (HighBit) ? 0x85 : 0;

        bBit++;
        iByte += bBit/8;
        bBit %= 8;
    }

    return Remainder;
}
#endif

/******************************************************************
  Function: RenderCallback
  Purpose:  To draw on screen information, for example buttons.
  input:    none
  output:   none
*******************************************************************/
EXPORT void CALL RenderCallback()
{
    GLuint saved_prog;
    GLuint saved_buf;

    glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&saved_prog);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint*)&saved_buf);

    /* we call init_gl here and not in PluginStartup because in PluginStartup
     * we cannot expect that the graphics plugin has already set up an opengl
     * context (input plugin gets initialized before the gfx plugin)
     */
    if(!gl_initialized)
    {
        if(!init_gl()) return;
    }

    glUseProgram(buttons_program);

    glEnableVertexAttribArray(POSITION_ATTR);
    glEnableVertexAttribArray(COLOR_ATTR);

    for(auto it=buttons_and_axes.begin();it!=buttons_and_axes.end();it++)
    {
        if  (   !(!render_joystick && JOYSTICK_AXIS == it->id) &&
                !(!render_dpad && (BUTTON_DLEFT == it->id || BUTTON_DDOWN == it->id || BUTTON_DRIGHT == it->id || BUTTON_DUP == it->id)) &&
                render_ba
            )
        {
            render_button(it->id, it->pressed);
        }
    }

    glDisableVertexAttribArray(POSITION_ATTR);
    glDisableVertexAttribArray(COLOR_ATTR);


    glUseProgram(saved_prog);
    glBindBuffer(GL_ARRAY_BUFFER, saved_buf);
}

/******************************************************************
  Function: ControllerCommand
  Purpose:  To process the raw data that has just been sent to a
            specific controller.
  input:    - Controller Number (0 to 3) and -1 signalling end of
              processing the pif ram.
            - Pointer of data to be processed.
  output:   none

  note:     This function is only needed if the DLL is allowing raw
            data, or the plugin is set to raw

            the data that is being processed looks like this:
            initilize controller: 01 03 00 FF FF FF
            read controller:      01 04 01 FF FF FF FF
*******************************************************************/
EXPORT void CALL ControllerCommand(int Control, unsigned char *Command)
{
#if 0
    unsigned char *Data = &Command[5];
#endif
    if (Control == -1)
        return;

    switch (Command[2])
    {
        case RD_GETSTATUS:
#ifdef _DEBUG
            DebugMessage(M64MSG_INFO, "Get status");
#endif
            break;
        case RD_READKEYS:
#ifdef _DEBUG
            DebugMessage(M64MSG_INFO, "Read keys");
#endif
            break;
        case RD_READPAK:
#ifdef _DEBUG
            DebugMessage(M64MSG_INFO, "Read pak");
#endif
#if 0
            if (controller[Control].control->Plugin == PLUGIN_RAW)
            {
                unsigned int dwAddress = (Command[3] << 8) + (Command[4] & 0xE0);

                if(( dwAddress >= 0x8000 ) && ( dwAddress < 0x9000 ) )
                    memset( Data, 0x80, 32 );
                else
                    memset( Data, 0x00, 32 );

                Data[32] = DataCRC( Data, 32 );
            }
#endif
            break;
        case RD_WRITEPAK:
#ifdef _DEBUG
            DebugMessage(M64MSG_INFO, "Write pak");
#endif
#if 0
            if (controller[Control].control->Plugin == PLUGIN_RAW)
            {
                unsigned int dwAddress = (Command[3] << 8) + (Command[4] & 0xE0);
              if (dwAddress == PAK_IO_RUMBLE && *Data)
                    DebugMessage(M64MSG_VERBOSE, "Triggering rumble pack.");
#if SDL_VERSION_ATLEAST(2,0,0)
                if(dwAddress == PAK_IO_RUMBLE && controller[Control].event_joystick) {
                    if (*Data) {
                        SDL_HapticRumblePlay(controller[Control].event_joystick, 1, SDL_HAPTIC_INFINITY);
                    } else {
                        SDL_HapticRumbleStop(controller[Control].event_joystick);
                    }
                }
#elif __linux__
                struct input_event play;
                if( dwAddress == PAK_IO_RUMBLE && controller[Control].event_joystick != 0)
                {
                    if( *Data )
                    {
                        play.type = EV_FF;
                        play.code = ffeffect[Control].id;
                        play.value = 1;

                        if (write(controller[Control].event_joystick, (const void*) &play, sizeof(play)) == -1)
                            perror("Error starting rumble effect");

                    }
                    else
                    {
                        play.type = EV_FF;
                        play.code = ffeffect[Control].id;
                        play.value = 0;

                        if (write(controller[Control].event_joystick, (const void*) &play, sizeof(play)) == -1)
                            perror("Error stopping rumble effect");
                    }
                }
#endif //__linux__
                Data[32] = DataCRC( Data, 32 );
            }
#endif
            break;
        case RD_RESETCONTROLLER:
#ifdef _DEBUG
            DebugMessage(M64MSG_INFO, "Reset controller");
#endif
            break;
        case RD_READEEPROM:
#ifdef _DEBUG
            DebugMessage(M64MSG_INFO, "Read eeprom");
#endif
            break;
        case RD_WRITEEPROM:
#ifdef _DEBUG
            DebugMessage(M64MSG_INFO, "Write eeprom");
#endif
            break;
        }
}

/******************************************************************
  Function: GetKeys
  Purpose:  To get the current state of the controllers buttons.
  input:    - Controller Number (0 to 3)
            - A pointer to a BUTTONS structure to be filled with
            the controller state.
  output:   none
*******************************************************************/
EXPORT void CALL GetKeys( int Control, BUTTONS *Keys )
{
    Keys->Value = 0;

    for(auto it=input_slots.begin();it!=input_slots.end();it++)
    {
        if(it->second.released)
        {
            auto tmp = it;
            tmp--;
            input_slots.erase(it);
            it = tmp;
        }
    }

    process_sdl_events();

    for(auto it=input_slots.begin();it!=input_slots.end();it++)
    {
        if(it->second.pressed)
        {
            bool axes_done = false;
            int ax_x = -1, ax_y = -1;

            for(unsigned int i=0;i<buttons_and_axes.size();i++)
            {
                if(JOYSTICK_AXIS == buttons_and_axes[i].id)
                {
                    ax_x = buttons_and_axes[i].pos.x;
                    ax_y = buttons_and_axes[i].pos.y;
                }
                if(distance(it->second.x*corr_factor_x,it->second.y*corr_factor_y,buttons_and_axes[i].pos.x*corr_factor_x,buttons_and_axes[i].pos.y*corr_factor_y) <= buttons_and_axes[i].radius)
                {
                    if(JOYSTICK_AXIS == buttons_and_axes[i].id)
                    {
                        int x = it->second.x - buttons_and_axes[i].pos.x;
                        int y = buttons_and_axes[i].pos.y - it->second.y;
                        int xn64, yn64;

                        xn64 = static_cast<int>((80.0 * static_cast<float>(y)*corr_factor_y)/buttons_and_axes[i].radius);
                        yn64 = static_cast<int>((80.0 * static_cast<float>(x)*corr_factor_x)/buttons_and_axes[i].radius);

                        if(abs(xn64) > 80 || abs(yn64) > 80)
                        {
                            DebugMessage(M64MSG_ERROR,"x_axis/y_axis calculation: %d, %d",xn64,yn64);
                            return;
                        }

                        Keys->X_AXIS = yn64;
                        Keys->Y_AXIS = xn64;

                        axes_done = true;
                    }
                    buttons_and_axes[i].pressed = true;
                    press_button(buttons_and_axes[i], it->first, Keys);
                }
            }
            if((!axes_done) && it->first == fingerIds[JOYSTICK_AXIS])
            {
                int x = it->second.x - ax_x;
                int y = ax_y - it->second.y;
                int xn64, yn64;

                xn64 = static_cast<int>(80.0 * static_cast<float>(y))/norm(x,y);
                yn64 = static_cast<int>(80.0 * static_cast<float>(x))/norm(x,y);
                    
                if(abs(xn64) > 80 || abs(yn64) > 80)
                {
                    DebugMessage(M64MSG_ERROR,"x_axis/y_axis calculation: %d, %d",xn64,yn64);
                    return;
                }

                Keys->X_AXIS = yn64;
                Keys->Y_AXIS = xn64;
            }
        }
        else if(it->second.released)
        {
            unpress_button(it->first, Keys);
        }
    }
}

/******************************************************************
  Function: InitiateControllers
  Purpose:  This function initialises how each of the controllers
            should be handled.
  input:    - The handle to the main window.
            - A controller structure that needs to be filled for
              the emulator to know how to handle each controller.
  output:   none
*******************************************************************/
EXPORT void CALL InitiateControllers(CONTROL_INFO ControlInfo)
{
    ControlInfo.Controls[0].Present = true;
    ControlInfo.Controls[0].Plugin = PLUGIN_RAW;

    DebugMessage(M64MSG_INFO, "%s version %i.%i.%i initialized.", PLUGIN_NAME, VERSION_PRINTF_SPLIT(PLUGIN_VERSION));
}

/******************************************************************
  Function: ReadController
  Purpose:  To process the raw data in the pif ram that is about to
            be read.
  input:    - Controller Number (0 to 3) and -1 signalling end of
              processing the pif ram.
            - Pointer of data to be processed.
  output:   none
  note:     This function is only needed if the DLL is allowing raw
            data.
*******************************************************************/
EXPORT void CALL ReadController(int Control, unsigned char *Command)
{
#ifdef _DEBUG
    if (Command != NULL)
        DebugMessage(M64MSG_INFO, "Raw Read (cont=%d):  %02X %02X %02X %02X %02X %02X", Control,
                     Command[0], Command[1], Command[2], Command[3], Command[4], Command[5]);
#endif
}

/******************************************************************
  Function: RomClosed
  Purpose:  This function is called when a rom is closed.
  input:    none
  output:   none
*******************************************************************/
EXPORT void CALL RomClosed(void)
{
    romopen = 0;
}

/******************************************************************
  Function: RomOpen
  Purpose:  This function is called when a rom is open. (from the
            emulation thread)
  input:    none
  output:   none
*******************************************************************/
EXPORT int CALL RomOpen(void)
{
    romopen = 1;
    return 1;
}

/******************************************************************
  Function: SDL_KeyDown
  Purpose:  To pass the SDL_KeyDown message from the emulator to the
            plugin.
  input:    keymod and keysym of the SDL_KEYDOWN message.
  output:   none
*******************************************************************/
EXPORT void CALL SDL_KeyDown(int keymod, int keysym)
{
}

/******************************************************************
  Function: SDL_KeyUp
  Purpose:  To pass the SDL_KeyUp message from the emulator to the
            plugin.
  input:    keymod and keysym of the SDL_KEYUP message.
  output:   none
*******************************************************************/
EXPORT void CALL SDL_KeyUp(int keymod, int keysym)
{
}

