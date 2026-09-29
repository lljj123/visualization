#!/usr/bin/env python3
"""Publish configured waypoints as RViz markers."""

import math

import rospy
from geometry_msgs.msg import Point
from nav_msgs.msg import OccupancyGrid
from visualization_msgs.msg import Marker, MarkerArray


class WaypointMarkerNode:
    """Convert configured waypoints to a latched MarkerArray."""

    def __init__(self):
        rospy.init_node("waypoint_marker", anonymous=False)

        self.frame_id = rospy.get_param("~frame_id", "map")
        self.coordinate_mode = str(
            rospy.get_param("~coordinate_mode", "map")
        ).lower()
        self.map_topic = rospy.get_param("~map_topic", "/map")
        marker_topic = rospy.get_param("~marker_topic", "/waypoints/markers")
        self.waypoints = rospy.get_param("~waypoints", [])
        self.style = rospy.get_param("~style", {})

        if self.coordinate_mode not in ("map", "pixel"):
            raise rospy.ROSInitException(
                "~coordinate_mode must be either 'map' or 'pixel'"
            )
        if not isinstance(self.waypoints, list) or not self.waypoints:
            raise rospy.ROSInitException("~waypoints must be a non-empty list")

        self.publisher = rospy.Publisher(
            marker_topic, MarkerArray, queue_size=1, latch=True
        )
        self.map_signature = None

        if self.coordinate_mode == "map":
            positions = [self._map_position(item) for item in self.waypoints]
            self._publish(positions)
        else:
            self.map_subscriber = rospy.Subscriber(
                self.map_topic,
                OccupancyGrid,
                self._map_callback,
                queue_size=1,
            )
            rospy.loginfo(
                "[%s] Waiting for OccupancyGrid on %s to convert pixel waypoints",
                rospy.get_name(),
                self.map_topic,
            )

    @staticmethod
    def _required_number(item, key):
        if key not in item:
            raise rospy.ROSInitException(
                "Waypoint {!r} is missing required field {!r}".format(item, key)
            )
        try:
            return float(item[key])
        except (TypeError, ValueError):
            raise rospy.ROSInitException(
                "Waypoint field {!r} must be numeric: {!r}".format(key, item[key])
            )

    def _map_position(self, item):
        return (
            self._required_number(item, "x"),
            self._required_number(item, "y"),
        )

    def _pixel_position(self, item, map_info):
        """Convert top-left-origin PGM pixels to the OccupancyGrid map frame."""
        u = self._required_number(item, "u")
        v = self._required_number(item, "v")
        if not (0.0 <= u < map_info.width and 0.0 <= v < map_info.height):
            raise rospy.ROSInitException(
                "Pixel waypoint ({}, {}) is outside map size {}x{}".format(
                    u, v, map_info.width, map_info.height
                )
            )

        local_x = (u + 0.5) * map_info.resolution
        local_y = (map_info.height - v - 0.5) * map_info.resolution

        orientation = map_info.origin.orientation
        sin_yaw = 2.0 * (
            orientation.w * orientation.z + orientation.x * orientation.y
        )
        cos_yaw = 1.0 - 2.0 * (
            orientation.y * orientation.y + orientation.z * orientation.z
        )
        origin_yaw = math.atan2(sin_yaw, cos_yaw)

        origin = map_info.origin.position
        x = origin.x + math.cos(origin_yaw) * local_x - math.sin(origin_yaw) * local_y
        y = origin.y + math.sin(origin_yaw) * local_x + math.cos(origin_yaw) * local_y
        return x, y

    def _map_callback(self, message):
        info = message.info
        signature = (
            info.width,
            info.height,
            info.resolution,
            info.origin.position.x,
            info.origin.position.y,
            info.origin.orientation.x,
            info.origin.orientation.y,
            info.origin.orientation.z,
            info.origin.orientation.w,
        )
        if signature == self.map_signature:
            return

        positions = [self._pixel_position(item, info) for item in self.waypoints]
        self.map_signature = signature
        self._publish(positions)

    def _style_value(self, key, default):
        try:
            return float(self.style.get(key, default))
        except (TypeError, ValueError):
            raise rospy.ROSInitException("Style field {!r} must be numeric".format(key))

    def _new_marker(self, namespace, marker_id, marker_type):
        marker = Marker()
        marker.header.frame_id = self.frame_id
        marker.header.stamp = rospy.Time.now()
        marker.ns = namespace
        marker.id = marker_id
        marker.type = marker_type
        marker.action = Marker.ADD
        marker.pose.orientation.w = 1.0
        marker.lifetime = rospy.Duration(0)
        return marker

    @staticmethod
    def _set_color(marker, red, green, blue, alpha=1.0):
        marker.color.r = red
        marker.color.g = green
        marker.color.b = blue
        marker.color.a = alpha

    def _publish(self, positions):
        output = MarkerArray()

        clear = Marker()
        clear.action = Marker.DELETEALL
        output.markers.append(clear)

        path = self._new_marker("waypoint_path", 0, Marker.LINE_STRIP)
        path.scale.x = self._style_value("line_width", 0.06)
        self._set_color(path, 0.10, 0.75, 1.00, 0.85)

        for index, (item, position) in enumerate(zip(self.waypoints, positions)):
            x, y = position
            marker_id = int(item.get("id", index + 1))
            label = str(item.get("name", "WP{}".format(marker_id)))
            yaw = math.radians(float(item.get("yaw_deg", 0.0)))
            z = float(item.get("z", 0.05))

            point = self._new_marker("waypoint_points", marker_id, Marker.CYLINDER)
            point.pose.position.x = x
            point.pose.position.y = y
            point.pose.position.z = z
            point.scale.x = self._style_value("point_diameter", 0.30)
            point.scale.y = point.scale.x
            point.scale.z = self._style_value("point_height", 0.12)
            self._set_color(point, 1.00, 0.30, 0.08, 0.95)
            output.markers.append(point)

            arrow = self._new_marker("waypoint_headings", marker_id, Marker.ARROW)
            arrow.pose.position.x = x
            arrow.pose.position.y = y
            arrow.pose.position.z = z + point.scale.z * 0.5
            arrow.pose.orientation.z = math.sin(yaw * 0.5)
            arrow.pose.orientation.w = math.cos(yaw * 0.5)
            arrow.scale.x = self._style_value("arrow_length", 0.55)
            arrow.scale.y = self._style_value("arrow_width", 0.10)
            arrow.scale.z = self._style_value("arrow_height", 0.10)
            self._set_color(arrow, 1.00, 0.85, 0.05, 1.00)
            output.markers.append(arrow)

            text = self._new_marker("waypoint_labels", marker_id, Marker.TEXT_VIEW_FACING)
            text.pose.position.x = x
            text.pose.position.y = y
            text.pose.position.z = z + self._style_value("text_z", 0.42)
            text.scale.z = self._style_value("text_height", 0.24)
            text.text = "{}: {}".format(marker_id, label)
            self._set_color(text, 1.00, 1.00, 1.00, 1.00)
            output.markers.append(text)

            path_point = Point(x=x, y=y, z=z + 0.02)
            path.points.append(path_point)

        if len(path.points) >= 2:
            output.markers.append(path)

        self.publisher.publish(output)
        rospy.loginfo(
            "[%s] Published %d waypoints in %s coordinates on %s",
            rospy.get_name(),
            len(positions),
            self.coordinate_mode,
            self.publisher.resolved_name,
        )


def main():
    try:
        WaypointMarkerNode()
        rospy.spin()
    except (rospy.ROSInitException, ValueError) as error:
        rospy.logfatal("Waypoint marker configuration error: %s", error)
        raise SystemExit(1)


if __name__ == "__main__":
    main()
