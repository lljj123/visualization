#!/usr/bin/env python3
"""Publish the robot trajectory collected during the current node run."""

import rospy
import tf2_ros
from geometry_msgs.msg import PoseStamped
from nav_msgs.msg import Path


class CurrentPathPublisher:
    """Sample a TF pose and append sufficiently separated poses to a Path."""

    def __init__(self):
        rospy.init_node("current_path_publisher", anonymous=False)

        self.parent_frame = rospy.get_param("~parent_frame", "map")
        self.child_frame = rospy.get_param("~child_frame", "base_link")
        self.path_topic = rospy.get_param("~path_topic", "/current_path")
        self.lookup_rate = float(rospy.get_param("~lookup_rate", 30.0))
        self.min_distance = float(rospy.get_param("~min_distance", 0.05))
        self.tf_timeout = float(rospy.get_param("~tf_timeout", 0.1))

        if self.lookup_rate <= 0.0:
            raise rospy.ROSInitException("~lookup_rate must be positive")
        if self.min_distance < 0.0:
            raise rospy.ROSInitException("~min_distance cannot be negative")
        if self.tf_timeout < 0.0:
            raise rospy.ROSInitException("~tf_timeout cannot be negative")

        self.min_distance_squared = self.min_distance ** 2
        self.path = Path()
        self.path.header.frame_id = self.parent_frame
        self.last_pose = None

        self.publisher = rospy.Publisher(
            self.path_topic, Path, queue_size=1, latch=True
        )
        self.tf_buffer = tf2_ros.Buffer(cache_time=rospy.Duration(10.0))
        self.tf_listener = tf2_ros.TransformListener(self.tf_buffer)

        # An empty, latched Path clears any trajectory left in an existing RViz display.
        self.path.header.stamp = rospy.Time.now()
        self.publisher.publish(self.path)

        rospy.loginfo(
            "[%s] Recording %s -> %s on %s at %.1f Hz (distance > %.3f m)",
            rospy.get_name(),
            self.parent_frame,
            self.child_frame,
            self.publisher.resolved_name,
            self.lookup_rate,
            self.min_distance,
        )

    def _lookup_pose(self):
        transform = self.tf_buffer.lookup_transform(
            self.parent_frame,
            self.child_frame,
            rospy.Time(0),
            rospy.Duration(self.tf_timeout),
        )

        pose = PoseStamped()
        pose.header.frame_id = self.parent_frame
        pose.header.stamp = transform.header.stamp
        pose.pose.position.x = transform.transform.translation.x
        pose.pose.position.y = transform.transform.translation.y
        pose.pose.position.z = transform.transform.translation.z
        pose.pose.orientation.x = transform.transform.rotation.x
        pose.pose.orientation.y = transform.transform.rotation.y
        pose.pose.orientation.z = transform.transform.rotation.z
        pose.pose.orientation.w = transform.transform.rotation.w
        return pose

    def _moved_enough(self, pose):
        if self.last_pose is None:
            return True

        delta_x = pose.pose.position.x - self.last_pose.pose.position.x
        delta_y = pose.pose.position.y - self.last_pose.pose.position.y
        delta_z = pose.pose.position.z - self.last_pose.pose.position.z
        distance_squared = delta_x ** 2 + delta_y ** 2 + delta_z ** 2
        return distance_squared > self.min_distance_squared

    def _append_pose(self, pose):
        self.path.header.stamp = pose.header.stamp
        self.path.header.frame_id = self.parent_frame
        self.path.poses.append(pose)
        self.last_pose = pose
        self.publisher.publish(self.path)

    def run(self):
        rate = rospy.Rate(self.lookup_rate)
        while not rospy.is_shutdown():
            try:
                pose = self._lookup_pose()
                if self._moved_enough(pose):
                    self._append_pose(pose)
            except (
                tf2_ros.LookupException,
                tf2_ros.ConnectivityException,
                tf2_ros.ExtrapolationException,
            ) as error:
                rospy.logwarn_throttle(
                    5.0,
                    "[%s] Waiting for TF %s -> %s: %s"
                    % (
                        rospy.get_name(),
                        self.parent_frame,
                        self.child_frame,
                        error,
                    ),
                )
            rate.sleep()


def main():
    try:
        node = CurrentPathPublisher()
        node.run()
    except (rospy.ROSInitException, ValueError) as error:
        rospy.logfatal("Current path configuration error: %s", error)
        raise SystemExit(1)
    except rospy.ROSInterruptException:
        pass


if __name__ == "__main__":
    main()
