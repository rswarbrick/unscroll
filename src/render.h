#ifndef RENDER_H
#define RENDER_H

#include <poppler.h>
#include <glib.h>

void render_pdf (PopplerDocument* doc, GSList *page_mappings, const char* fname);

#endif
