#include <cstdlib>
#include <deque>
#include <iostream>
#include "SPPackingCommand.h"
#include "Layout.h"
#include <boost/graph/dag_shortest_paths.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/adjacency_list.hpp>

namespace RECTPACKING {
using namespace std;
using namespace boost;

static void OneDirectionPacking(vector<pair<int, int> > &cons, Layout &layout, bool hor)
{
  typedef adjacency_list < listS, vecS, directedS,
    property<vertex_distance_t, int>, property < edge_weight_t, int > > graph_t;
  typedef graph_traits < graph_t >::vertex_descriptor vertex_descriptor;
  typedef graph_traits < graph_t >::edge_descriptor edge_descriptor;
  //number of rects + virtual source + virtual target
  vector<Rect> &rects = layout.getRects();
  const int rectNum = rects.size();
  const int num_nodes = rectNum+2;
  const int sourceId = rectNum;
  const int sinkId = rectNum+1;
  graph_t g(num_nodes);
  property_map<graph_t, edge_weight_t>::type weightmap = get(edge_weight, g);
  //add the edges
  if (hor)
  {
    for (int i = 0; i < rectNum; ++ i)
    {
      add_edge(sourceId, i, -rects[i].width, g);
      add_edge(i, sinkId, 0, g);
    }
    for (int i = 0; i < (int)cons.size(); ++ i)
    {
      add_edge(cons[i].first, cons[i].second, -rects[cons[i].second].width, g);
    }
  }
  else
  {
    for (int i = 0; i < rectNum; ++ i)
    {
      add_edge(sourceId, i, -rects[i].height, g);
      add_edge(i, sinkId, 0, g);
    }
    for (int i = 0; i < (int)cons.size(); ++ i)
    {
      add_edge(cons[i].first, cons[i].second, -rects[cons[i].second].height, g);
    }
  }
  cout << "Finished building the graph with " << num_vertices(g) << " vertices and " << num_edges(g) << " edges" << endl;
  property_map<graph_t, vertex_distance_t>::type d_map = get(vertex_distance, g);
  vertex_descriptor s = vertex(sourceId, g);
#if defined(BOOST_MSVC) && BOOST_MSVC <= 1300
  // VC++ has trouble with the named-parameter mechanism, so
  // we make a direct call to the underlying implementation function.
  std::vector<default_color_type> color(num_vertices(g));
  std::vector<std::size_t> pred(num_vertices(g));
  default_dijkstra_visitor vis;
  std::less<int> compare;
  closed_plus<int> combine;
  property_map<graph_t, edge_weight_t>::type w_map = get(edge_weight, g);
  dag_shortest_paths(g, s, d_map, w_map, &color[0], &pred[0],
     vis, compare, combine, (std::numeric_limits<int>::max)(), 0);
#else
  dag_shortest_paths(g, s, distance_map(d_map));
#endif

  //set the coordinates of the rects
  graph_traits<graph_t>::vertex_iterator vi , vi_end;
  for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
    if (d_map[*vi] == (std::numeric_limits<int>::max)())
      std::cout << (int)(*vi) << ": inifinity\n";
    else
    {
      int dis = -d_map[*vi];
      std::cout << (int)(*vi) << ": " << dis << '\n';
      if (hor)
      {
        rects[*vi].lb.x = dis-rects[*vi].width;
      }
      else
      {
        rects[*vi].lb.y = dis-rects[*vi].height;
      }
    }
}

void LongestGraphPacking(vector<pair<int, int> > &horCons, vector<pair<int, int> > &verCons, Layout &layout)
{
  cout << "In LongestGraphPacking " << endl;
  bool hor = true;
  cout << "========== horizontal ==========" << endl;
  OneDirectionPacking(horCons, layout, hor);
  hor = false;
  cout << "========== vertical ==========" << endl;
  OneDirectionPacking(verCons, layout, hor);
}

void topoSortForSP(vector<pair<int, int> > &horCons, vector<pair<int, int> > &verCons, Layout &layout, vector<int> &s1, vector<int> &s2)
{
  cout << "================ In topoSortForSP ================" << endl;
  typedef adjacency_list < listS, vecS, directedS > graph_t;
  typedef graph_traits<graph_t>::vertex_descriptor Vertex;
  //number of rects + virtual source + virtual target
  vector<Rect> &rects = layout.getRects();
  const int rectNum = rects.size();
  const int num_nodes = rectNum;
/*
  const int num_nodes = rectNum+2;
  const int sourceId = rectNum;
  const int sinkId = rectNum+1;
*/
  graph_t g1(num_nodes), g2(num_nodes);
  //add the edges
/*
  for (int i = 0; i < rectNum; ++ i)
  {
    add_edge(sourceId, i, g1);
    cout << "G1: edge " << sourceId << "->" << i << endl;
    add_edge(i, sinkId, g1);
    cout << "G1: edge " << i << "->" << sinkId << endl;
    add_edge(sourceId, i, g2);
    cout << "G2: edge " << sourceId << "->" << i << endl;
    add_edge(i, sinkId, g2);
    cout << "G2: edge " << i << "->" << sinkId << endl;
  }
*/
  //for horizontal constraints
  for (int i = 0; i < (int)horCons.size(); ++ i)
  {
    int r1 = horCons[i].first;
    int r2 = horCons[i].second;
    cout << "R" << r1 << " is to the left of R" << r2 << endl;
    //r1 to the left of r2: bb
    add_edge(r1, r2, g1);
    cout << "G1: edge " << r1 << "->" << r2 << endl;
    add_edge(r1, r2, g2);
    cout << "G2: edge " << r1 << "->" << r2 << endl;
  }
  for (int i = 0; i < (int)verCons.size(); ++ i)
  {
    int r1 = verCons[i].first;
    int r2 = verCons[i].second;
    cout << "R" << r1 << " is below R" << r2 << endl;
    //r1 is below r2: ab
    add_edge(r2, r1, g1);
    cout << "G1: edge " << r2 << "->" << r1 << endl;
    add_edge(r1, r2, g2);
    cout << "G2: edge " << r1 << "->" << r2 << endl;
  }
  cout << "Finished building graph1 with " << num_vertices(g1) << " vertices and " << num_edges(g1) << " edges" << endl;
  cout << "Finished building graph2 with " << num_vertices(g2) << " vertices and " << num_edges(g2) << " edges" << endl;
/*
  typedef std::vector< Vertex > container;
  container c;
  cout << "Start sorting: " << endl;
  topological_sort(g1, std::back_inserter(c));

  cout << "A topological ordering: " << endl;

  for ( container::reverse_iterator ii=c.rbegin(); ii!=c.rend(); ++ii)
    cout << (int)(*ii) << " ";
  cout << endl;
*/

  deque<Vertex> topo_order1, topo_order2;
  //for s1 and s2
  topological_sort(g1, front_inserter(topo_order1), vertex_index_map(identity_property_map()));
  topological_sort(g2, front_inserter(topo_order2), vertex_index_map(identity_property_map()));

  s1.clear();
  s2.clear();
  s1.assign(rectNum, 0);
  s2.assign(rectNum, 0);
  int index = 0;
  cout << "Topo order 1: ";
  for (typeof(topo_order1.begin()) it = topo_order1.begin(); it != topo_order1.end(); ++it)
  {
    cout << *it << " ";
    s1[index++] = *it;
  }
  cout << endl;
  //for s2
  index = 0;
  cout << "Topo order 2: ";
  for (typeof(topo_order2.begin()) it = topo_order2.begin(); it != topo_order2.end(); ++it)
  {
    cout << *it << " ";
    s2[index++] = *it;
  }
  cout << endl;

}

}
