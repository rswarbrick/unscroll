#ifndef HUNKS_H
#define HUNKS_H

#include <gdk/gdk.h>
#include <poppler.h>
#include <glib.h>

typedef struct _MappedRectangle MappedRectangle;

struct _MappedRectangle {
  guint src_page;
  GdkRectangle src, dest;
};

// find_new_layout returns a list of lists, one per output page. Each
// list consists of MappedRectangle instances.
GSList *find_new_layout (PopplerDocument* doc);

// Goes through a list of lists as above and frees all of the relevant
// memory.
void destroy_page_mappings (GSList **pages);

#endif
