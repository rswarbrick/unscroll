#ifndef HUNKS_H
#define HUNKS_H

#include <gdk/gdk.h>
#include <poppler.h>
#include <glib.h>

typedef struct _DblRectangle DblRectangle;
typedef struct _MappedRectangle MappedRectangle;

struct _DblRectangle {
  double x, y, width, height;
};

struct _MappedRectangle {
  guint src_page;
  GdkRectangle src;

  /*
    dest is the "destination rectangle" in the output
    document. MappedRectangle's are created by collate_pages, which
    knows the output aspect ratio. It scales hunks so that their width
    (the width of the bounding box of the source page) gets sent to
    1. Then the y coordinates of dest will be between 0 and the aspect
    ratio (~1.46 for A4 paper).
   */
  DblRectangle dest;
};

// find_new_layout returns a list of lists, one per output page. Each
// list consists of MappedRectangle instances.
GSList *find_new_layout (PopplerDocument* doc);

// Goes through a list of lists as above and frees all of the relevant
// memory.
void destroy_page_mappings (GSList **pages);

void print_mapped_rectangle (const MappedRectangle* mr);

#endif
