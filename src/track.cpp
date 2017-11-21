#include "ros/ros.h"
#include "std_msgs/String.h"
#include <visualization_msgs/Marker.h>
#include "geometry_msgs/Twist.h"
#include <map>
#include <tf/tf.h>
#include <tf/transform_listener.h>
#include <visualization_msgs/MarkerArray.h>
#include "Eigen/Dense"
#include "tf/transform_datatypes.h"
#include "Eigen/Core"
#include "Eigen/Geometry"
#include <iostream>
#include <fstream>

bool markerSeen = false;
float THRESHOLD = 1;

visualization_msgs::Marker current_vis_msg;
visualization_msgs::MarkerArray marker_array_msg;

void vis_cb(const visualization_msgs::Marker::ConstPtr& msg) {
    current_vis_msg = *msg;
    markerSeen = true;
}

int main(int argc, char **argv){

    ros::init(argc, argv, "ar_tag_tracker");
	ros::NodeHandle n;
    tf::TransformListener tf_l;

    //subscriber for the /visualization_marker
    ros::Subscriber vis_sub = n.subscribe("/visualization_marker", 1, vis_cb);
    //publisher for the markers on the map
    ros::Publisher marker_pub = n.advertise<visualization_msgs::MarkerArray>("map_markers", 10);

    //HOLDS ID->Pose pairs
    //Internal Data Structure
    std::map<int, geometry_msgs::PoseStamped> pose_map;
    std::map<int, geometry_msgs::PoseStamped>::iterator it;

   	geometry_msgs::PoseStamped stampedPose;

    //output stampedPose
    geometry_msgs::PoseStamped tag_wresp_map;

    while(ros::ok()) {
    	ros::spinOnce();
    	if(markerSeen) {
    		//Iterator stores the location of the id within the internal map
	        it = pose_map.find(current_vis_msg.id);
	        //If the tag is NEW
	        if(it == pose_map.end()) {
	          //first visualize it it in to the rviz
	          marker_array_msg.markers.push_back(current_vis_msg);
	      	  //Transfer information into a StampedPose
	  				stampedPose.header = current_vis_msg.header;
	  				stampedPose.pose = current_vis_msg.pose;
	        	try {
	        		tf_l.waitForTransform("/map",
	                              stampedPose.header.frame_id, ros::Time(0), ros::Duration(1));
	        		//Performs transformation and stores it in the third parameter
	        		tf_l.transformPose("/map", stampedPose, tag_wresp_map);
	        		//JAMIN TAKES tag_wresp_map and annotates the map with its
		        }
			    catch (tf::TransformException ex) {
			        ROS_ERROR("TransformException");
			    }
	          ROS_INFO("Current tag is at x : %f, y : %f, z : %f", tag_wresp_map.pose.position.x, tag_wresp_map.pose.position.y
	          , tag_wresp_map.pose.position.z);
	          pose_map.insert(it, std::pair<int, geometry_msgs::PoseStamped>(current_vis_msg.id , tag_wresp_map));
		        }
		        it = pose_map.find(current_vis_msg.id);
		        if(it == pose_map.end()) {
					stampedPose.header = current_vis_msg.header;
					stampedPose.pose = current_vis_msg.pose;
		        	try {
		        		tf_l.waitForTransform("/map", stampedPose.header.frame_id, ros::Time(0), ros::Duration(1));
		        		tf_l.transformPose("/map", stampedPose, tag_wresp_map);
				    }
				    catch (tf::TransformException ex) {
				        ROS_ERROR("TransformException");
				    }
		          	//ROS_INFO("Current tag is at x : %f, y : %f, z : %f", tag_wresp_map.pose.position.x, tag_wresp_map.pose.position.y, tag_wresp_map.pose.position.z);
		          	pose_map.insert(it, std::pair<int, geometry_msgs::PoseStamped>(current_vis_msg.id , tag_wresp_map));
		        }		    	
		        else {
			    	Eigen::Quaternionf pQuat;
	 				pQuat.x() = current_vis_msg.pose.position.x;
	 				pQuat.y() = current_vis_msg.pose.position.y;
	 				pQuat.z() = current_vis_msg.pose.position.z;
	 				pQuat.w() = 0;
	 
	 
	 				Eigen::Quaternionf tempQuat;
	 				tempQuat.x() = current_vis_msg.pose.orientation.x;
	 				tempQuat.y() = current_vis_msg.pose.orientation.y;
	 				tempQuat.z() = current_vis_msg.pose.orientation.z;
	 				tempQuat.w() = current_vis_msg.pose.orientation.w;
	 
	 				//This quaternion stores the xyz of the arTag with respect to the robot.
	 				//Got to this quaternion through basic computation from the cur_vis_msg
	 				Eigen::Quaternionf arPose_wrt_robot = tempQuat.inverse();
	 				arPose_wrt_robot*= pQuat;
	 				arPose_wrt_robot*= tempQuat;
	 
	 				geometry_msgs::Pose curPose = pose_map.at(current_vis_msg.id).pose;
	 				Eigen::Quaternionf pARTag;
	 				pARTag.x() = curPose.position.x;
	 				pARTag.y() = curPose.position.y;
	 				pARTag.z() = curPose.position.z;
	 				pARTag.w() = 0;
	 
	 				Eigen::Quaternionf tempQuat2;
	 				tempQuat2.x() = curPose.orientation.x;
					tempQuat2.y() = curPose.orientation.y;
	 				tempQuat2.z() = curPose.orientation.z;
	 				tempQuat2.w() = curPose.orientation.w;
	 
	 				//This quaternion stores the xyz of the arTag with respect to the map.
	 				//Got to this quaternion through basic computation from the Pose stored in the internal Map
	 				Eigen::Quaternionf arPose_wrt_map = tempQuat2.inverse();
	 				arPose_wrt_map*= pARTag;
	 				arPose_wrt_map*= tempQuat2;
	 

	 		    	geometry_msgs::Pose outputPose;
			    	outputPose.position.x = arPose_wrt_robot.x - arPose_wrt_map.x;
	 		    	outputPose.position.y = arPose_wrt_robot.y - arPose_wrt_map.y;
	 		    	outputPose.position.z = arPose_wrt_robot.z - arPose_wrt_map.z;
	 
	 		    	outputPose.orientation.x = 0;
	 		    	outputPose.orientation.y = 0;
	 		    	outputPose.orientation.z = 0;
	 		    	outputPose.orientation.w = 1;
	 		    
	 		    	ROS_INFO("The robot is at : %f, y : %f, z : %f", arPose_wrt_robot.x() - arPose_wrt_map.x(), arPose_wrt_robot.y() - arPose_wrt_map.y(), arPose_wrt_robot.z() - arPose_wrt_map.z());	
				}
	    	markerSeen = false;
        	marker_pub.publish(marker_array_msg);
	    }
	}
}