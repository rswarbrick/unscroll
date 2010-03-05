#ifndef PAGE_H
#define PAGE_H

#include <gdk/gdk.h>
#include <glib.h>
#include <poppler.h>

typedef struct _PageInfo PageInfo;
typedef struct _GapData GapData;

struct _GapData {
  int start;
  int end;
};

struct _PageInfo {
  GdkRectangle bbox;

  guint  num_gaps;
  GapData *gaps;
};

PageInfo *analyse_page (PopplerDocument *doc, guint page_num);
void print_page_info (const PageInfo *pi);
void free_page_info (PageInfo **pi);

#endif
