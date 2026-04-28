#!/usr/bin/env python3
"""
Single camera ROS2 image publisher for Jetson.

- Captures from /dev/videoN via GStreamer (UYVY -> BGRx)
- Converts BGRx to BGR
- Publishes sensor_msgs/Image on a ROS2 topic

Usage:
  python3 camera_single_topic.py
  python3 camera_single_topic.py --cam 3 --topic /cam0/image_raw --pub-fps 10
"""

import argparse
from array import array
import threading
import time

import cv2
import rclpy
from rclpy.node import Node
from rclpy.qos import HistoryPolicy
from rclpy.qos import QoSProfile
from rclpy.qos import ReliabilityPolicy
from sensor_msgs.msg import Image



DEFAULT_WIDTH = 1920
DEFAULT_HEIGHT = 1080
DEFAULT_CAPTURE_FPS = 30


def build_pipeline(dev_index: int, width: int, height: int, fps: int) -> str:
    return (
        f"v4l2src device=/dev/video{dev_index} io-mode=2 ! "
        f"video/x-raw,format=UYVY,width={width},height={height},framerate={fps}/1 ! "
        f"nvvidconv ! video/x-raw,format=BGRx ! "
        f"appsink drop=true max-buffers=1 sync=false"
    )


class SingleCamPublisher(Node):
    def __init__(
        self,
        cam_id: int,
        topic: str,
        frame_id: str,
        width: int,
        height: int,
        capture_fps: int,
        pub_fps: float,
    ):
        super().__init__("single_cam_publisher")

        self.cam_id = cam_id
        self.topic = topic
        self.frame_id = frame_id
        qos = QoSProfile(
            history=HistoryPolicy.KEEP_LAST,
            depth=1,
            reliability=ReliabilityPolicy.BEST_EFFORT,
        )
        self.publisher = self.create_publisher(Image, topic, qos)

        pipeline = build_pipeline(cam_id, width, height, capture_fps)
        self.get_logger().info(f"opening camera pipeline: {pipeline}")
        self.cap = cv2.VideoCapture(pipeline, cv2.CAP_GSTREAMER)
        if not self.cap.isOpened():
            raise RuntimeError(f"failed to open /dev/video{cam_id}")

        self.frame_lock = threading.Lock()
        self.latest_frame = None
        self.have_frame_event = threading.Event()
        self.stop_event = threading.Event()
        self.capture_count = 0
        self.capture_thread = threading.Thread(target=self._capture_loop, daemon=True)
        self.capture_thread.start()

        if not self.have_frame_event.wait(timeout=3.0):
            self.get_logger().warn("camera warm-up timeout: no frame yet")

        timer_period = 1.0 / pub_fps
        self.timer = self.create_timer(timer_period, self.publish_once)
        self.published_count = 0
        self.last_stat_t = time.monotonic()
        self.last_stat_pub = 0
        self.last_stat_cap = 0

        self.get_logger().info(
            f"publishing camera /dev/video{cam_id} to {topic} "
            f"with frame_id {frame_id} at {pub_fps:.2f} Hz"
        )

    def _capture_loop(self):
        while not self.stop_event.is_set() and rclpy.ok():
            ok, frame = self.cap.read()
            if not ok or frame is None:
                time.sleep(0.002)
                continue

            with self.frame_lock:
                self.latest_frame = frame
            self.capture_count += 1
            if not self.have_frame_event.is_set():
                self.have_frame_event.set()

    def publish_once(self):
        with self.frame_lock:
            frame = self.latest_frame
        if frame is None:
            return

        # The intrinsic calibrator consumes standard color images more reliably.
        if len(frame.shape) == 3 and frame.shape[2] == 4:
            frame = cv2.cvtColor(frame, cv2.COLOR_BGRA2BGR)

        stamp = self.get_clock().now().to_msg()
        msg = Image()
        msg.header.stamp = stamp
        msg.header.frame_id = self.frame_id
        msg.height = frame.shape[0]
        msg.width = frame.shape[1]
        msg.encoding = "bgr8"
        msg.is_bigendian = 0
        msg.step = msg.width * 3
        msg.data = array("B", frame.tobytes())

        self.publisher.publish(msg)
        self.published_count += 1

        now = time.monotonic()
        dt = now - self.last_stat_t
        if dt >= 1.0:
            pub_delta = self.published_count - self.last_stat_pub
            cap_delta = self.capture_count - self.last_stat_cap
            self.get_logger().info(
                f"rate: pub={pub_delta / dt:.2f}Hz cap={cap_delta / dt:.2f}Hz"
            )
            self.last_stat_t = now
            self.last_stat_pub = self.published_count
            self.last_stat_cap = self.capture_count

    def destroy_node(self):
        self.stop_event.set()
        if hasattr(self, "capture_thread") and self.capture_thread.is_alive():
            self.capture_thread.join(timeout=1.0)
        if hasattr(self, "cap") and self.cap is not None:
            self.cap.release()
        super().destroy_node()


def parse_args():
    p = argparse.ArgumentParser()
    p.add_argument("--cam", type=int, default=0, help="camera index (default: 0)")
    p.add_argument("--topic", default="/cam0/image_raw", help="ROS topic name")
    p.add_argument(
        "--frame-id",
        default="",
        help="Image header frame_id (default: cam<cam-id>)",
    )
    p.add_argument("--width", type=int, default=DEFAULT_WIDTH, help="frame width")
    p.add_argument("--height", type=int, default=DEFAULT_HEIGHT, help="frame height")
    p.add_argument(
        "--capture-fps",
        type=int,
        default=DEFAULT_CAPTURE_FPS,
        help="camera capture FPS for pipeline",
    )
    p.add_argument(
        "--pub-fps",
        type=float,
        default=10.0,
        help="ROS publish FPS",
    )
    return p.parse_args()


def main():
    args = parse_args()
    rclpy.init()
    frame_id = args.frame_id or f"cam{args.cam}"

    node = SingleCamPublisher(
        cam_id=args.cam,
        topic=args.topic,
        frame_id=frame_id,
        width=args.width,
        height=args.height,
        capture_fps=args.capture_fps,
        pub_fps=args.pub_fps,
    )

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == "__main__":
    main()
