#include "hunks.h"
#include "page.h"
#include <stdio.h>
#include <string.h>

// Idea of algorithm:
//
// 1) Read through the pdf at some fixed resolution using poppler and
//    use page.c to find out the distribution of hunks.
//
// 2) Rescale this per page to make the bounding box width 1.
//
// 3) Perform a "poor-man's justification" algorithm, which is
//    basically word-wrap followed by scaling the spacing to make us
//    fit the correct bounding box dimensions.
//
// 4) Return _this_ data in a useful format to the caller.

typedef struct _ScaledRectangle ScaledRectangle;

struct _ScaledRectangle {
  guint src_page;
  GdkRectangle src;
  double scaling;
};

static void rectangles_from_page (PopplerDocument *doc, guint pagenum,
                                  GSList **list)
{
  PageInfo* pi = analyse_page (doc, pagenum);
  int i, n;
  g_assert (doc);

  n = pi->num_hunks;

  for (i=0; i<n; i++) {
    ScaledRectangle *rect = g_new(ScaledRectangle, 1);
    rect->src_page = pagenum;
    rect->src.x = pi->bbox.x;
    rect->src.width = pi->bbox.width;
    rect->src.y = (pi->hunks + i)->start;
    rect->src.height = (pi->hunks + i)->end - (pi->hunks + i)->start;
    rect->scaling = 1.0 / pi->bbox.width;
    *list = g_slist_prepend (*list, rect);
  }

  free_page_info (&pi);
}

static GSList *collate_pages (GSList **pages, double bbox_rel_height)
{
  g_assert(pages && *pages);
  GSList *this_page = NULL;
  GSList *complete_pages = NULL;

  double current_height = 0;
  guint current_page = 0;
  ScaledRectangle *lastsr = NULL;

  while (*pages) {
    ScaledRectangle *sr = (ScaledRectangle *)(*pages)->data;
    double hunk_height = sr->scaling*sr->src.height;
    double gap_height;

    if (current_height == 0) gap_height = 0;
    else if (sr->src_page != current_page) {
      gap_height = 0.05;
    } else {
      gap_height = sr->src.y - (lastsr->src.y+lastsr->src.height);
      gap_height *= sr->scaling;
      if (gap_height > 0.1) gap_height = 0.1;
    }

    if (current_height == 0
        ||
        current_height + hunk_height + gap_height <= bbox_rel_height) {
      MappedRectangle *mr = g_new(MappedRectangle, 1);
      mr->src_page = sr->src_page;
      memcpy (&mr->src, &sr->src, sizeof(GdkRectangle));

      mr->dest.x = 0;
      mr->dest.width = 1;
      mr->dest.y = current_height + gap_height;
      mr->dest.height = hunk_height;

      this_page = g_slist_prepend (this_page, mr);

      current_height += hunk_height + gap_height;
      current_page = sr->src_page;

      g_free (lastsr);
      lastsr = sr;

      *pages = g_slist_next (*pages);
    } else {
      complete_pages = g_slist_prepend (complete_pages,
                                        g_slist_reverse (this_page));
      this_page = NULL;
      current_height = 0;
    }
  }

  // This is not a memory leak, since the data was freed as we went
  // along. We only still need to kill lastsr.
  *pages = NULL;
  g_free (lastsr);

  complete_pages = g_slist_prepend (complete_pages,
                                    g_slist_reverse (this_page));

  return g_slist_reverse (complete_pages);
}

GSList *find_new_layout (PopplerDocument* doc)
{
  int i;
  GSList *srlist = NULL;
  GSList *paged_lists = NULL;

  for (i=0; i<poppler_document_get_n_pages (doc); i++) {
    rectangles_from_page (doc, i, &srlist);
  }
  srlist = g_slist_reverse (srlist);

  printf ("Hunks found:  %5d\n", g_slist_length (srlist));

  paged_lists = collate_pages (&srlist, 1.46);

  printf ("Pages before: %5d\nPages after:  %5d\n",
          poppler_document_get_n_pages (doc),
          g_slist_length (paged_lists));

  return paged_lists;
}

void destroy_page_mappings (GSList **pages)
{
  GSList *ptr = *pages;
  while (ptr) {
    g_slist_free (ptr->data);
    ptr = g_slist_next(ptr);
  }
  g_slist_free (*pages);
  *pages = NULL;
}

void
print_mapped_rectangle (const MappedRectangle* mr)
{
  printf ("Rectangle from page %3d, x=%d, y=%d, w=%d, h=%d\n",
          mr->src_page,
          mr->src.x, mr->src.y, mr->src.width, mr->src.height);
  printf ("            to           x=%f, y=%f, w=%f, h=%f\n",
          mr->dest.x, mr->dest.y, mr->dest.width, mr->dest.height);
}
