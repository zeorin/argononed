#ifndef _GRAPH_UI_H

#define _GRAPH_UI_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef enum{
  GRAPH_MODE_ANSI = 0,
  GRAPH_MODE_STDOUT = 0x002,
  GRAPH_MODE_STDERR = 0x003,
  GRAPH_MODE_NCURSE = 0x001,
  GRAPH_FLAG_OLED = 0x800,
} GraphModes;

typedef struct {
  int full;
  int empty;
  int begin;
  int finished;
  int  min;
  int  max;
  int  segments;
  int  *colours;
  GraphModes mode;
} GraphProperties;
typedef struct {
  int full_begin;
  int full_finished;
} GraphPropertiesEx;

//void *graph(int minv, int maxv, int value, GraphProperties prop);
//void *graph_test(int minv, int maxv, int value, GraphProperties prop);

void *graph(int minv, int maxv, int value, int pallet)
{
    int i,x;
    int s = 0;
    int *pos;
    void *ret;
    int pallets[][4] = {{92,93,91},{91,93,92},{96,92,93,91},{92,92,93,91}};
    int pallets_segs[] = {3,3,4,4};
    GraphProperties prop = {
        '=',  // full   char to show when full
        ' ',  // empty  char to show when empty
        '[',  // begin  char to show at beginning of graph
        ']',  // finished  char to show at end of graph
        0,    // min  0 or 1 is the start value
        20,   // max  size of graph
        0,    // segments  number of colours
        NULL,  // colours array
        GRAPH_MODE_STDOUT     //
    };
    if (pallet > 0 && pallet < 5) { prop.segments = pallets_segs[pallet - 1]; prop.colours = (int *)pallets[pallet - 1]; }
    ret = calloc( (prop.max - prop.min) + (prop.mode & GRAPH_FLAG_OLED ? 1 : 3), sizeof(int));
    x = (prop.min + (value - minv) * (prop.max - prop.min) / (maxv - minv ));
    pos = ret;
    if ( (prop.begin != 0) && (~prop.mode & GRAPH_FLAG_OLED) ) *pos++ = prop.begin;
    for(i=1;i<=prop.max;i++)
    {
        if (prop.segments > 0) s = (int)(((i - prop.min) * (prop.segments )) / ((prop.max+1) - prop.min)) + 1;
        *pos++ = i <= x ? (prop.full + (s << 16)): prop.empty;
    }
    if (prop.finished != 0 && ~prop.mode & GRAPH_FLAG_OLED) *pos = prop.finished;
    FILE *fpd = stdout;
    for(pos=ret;*pos!=0;pos++)
    {
      s = (*pos >> 16) & 0xffff;
      if(s == 0)
        fprintf (fpd, "%lc",*pos & 0xffff);
      else
        fprintf (fpd, "\e[%dm%lc\e[0m", prop.colours[s - 1], *pos & 0xffff);
    }
    free (ret);
    return NULL;
}
#endif
