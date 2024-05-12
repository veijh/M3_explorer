#ifndef HASTAR_H
#define HASTAR_H
#include <Eigen/Dense>
#include <octomap/octomap.h>
#include <ros/ros.h>

using namespace std;
const double kDiv = 12.0 / 2.0;

class PathNode {
private:
public:
  Eigen::Vector3f position;
  // yaw [-PI, PI]
  float yaw;

  // father_yaw + father_yaw_offset = yaw
  float father_yaw_offset;
  int father_id;
  float f_score, g_score, h_score;

  PathNode() { position = Eigen::Vector3f::Zero(); }
  PathNode(const Eigen::Vector3f &_position, const float &_yaw)
      : position(_position), yaw(_yaw){};
  PathNode(const float &px, const float &py, const float &pz, const float &_yaw)
      : position(Eigen::Vector3f(px, py, pz)), yaw(_yaw){};

  // 用于unordered_map
  bool operator==(const PathNode &n) const {
    int x0 = (int)(position.x() / 0.1);
    int y0 = (int)(position.y() / 0.1);

    int x1 = (int)(n.position.x() / 0.1);
    int y1 = (int)(n.position.y() / 0.1);

    // 偏航角离散为12个部分。由于+-12表示同一角度，因此实际可能有13个取值。
    int yaw0 = (int)(kDiv * yaw / M_PI);
    int yaw1 = (int)(kDiv * n.yaw / M_PI);

    return x0 == x1 && y0 == y1 && yaw0 == yaw1;
  }
};

// 用于优先队列
struct NodeCmp {
  bool operator()(const PathNode &lhs, const PathNode &rhs) {
    return lhs.f_score > rhs.f_score;
  }
};

// 用于map
struct MapCmp {
  bool operator()(const PathNode &lhs, const PathNode &rhs) {
    int x0 = (int)(lhs.position.x() / 0.1);
    int y0 = (int)(lhs.position.y() / 0.1);
    int z0 = (int)(lhs.position.z() / 0.1);

    int x1 = (int)(rhs.position.x() / 0.1);
    int y1 = (int)(rhs.position.y() / 0.1);
    int z1 = (int)(rhs.position.z() / 0.1);

    int yaw0 = (int)(kDiv * lhs.yaw / M_PI);
    int yaw1 = (int)(kDiv * rhs.yaw / M_PI);

    if (x0 != x1)
      return x0 < x1;
    if (y0 != y1)
      return y0 < y1;
    if (z0 != z1)
      return z0 < z1;
    return yaw0 < yaw1;
  }
};

// 用于unordered_map
struct NodeHash {
  size_t operator()(const PathNode &node) const {
    // (x,y) max_range: [-100, 100], signed 10 bit
    // z max_range: [-5, 5], signed 7 bit
    // yaw max_range: [-12, 12], signed 5 bit
    // key = x  y   z   yaw
    // bit =  31-22   21-12   11-5    4-0
    int x0 = (int)(node.position.x() / 0.1) & 0x3FF;
    x0 <<= 22;
    int y0 = (int)(node.position.y() / 0.1) & 0x3FF;
    y0 <<= 12;
    int z0 = (int)(node.position.z() / 0.1) & 0x7F;
    z0 <<= 5;

    int yaw0 = (int)(kDiv * node.yaw / M_PI) & 0x1F;
    yaw0 <<= 0;

    int key = x0 | y0 | z0 | yaw0;
    return hash<int>()(key);
  }
};

class Traj {
public:
  Eigen::Vector3f pos;
  Eigen::Vector3f vel;
  Eigen::Vector3f acc;
  float yaw;
  float yaw_rate;
};

class Hastar {
private:
  bool trajectory_generate();

public:
  float tau = 0.2;
  float traj_sample = 0.05;
  vector<Traj> traj;
  vector<PathNode> path;
  bool search_path(const octomap::OcTree *ocmap, const Eigen::Vector3f &start_p,
                   const Eigen::Vector3f &end_p, const float &yaw);
  float calc_h_score(const Eigen::Vector3f &start_p,
                     const Eigen::Vector3f &end_p);
  bool is_path_valid(const octomap::OcTree *ocmap,
                     const Eigen::Vector3f &cur_pos,
                     const Eigen::Vector3f &next_pos);
};

#endif
