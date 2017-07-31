/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Josh Aas <josh@mozilla.com>
 *   Jim Mathies <jmathies@mozilla.com>
 *   Tamir Evan <tamirevan@gmail.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

// This just needs to include NPAPI headers, change the path to whatever works
// for you.

#include <windows.h>

#include "../../headers/npfunctions.h"

#include <stdlib.h> /* malloc(), free() */
#include <stddef.h>     /* offsetof() */

static NPNetscapeFuncs* sBrowserFuncs = NULL;

typedef struct InstanceData {
  NPP npp;
  NPWindow window;
} InstanceData;

static void
drawToDC(InstanceData* instanceData, HDC dc)
{
  int x = instanceData->window.x;
  int y = instanceData->window.y;
  int width = instanceData->window.width;
  int height = instanceData->window.height;

  const RECT border = { x, y, x + width, y + height };

  int oldBkMode = SetBkMode(dc, TRANSPARENT);
  HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
  if (brush) {
    FillRect(dc, &border, brush);
    DeleteObject(brush);
  }
  if (width > 6 && height > 6) {
    brush = CreateSolidBrush(RGB(192, 192, 192));
    if (brush) {
      RECT fill = { x + 3, y + 3, x + width - 3, y + height - 3 };
      FillRect(dc, &fill, brush);
      DeleteObject(brush);
    }
  }

  const char* uaString = sBrowserFuncs->uagent(instanceData->npp);
  if (uaString && width > 10 && height > 10) {
    HFONT font = CreateFontA( 20, 0, 0, 0, 400, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                              CLIP_DEFAULT_PRECIS, 5, // CLEARTYPE_QUALITY
                              DEFAULT_PITCH, "Arial");
    if (font) {
      HFONT oldFont = (HFONT)SelectObject(dc, font);
      RECT inset = { x + 5, y + 5, x + width - 5, y + height - 5 };
      DrawTextA( dc, uaString, -1, &inset,
                 DT_LEFT | DT_TOP | DT_NOPREFIX | DT_WORDBREAK);
      SelectObject(dc, oldFont);
      DeleteObject(font);
    }
  }
  SetBkMode(dc, oldBkMode);
}

NPError OSCALL
NP_Initialize(NPNetscapeFuncs* bFuncs)
{  
  // Save the browser function table.
  sBrowserFuncs = bFuncs;

  return NPERR_NO_ERROR;
}

// Function called by the browser to get the plugin's function table.
NPError OSCALL
NP_GetEntryPoints(NPPluginFuncs* pFuncs)
{
  // Check the size of the provided structure based on the offset of the
  // last member we need.
  if (pFuncs->size < (offsetof(NPPluginFuncs, setvalue) + sizeof(void*)))
    return NPERR_INVALID_FUNCTABLE_ERROR;

  pFuncs->newp = NPP_New;
  pFuncs->destroy = NPP_Destroy;
  pFuncs->setwindow = NPP_SetWindow;
  pFuncs->newstream = NPP_NewStream;
  pFuncs->destroystream = NPP_DestroyStream;
  pFuncs->asfile = NPP_StreamAsFile;
  pFuncs->writeready = NPP_WriteReady;
  pFuncs->write = NPP_Write;
  pFuncs->print = NPP_Print;
  pFuncs->event = NPP_HandleEvent;
  pFuncs->urlnotify = NPP_URLNotify;
  pFuncs->getvalue = NPP_GetValue;
  pFuncs->setvalue = NPP_SetValue;

  return NPERR_NO_ERROR;
}

NPError OSCALL
NP_Shutdown()
{
  return NPERR_NO_ERROR;
}

NPError
NPP_New(NPMIMEType pluginType, NPP instance, uint16_t mode, int16_t argc, char* argn[], char* argv[], NPSavedData* saved) {
  // Make sure we can render this plugin
  NPBool browserSupportsWindowless = false;
  sBrowserFuncs->getvalue(instance, NPNVSupportsWindowless, &browserSupportsWindowless);
  if (!browserSupportsWindowless) {
    MessageBox( NULL,(LPCSTR) "Windowless mode not supported by the browser",(LPCSTR) "Basic plugin", MB_ICONERROR);
    return NPERR_GENERIC_ERROR;
  }

  sBrowserFuncs->setvalue(instance, NPPVpluginWindowBool, (void*)false);

  // set up our our instance data
  InstanceData* instanceData = (InstanceData*)malloc(sizeof(InstanceData));
  if (!instanceData)
    return NPERR_OUT_OF_MEMORY_ERROR;
  memset(instanceData, 0, sizeof(InstanceData));
  instanceData->npp = instance;
  instance->pdata = instanceData;

  return NPERR_NO_ERROR;
}

NPError
NPP_Destroy(NPP instance, NPSavedData** save) {
  InstanceData* instanceData = (InstanceData*)(instance->pdata);
  free(instanceData);
  return NPERR_NO_ERROR;
}

NPError
NPP_SetWindow(NPP instance, NPWindow* window) {
  InstanceData* instanceData = (InstanceData*)(instance->pdata);
  instanceData->window = *window;
  return NPERR_NO_ERROR;
}

NPError
NPP_NewStream(NPP instance, NPMIMEType type, NPStream* stream, NPBool seekable, uint16_t* stype) {
  return NPERR_GENERIC_ERROR;
}

NPError
NPP_DestroyStream(NPP instance, NPStream* stream, NPReason reason) {
  return NPERR_GENERIC_ERROR;
}

int32_t
NPP_WriteReady(NPP instance, NPStream* stream) {
  return 0;
}

int32_t
NPP_Write(NPP instance, NPStream* stream, int32_t offset, int32_t len, void* buffer) {
  return 0;
}

void
NPP_StreamAsFile(NPP instance, NPStream* stream, const char* fname) {

}

void
NPP_Print(NPP instance, NPPrint* platformPrint) {

}

int16_t
NPP_HandleEvent(NPP instance, void* event) {
  InstanceData *instanceData = (InstanceData*)(instance->pdata);
  NPEvent *nativeEvent = (NPEvent*)event;

  if (nativeEvent->event != WM_PAINT)
    return 0;

  HDC hdc = (HDC)instanceData->window.window;
  if (hdc == NULL)
    return 0;

  // Push the browser's hdc on the resource stack, as
  // we share the drawing surface with the rest of the browser.
  int savedDCID = SaveDC(hdc);

  drawToDC(instanceData, hdc);

  // Pop our hdc changes off the resource stack.
  RestoreDC(hdc, savedDCID);
  return 1;
}

void
NPP_URLNotify(NPP instance, const char* URL, NPReason reason, void* notifyData) {

}

NPError
NPP_GetValue(NPP instance, NPPVariable variable, void *value) {
  return NPERR_GENERIC_ERROR;
}

NPError
NPP_SetValue(NPP instance, NPNVariable variable, void *value) {
  return NPERR_GENERIC_ERROR;
}

