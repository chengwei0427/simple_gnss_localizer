
#include <ros/ros.h>
#include <geometry_msgs/PointStamped.h>
#include <geometry_msgs/PoseStamped.h>
#include <nav_msgs/Odometry.h>
#include <nav_msgs/Path.h>
#include <geometry_msgs/Twist.h>
#include <ros/ros.h>
#include <sensor_msgs/NavSatFix.h>
#include <std_msgs/Bool.h>
#include <tf/transform_broadcaster.h>

#include <iostream>
#include <Eigen/Core>

#include "gnss_localizer/utils.h"

static ros::Publisher pose_publisher;

static ros::Publisher stat_publisher, path_publisher;
static std_msgs::Bool gnss_stat_msg;

static geometry_msgs::PoseStamped _prev_pose;
static geometry_msgs::Quaternion _quat;
nav_msgs::Path nav_path_;

static double yaw;
// true if position history is long enough to compute orientation
static bool _orientation_ready = false;

Eigen::Vector3d init_lla_; // The initial reference gps point.
static bool _first_fix_gnss = false;


bool _save_origin = false;
bool _load_origin = false;
double ori_lat, ori_lon, ori_alt;

static void GNSSCallback(const sensor_msgs::NavSatFixConstPtr &msg)
{
  // Check the gps_status.
  if (msg->status.status == 2 || msg->status.status == 0)
  {
  }
  else
  {
    ROS_WARN("[GpsCallBack]: Bad gps message!");
    return;
  }

  if (!_first_fix_gnss)
  {
    init_lla_ = Eigen::Vector3d(msg->latitude,
                                msg->longitude,
                                msg->altitude);
    _first_fix_gnss = true;
    if (_save_origin)
    {
      //  TODO: save origin
      ROS_WARN("ori gnss: %.8f,%.8f,%.8f", init_lla_[0], init_lla_[1], init_lla_[2]);
    }
    ROS_WARN("first gnss: %.8f,%.8f,%.8f", init_lla_[0], init_lla_[1], init_lla_[2]);
  }
  if (_load_origin)
  {
    init_lla_ = Eigen::Vector3d(ori_lat, ori_lon, ori_alt);
  }

  static tf::TransformBroadcaster pose_broadcaster;
  tf::Transform pose_transform;
  tf::Quaternion pose_q;

  Eigen::Vector3d enu;
  ImuGpsLocalization::ConvertLLAToENU(init_lla_, Eigen::Vector3d(msg->latitude, msg->longitude, msg->altitude), &enu);

  geometry_msgs::PoseStamped pose;
  pose.header = msg->header;
  // pose.header.stamp = ros::Time::now();
  pose.header.frame_id = "map";
  pose.pose.position.x = enu(0);
  pose.pose.position.y = enu(1);
  pose.pose.position.z = enu(2);

  // set gnss_stat
  if (pose.pose.position.x == 0.0 || pose.pose.position.y == 0.0 || pose.pose.position.z == 0.0)
  {
    gnss_stat_msg.data = false;
  }
  else
  {
    gnss_stat_msg.data = true;
  }

  double distance = sqrt(pow(pose.pose.position.y - _prev_pose.pose.position.y, 2) +
                         pow(pose.pose.position.x - _prev_pose.pose.position.x, 2));
  std::cout << "distance : " << distance << std::endl;

  if (distance > 0.2)
  {
    yaw = atan2(pose.pose.position.y - _prev_pose.pose.position.y, pose.pose.position.x - _prev_pose.pose.position.x);
    _quat = tf::createQuaternionMsgFromYaw(yaw);
    _prev_pose = pose;
    _orientation_ready = true;
  }

  if (_orientation_ready)
  {
    //  pub pose
    pose.pose.orientation = _quat;
    pose_publisher.publish(pose);
    stat_publisher.publish(gnss_stat_msg);

    // publish the path
    nav_path_.header = pose.header;
    nav_path_.poses.push_back(pose);
    path_publisher.publish(nav_path_);

    //  pub tf
    static tf::TransformBroadcaster br;
    tf::Transform transform;
    tf::Quaternion q;
    transform.setOrigin(tf::Vector3(pose.pose.position.x, pose.pose.position.y, pose.pose.position.z));
    q.setRPY(0, 0, yaw);
    transform.setRotation(q);
    br.sendTransform(tf::StampedTransform(transform, msg->header.stamp, "map", "gps"));

  }
}



int main(int argc, char **argv)
{
  ros::init(argc, argv, "gnss_localizer");
  ros::NodeHandle nh;
  ros::NodeHandle pnh("~");
  pnh.getParam("save_origin", _save_origin);
  pnh.getParam("load_origin", _load_origin);
  pnh.getParam("ori_lat", ori_lat);
  pnh.getParam("ori_lon", ori_lon);
  pnh.getParam("ori_alt", ori_alt);

  ROS_WARN("load ori: %.8f,%.8f,%.8f", ori_lat, ori_lon, ori_alt);

  pose_publisher = nh.advertise<geometry_msgs::PoseStamped>("gnss_pose", 1000);
  stat_publisher = nh.advertise<std_msgs::Bool>("/gnss_stat", 1000);
  path_publisher = nh.advertise<nav_msgs::Path>("nav_path", 10);

  ros::Subscriber gnss_pose_subscriber = nh.subscribe("/fix", 100, GNSSCallback);

  ros::spin();
  return 0;
}
