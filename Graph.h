// Graph.h - Weighted graph (adjacency lists) used for two models:
//   * undirected campus map of buildings (BFS, DFS, Dijkstra, Kruskal MST)
//   * directed course-prerequisite graph (topological sort / cycle detection)
#pragma once
#include <climits>
#include <string>
#include "ADT.h"
#include "Heap.h"
#include "Sorting.h"
#include "StackQueue.h"

using namespace std;

namespace ums
{

    struct Edge
    {
        int to;
        int weight;
    };
    struct WeightedEdge
    {
        int u, v, w;
    };

    class Graph
    {
        struct QItem
        {
            int v;
            int d;
        };
        struct CloserFirst
        {
            bool operator()(const QItem &a, const QItem &b) const { return a.d < b.d; }
        };

    public:
        explicit Graph(bool directed = false) : directed_(directed), edges_(0) {}

        bool directed() const { return directed_; }
        int vertexCount() const { return names_.size(); }
        int edgeCount() const { return edges_; }
        const string &name(int i) const { return names_[i]; }
        const DynArray<Edge> &neighbors(int i) const { return adj_[i]; }

        // Adds a vertex (or returns the existing index if the name is already present).
        int addVertex(const string &name)
        {
            int i = indexOf(name);
            if (i >= 0)
                return i;
            names_.push_back(name);
            adj_.push_back(DynArray<Edge>());
            return names_.size() - 1;
        }
        int indexOf(const string &name) const
        {
            for (int i = 0; i < names_.size(); ++i)
                if (names_[i] == name)
                    return i;
            return -1;
        }
        bool hasEdge(int u, int v) const
        {
            if (u < 0 || u >= vertexCount())
                return false;
            for (int i = 0; i < adj_[u].size(); ++i)
                if (adj_[u][i].to == v)
                    return true;
            return false;
        }
        // Returns false for invalid vertices, self-loops, negative weights or duplicates.
        bool addEdge(int u, int v, int w = 1)
        {
            if (u < 0 || v < 0 || u >= vertexCount() || v >= vertexCount() || u == v || w < 0)
                return false;
            if (hasEdge(u, v))
                return false;
            adj_[u].push_back(Edge{v, w});
            if (!directed_)
                adj_[v].push_back(Edge{u, w});
            ++edges_;
            return true;
        }
        void removeEdge(int u, int v)
        {
            if (removeOne(u, v) && !directed_)
                removeOne(v, u);
        }

        // Breadth-first traversal: visit order (queue based)
        DynArray<int> bfs(int start) const
        {
            DynArray<int> order;
            int n = vertexCount();
            if (start < 0 || start >= n)
                return order;
            DynArray<char> seen(n, 0);
            Queue<int> q;
            q.enqueue(start);
            seen[start] = 1;
            int u;
            while (q.dequeue(u))
            {
                order.push_back(u);
                for (int i = 0; i < adj_[u].size(); ++i)
                {
                    int v = adj_[u][i].to;
                    if (!seen[v])
                    {
                        seen[v] = 1;
                        q.enqueue(v);
                    }
                }
            }
            return order;
        }

        // Depth-first traversal: recursive
        DynArray<int> dfs(int start) const
        {
            DynArray<int> order;
            int n = vertexCount();
            if (start < 0 || start >= n)
                return order;
            DynArray<char> seen(n, 0);
            dfsRec(start, seen, order);
            return order;
        }

        // Fewest-hops path (BFS tree). Returns false if unreachable.
        bool fewestHops(int s, int t, DynArray<int> &path) const
        {
            path = DynArray<int>();
            int n = vertexCount();
            if (s < 0 || t < 0 || s >= n || t >= n)
                return false;
            DynArray<int> prev(n, -1);
            DynArray<char> seen(n, 0);
            Queue<int> q;
            q.enqueue(s);
            seen[s] = 1;
            int u;
            while (q.dequeue(u))
            {
                for (int i = 0; i < adj_[u].size(); ++i)
                {
                    int v = adj_[u][i].to;
                    if (!seen[v])
                    {
                        seen[v] = 1;
                        prev[v] = u;
                        q.enqueue(v);
                    }
                }
            }
            if (!seen[t])
                return false;
            buildPath(prev, t, path);
            return true;
        }

        // Number of connected components (meaningful for undirected graphs).
        int components() const
        {
            int n = vertexCount(), count = 0;
            DynArray<char> seen(n, 0);
            for (int s = 0; s < n; ++s)
            {
                if (seen[s])
                    continue;
                ++count;
                DynArray<int> comp = bfs(s);
                for (int i = 0; i < comp.size(); ++i)
                    seen[comp[i]] = 1;
            }
            return count;
        }
        bool isConnected() const { return vertexCount() == 0 || components() == 1; }

