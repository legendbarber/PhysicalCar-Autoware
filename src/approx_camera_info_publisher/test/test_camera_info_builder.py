from approx_camera_info_publisher import build_camera_info_msg, validate_parameters
from sensor_msgs.msg import Image


def test_build_camera_info_msg_uses_explicit_intrinsics_and_overrides() -> None:
    config = validate_parameters(
        {
            'image_topic': '/image_raw',
            'camera_info_topic': '/camera_info',
            'frame_id_override': 'camera_optical_frame_override',
            'width_override': 800,
            'height_override': 600,
            'fx': 960.0,
            'fy': 950.0,
            'cx': 400.0,
            'cy': 300.0,
            'distortion_model': 'plumb_bob',
            'd': [0.0, 0.0, 0.0, 0.0, 0.0],
            'r': [],
            'p': [],
            'hfov_deg': 0.0,
            'vfov_deg': 0.0,
            'dfov_deg': 0.0,
        }
    )

    image_msg = Image()
    image_msg.header.stamp.sec = 12
    image_msg.header.stamp.nanosec = 345
    image_msg.header.frame_id = 'camera_optical_frame'
    image_msg.width = 640
    image_msg.height = 480

    camera_info_msg = build_camera_info_msg(image_msg, config)

    assert camera_info_msg.header.stamp.sec == 12
    assert camera_info_msg.header.stamp.nanosec == 345
    assert camera_info_msg.header.frame_id == 'camera_optical_frame_override'
    assert camera_info_msg.width == 800
    assert camera_info_msg.height == 600
    assert camera_info_msg.distortion_model == 'plumb_bob'
    assert list(camera_info_msg.d) == [0.0, 0.0, 0.0, 0.0, 0.0]
    assert list(camera_info_msg.k) == [960.0, 0.0, 400.0, 0.0, 950.0, 300.0, 0.0, 0.0, 1.0]
    assert list(camera_info_msg.r) == [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0]
    assert list(camera_info_msg.p) == [
        960.0,
        0.0,
        400.0,
        0.0,
        0.0,
        950.0,
        300.0,
        0.0,
        0.0,
        0.0,
        1.0,
        0.0,
    ]
