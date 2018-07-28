#include <iostream>
#include <sstream>
#include <math.h>
#include <ctime>
#include "segmenter/point_cloud_segmenter.h"
#include "segmenter/segment_processor.h"
#include "segmenter/segmenter_node.h"

segmenter::segmenter()
{

}

segmenter::~segmenter()
{

}

int main(int argc, char** argv)
{
  ros::init(argc, argv, "segmenter_node");
  segmenter segmenter_object;
  segmenter_object.InitRosComm();

  ros::Rate loop_rate(6);

  while(ros::ok())
  {
    ros::spinOnce();

    loop_rate.sleep();
  }
}

void segmenter::InitRosComm()
{
  segmenter_pub = segmenter_nh.advertise<sensor_msgs::PointCloud2>("clusters", 10);
  segmenter_sub = segmenter_nh.subscribe("velodyne_sweep", 100, &segmenter::MessageCallback, this);
  ROS_INFO("Subscriber has been set");
}

void segmenter::PublishSegments()
{
  
}

void segmenter::MessageCallback(const velodyne_puck_msgs::VelodynePuckSweep::ConstPtr& msg)
{
  ROS_INFO("Message Recieved");
  int n_iters = 3;
  int n_lpr = 20;
  int n_segs = 1;
  float seed_thresh = 0.3; //meters
  float dist_thresh = 0.1; //meters

  float th_run = 0.5;
  float th_merge = 1.0;
  int x_max = 30;
  int y_max = 30;
  int n_scanlines = 16;

  pcl::PointCloud<pcl::PointXYZI>::Ptr final_point_cloud;
  std::vector<pcl::PointIndices> clusters;

  std::vector<Vec3> input_cloud;
  std::vector<Vec3> predicted_ground;
  std::vector<Vec3> predicted_clusters;

  PointCloudSegmenter segmenter(x_max, y_max, n_iters, n_lpr, n_segs, seed_thresh, dist_thresh, n_scanlines, th_run, th_merge);
  ParseInput(input_cloud, segmenter, msg);
  segmenter.GroundPlaneFitting(input_cloud);
  predicted_ground = segmenter.GetGroundPoints();

  predicted_clusters = segmenter.ScanLineRun(segmenter.p_all);

  //clusters = GenerateOutput(predicted_clusters);
  SegmentProcessor seg_processor;

  seg_processor.ExtractIndices(predicted_clusters);
  seg_processor.FilterPoints(20);

  bool is_obstacle = seg_processor.FindObstacles();
  //std::cout << "Obstacle detected: " << is_obstacle << std::endl;

  final_point_cloud = seg_processor.GenerateColoredPointCloud();

  segmenter_pub.publish(final_point_cloud);

  //int time2 = clock();
  //std::cout << "Segmentation runtime: " << (time2 - time1) / double(CLOCKS_PER_SEC) * 1000 << " ms\n" << std::endl;
}


void segmenter::ParseInput(std::vector<Vec3>& in, PointCloudSegmenter& seg,  
     			   const velodyne_puck_msgs::VelodynePuckSweep::ConstPtr& cloud)
{
  for (int i = 15; i >= 0; i--)
  {
    Vec3 point;
    velodyne_puck_msgs::VelodynePuckPoint vlp_point;

    for (int j = 0; j < cloud->scans[i].points.size(); j++) 
    {
      vlp_point = cloud->scans[i].points[j];

      if (vlp_point.x < seg.max_x && vlp_point.x > -seg.max_x && vlp_point.y < seg.max_y && vlp_point.y > -seg.max_y) 
      {
        point.x = vlp_point.x; //(vlp_point.x * std::cos(theta)) + (vlp_point.z * std::sin(theta));
        point.y = vlp_point.y;
        point.z = vlp_point.z; //(vlp_point.x * std::sin(theta)) + (vlp_point.z * std::cos(theta));
        point.intensity = vlp_point.intensity;
	point.theta = vlp_point.azimuth;
        //point.label = -1; //TODO!!
        point.scanline = i;

        in.push_back(point); 
      }
    } 
  }
}


// std::vector<pcl::PointIndices> segmenter::GenerateOutput(std::vector<Vec3>& pts)
// {
//   std::vector<pcl::PointIndices> cluster_indices;
//   std::map<int, size_t> label_index_map;
//   int cur_point_index = 0;
//   size_t cluster_count = 0;

//   pcl::PointCloud<pcl::PointXYZI>::Ptr final_point_cloud(new pcl::PointCloud<pcl::PointXYZI>());
//   final_point_cloud->header.frame_id = "/velodyne";
//   final_point_cloud->height = 1;

//   std::vector<Vec3>::iterator v_it;

//   for (v_it = pts.begin(); v_it != pts.end(); v_it++) {
//     pcl::PointXYZI point;
//     point.x = v_it->x;
//     point.y = v_it->y;
//     point.z = v_it->z;
//     point.intensity = v_it->label;

//     final_point_cloud->points.push_back(point);
//     ++final_point_cloud->width;

//     std::map<int, size_t>::iterator m_it = label_index_map.find(v_it->label);
//     if (m_it != label_index_map.end())
//     {
//       cluster_indices[(*m_it).second].indices.push_back(cur_point_index);
//     } else 
//     {
//       cluster_indices.resize(cluster_count + 1);
//       cluster_indices[cluster_count].indices.push_back(cur_point_index);

//       std::pair<int, size_t> label_index_pair;
//       label_index_pair.first = v_it->label;
//       label_index_pair.second = cluster_count;

//       label_index_map.insert(label_index_pair);
//       ++cluster_count;
//     }

//     ++cur_point_index;
//   }

//   std::cout << "Cluster Count: " << cluster_indices.size() << std::endl;
//   segmenter_pub.publish(final_point_cloud);

//   return cluster_indices;
// }
