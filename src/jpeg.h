#ifndef JPEG_H
#define JPEG_H

#include <glib.h>
#include <cairo.h>

guchar* make_jpeg_hunk (cairo_surface_t *img,
                        guint x, guint y, guint w, guint h,
                        guint* length);

void free_jpeg_hunk (guchar* hunk);

#endif
