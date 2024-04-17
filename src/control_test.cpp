#include <ros/ros.h>
#include <geometry_msgs/PoseStamped.h>
#include <mavros_msgs/State.h>
#include <mavros_msgs/SetMode.h>
#include <mavros_msgs/CommandBool.h>
#include <math.h>

class CircleTrajectory {
public:
    CircleTrajectory() {
        state_sub = nh.subscribe<mavros_msgs::State>("/uav0/mavros/state", 10, &CircleTrajectory::state_cb, this);
        local_pos_pub = nh.advertise<geometry_msgs::PoseStamped>("/uav0/mavros/setpoint_position/local", 10);
        arming_client = nh.serviceClient<mavros_msgs::CommandBool>("/uav0/mavros/cmd/arming");
        set_mode_client = nh.serviceClient<mavros_msgs::SetMode>("/uav0/mavros/set_mode");
    }

    void state_cb(const mavros_msgs::State::ConstPtr& msg) {
        current_state = *msg;
    }

    void run() {
        ros::Rate rate(100.0);
        while (ros::ok() && !current_state.connected) {
            ros::spinOnce();
            rate.sleep();
        }

        geometry_msgs::PoseStamped pose;
        pose.header.frame_id = "map";
        pose.pose.position.x = 0;
        pose.pose.position.y = 0;
        pose.pose.position.z = 2;

        ros::Time last_request = ros::Time::now();
        ros::Time start = ros::Time::now();

        //send a few setpoints before starting
        for(int i = 5; ros::ok() && i > 0; --i){
            local_pos_pub.publish(pose);
            ros::spinOnce();
            rate.sleep();
        }

        offb_set_mode.request.custom_mode = "OFFBOARD";
        arm_cmd.request.value = true;

        while (ros::ok()) {
            if (current_state.mode != "OFFBOARD" && (ros::Time::now() - last_request > ros::Duration(5.0))) {
                if (set_mode_client.call(offb_set_mode) && offb_set_mode.response.mode_sent) {
                    ROS_INFO("Offboard mode enabled");
                }
                last_request = ros::Time::now();
            } else {
                if (!current_state.armed && (ros::Time::now() - last_request > ros::Duration(5.0))) {
                    if (arming_client.call(arm_cmd) && arm_cmd.response.success) {
                        ROS_INFO("Vehicle armed");
                    }
                    last_request = ros::Time::now();
                }
            }
            pose.pose.position.x = 2 * cos( 0.6 /2.0 * ros::Time::now().toSec());
            pose.pose.position.y = 2 * sin( 0.6 /2.0 * ros::Time::now().toSec());

            local_pos_pub.publish(pose);

            ros::spinOnce();
            rate.sleep();
        }
    }

private:
    ros::NodeHandle nh;
    ros::Subscriber state_sub;
    ros::Publisher local_pos_pub;
    ros::ServiceClient arming_client;
    ros::ServiceClient set_mode_client;
    mavros_msgs::State current_state;
    mavros_msgs::SetMode offb_set_mode;
    mavros_msgs::CommandBool arm_cmd;
};

int main(int argc, char **argv) {
    ros::init(argc, argv, "circle_trajectory_node");
    CircleTrajectory circle_trajectory;
    circle_trajectory.run();
    return 0;
}