        // Dijkstra's shortest path (min-heap priority queue)
        bool shortestPath(int s, int t, DynArray<int> &path, int &total) const
        {
            path = DynArray<int>();
            total = 0;
            int n = vertexCount();
            if (s < 0 || t < 0 || s >= n || t >= n)
                return false;
            const int INF = INT_MAX / 4; // leaves room so cur.d + weight cannot overflow
            DynArray<int> dist(n, INF);
            DynArray<int> prev(n, -1);
            PriorityQueue<QItem, CloserFirst> pq;
            dist[s] = 0;
            pq.push(QItem{s, 0});
            QItem cur;
            while (pq.pop(cur))
            {
                if (cur.d > dist[cur.v])
                    continue; // stale entry
                if (cur.v == t)
                    break;
                for (int i = 0; i < adj_[cur.v].size(); ++i)
                {
                    const Edge &e = adj_[cur.v][i];
                    int nd = cur.d + e.weight;
                    if (nd < dist[e.to])
                    {
                        dist[e.to] = nd;
                        prev[e.to] = cur.v;
                        pq.push(QItem{e.to, nd});
                    }
                }
            }
            if (dist[t] >= INF)
                return false;
            total = dist[t];
            buildPath(prev, t, path);
            return true;
        }

        // Kruskal's minimum spanning tree (sort edges + union-find)
        // Returns the total weight; `chosen` receives the MST (or forest) edges.
        int minimumSpanningTree(DynArray<WeightedEdge> &chosen) const
        {
            chosen = DynArray<WeightedEdge>();
            int n = vertexCount();
            DynArray<WeightedEdge> all;
            for (int u = 0; u < n; ++u)
                for (int i = 0; i < adj_[u].size(); ++i)
                    if (directed_ || u < adj_[u][i].to)
                        all.push_back(WeightedEdge{u, adj_[u][i].to, adj_[u][i].weight});
            mergeSort(all.data(), all.size(), [](const WeightedEdge &a, const WeightedEdge &b)
                      { return a.w < b.w; });
            DynArray<int> parent(n);
            for (int i = 0; i < n; ++i)
                parent[i] = i;
            auto findRoot = [&](int x)
            {
                while (parent[x] != x)
                {
                    parent[x] = parent[parent[x]]; // path halving
                    x = parent[x];
                }
                return x;
            };
            int total = 0;
            for (int i = 0; i < all.size(); ++i)
            {
                int a = findRoot(all[i].u), b = findRoot(all[i].v);
                if (a != b)
                {
                    parent[a] = b;
                    chosen.push_back(all[i]);
                    total += all[i].w;
                }
            }
            return total;
        }

        // Topological sort (Kahn's algorithm)
        // Returns false if the directed graph contains a cycle.
        bool topologicalSort(DynArray<int> &order) const
        {
            order = DynArray<int>();
            int n = vertexCount();
            DynArray<int> indeg(n, 0);
            for (int u = 0; u < n; ++u)
                for (int i = 0; i < adj_[u].size(); ++i)
                    ++indeg[adj_[u][i].to];
            Queue<int> q;
            for (int i = 0; i < n; ++i)
                if (indeg[i] == 0)
                    q.enqueue(i);
            int u;
            while (q.dequeue(u))
            {
                order.push_back(u);
                for (int i = 0; i < adj_[u].size(); ++i)
                    if (--indeg[adj_[u][i].to] == 0)
                        q.enqueue(adj_[u][i].to);
            }
            return order.size() == n;
        }

    private:
        void dfsRec(int u, DynArray<char> &seen, DynArray<int> &order) const
        {
            seen[u] = 1;
            order.push_back(u);
            for (int i = 0; i < adj_[u].size(); ++i)
                if (!seen[adj_[u][i].to])
                    dfsRec(adj_[u][i].to, seen, order);
        }
        static void buildPath(const DynArray<int> &prev, int t, DynArray<int> &path)
        {
            DynArray<int> rev;
            for (int v = t; v != -1; v = prev[v])
                rev.push_back(v);
            for (int i = rev.size() - 1; i >= 0; --i)
                path.push_back(rev[i]);
        }
        bool removeOne(int u, int v)
        {
            if (u < 0 || u >= vertexCount())
                return false;
            for (int i = 0; i < adj_[u].size(); ++i)
            {
                if (adj_[u][i].to == v)
                {
                    adj_[u].removeAt(i);
                    if (directed_ || u < v)
                        --edges_; // count each undirected edge once
                    return true;
                }
            }
            return false;
        }

        bool directed_;
        int edges_;
        DynArray<string> names_;
        DynArray<DynArray<Edge>> adj_;
    };

} // namespace ums