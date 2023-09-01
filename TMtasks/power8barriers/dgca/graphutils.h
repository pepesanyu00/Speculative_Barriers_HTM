#ifndef GRAPHUTILS_H
#define GRAPHUTILS_H

#include <assert.h>
#include "transaction.h" //RIC para define CACHE_BLOCK_SIZE
/* Aux functions and data structures for undirected graph with adjacency matrix */

/* Node info */
typedef struct {
  uint32_t color;
  uint8_t pad[CACHE_BLOCK_SIZE-sizeof(uint32_t)];
} __attribute__ ((aligned (CACHE_BLOCK_SIZE))) node_t;

/* Graph structure */
typedef struct {
  int nodes;
  uint8_t** adjMat;
  node_t* V;
} __attribute__ ((aligned (CACHE_BLOCK_SIZE))) graph_t;

/* Disperse graph structure */
typedef struct {
  int nodes;
  int* row;
  int* adjList;
  node_t* nodeInfo;
} __attribute__ ((aligned (CACHE_BLOCK_SIZE))) disgraph_t;

disgraph_t* convertGraph(graph_t ig) {
  int i, j, cnt;
  disgraph_t* og;

  og = (disgraph_t*) malloc(sizeof (disgraph_t));
  assert(og != NULL);

  og->nodes = ig.nodes;

  og->nodeInfo = (node_t*) malloc(ig.nodes * sizeof (node_t));
  assert(og->nodeInfo != NULL);

  og->row = (int*) malloc((ig.nodes + 1) * sizeof (int*));
  assert(og->row != NULL);

  /* Copy node attributes */
  for (i = 0; i < ig.nodes; i++) {
    og->nodeInfo[i].color = ig.V[i].color;
  }

  /* Pass 1: count all adjacencies to malloc adjList */
  cnt = 0;
  for (i = 0; i < ig.nodes; i++) {
    for (j = 0; j < ig.nodes; j++) {
      if (ig.adjMat[i][j] == 1) {
        cnt++;
      }
    }
  }

  og->adjList = (int*) malloc(cnt * sizeof (int*));
  assert(og->adjList != NULL);

  /* Pass 2: populate adjList */
  cnt = 0;
  for (i = 0; i < ig.nodes; i++) {
    og->row[i] = cnt;
    for (j = 0; j < ig.nodes; j++) {
      if (ig.adjMat[i][j] == 1) {
        og->adjList[cnt] = j;
        cnt++;
      }
    }
  }
  og->row[i] = cnt;

  return og;
}

/* Create an empty (dense) graph of n nodes */
graph_t* createGraph(int n) {
  int i;
  graph_t* g;

  g = (graph_t*) malloc(sizeof (graph_t));
  assert(g != NULL);
  g->nodes = n;
  g->V = (node_t*) malloc(n * sizeof (node_t));
  assert(g->V != NULL);

  g->adjMat = (uint8_t**) malloc(n * sizeof (uint8_t*));
  assert(g->adjMat != NULL);
  for (i = 0; i < n; i++) {
    g->adjMat[i] = (uint8_t*) calloc(n, sizeof (uint8_t));
    assert(g->adjMat[i] != NULL);
  }

  return g;
}

/* Deallocate graph */
void destroyGraph(graph_t* g) {
  int i;
  int nodes = g->nodes;

  for (i = 0; i < nodes; i++) {
    free(g->adjMat[i]);
  }
  free(g->adjMat);
  free(g->V);
  return;
}

/* Deallocate disperse graph */
void destroyDisgraph(disgraph_t* g) {
  free(g->nodeInfo);
  free(g->row);
  free(g->adjList);
  return;
}

/* Add connection between nodes v1 and v2 */
void addEdge(graph_t* g, int v1, int v2) {
  g->adjMat[v1][v2] = 1;
  g->adjMat[v2][v1] = 1;
}

/* Add color to a node */
void addColor(graph_t* g, int v, int color) {
  g->V[v].color = color;
}

/* Add color to a node */
void addColorDis(disgraph_t* g, int v, int color) {
  if (g->nodeInfo[v].color != color) g->nodeInfo[v].color = color;
}

/* Return color of a node */
int getColor(graph_t* g, int v) {
  return g->V[v].color;
}

/* Return color of a node */
int getColorDis(disgraph_t* g, int v) {
  return g->nodeInfo[v].color;
}

/* Return 1 if every two adjacent nodes have a different color, 0 otherwise */
int checkColors(graph_t g) {
  int i, j;
  int mycolor;

  for (i = 0; i < g.nodes; i++) {
    mycolor = g.V[i].color;
    for (j = 0; j < g.nodes; j++) {
      if (g.adjMat[i][j] == 1) {
        if (mycolor == g.V[j].color) {
          return 0;
        }
      }
    }
  }
  return 1;
}

/* Return 1 if every two adjacent nodes have a different color, 0 otherwise */
int checkColorsDis(disgraph_t g) {
  int i, j, start, stop;
  int mycolor;

  for (i = 0; i < g.nodes; i++) {
    start = g.row[i];
    stop = g.row[i + 1];
    mycolor = g.nodeInfo[i].color;
    for (j = start; j < stop; j++) {
      if (mycolor == g.nodeInfo[g.adjList[j]].color) {
        return 0;
      }
    }
  }
  return 1;
}

void printAdjList(graph_t g) {
  int i, j;
  int nodes = g.nodes;

  for (i = 0; i < nodes; i++) {
    printf("Node %d (col=%d): ", i, g.V[i].color);
    for (j = 0; j < nodes; j++) {
      if (g.adjMat[i][j] == 1) {
        printf("%d, ", j);
      }
    }
    printf("\b\b \n");
  }
}

void printAdjList2(disgraph_t dg) {
  int i, j, start, stop;

  for (i = 0; i < dg.nodes; i++) {
    printf("Node %d (col=%d): ", i, dg.nodeInfo[i].color);
    start = dg.row[i];
    stop = dg.row[i + 1];
    for (j = start; j < stop; j++) {
      printf("%d, ", dg.adjList[j]);
    }
    printf("\b\b \n");
  }
}

float getConnectivityDis(disgraph_t dg) {
  int i, j, start, stop, edgeCnt;

  edgeCnt = 0;

  for (i = 0; i < dg.nodes; i++) {
    start = dg.row[i];
    stop = dg.row[i + 1];
    for (j = start; j < stop; j++) {
      edgeCnt++;
    }
  }

  //edgeCnt = edgeCnt * 0.5;
  return (float) edgeCnt / (float) dg.nodes;

}

#endif /* GRAPHUTILS_H */
