#ifndef PAGE_H
#define PAGE_H

#include <gdk/gdk.h>
#include <glib.h>
#include <poppler.h>

typedef struct _PageInfo PageInfo;
typedef struct _HunkData HunkData;

#define ZOOM_SCALE 1.5f

struct _HunkData {
  int start;
  int end;
};

struct _PageInfo {
  GdkRectangle bbox;

  guint  num_hunks;
  HunkData *hunks;
};

PageInfo *analyse_page (PopplerDocument *doc, guint page_num);
void print_page_info (const PageInfo *pi);
void free_page_info (PageInfo **pi);

#endif
