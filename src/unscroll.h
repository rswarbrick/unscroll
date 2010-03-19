#ifndef UNSCROLL_H
#define UNSCROLL_H

typedef struct _Settings Settings;

struct _Settings {
  double dpi;
  const char* infile;
  const char* outfile;
  const char* papername;

  /* Width and height of the output paper in points */
  double pswidth, psheight;

  /* Margins. Currently hardcoded to the larger of 1cm and 0.05 times
   * the relevant dimension. */
  double psleft, psright, pstop, psbottom;
};

double output_bb_width ();
double output_bb_height ();
double output_rel_height ();

#endif
