import math

from approx_camera_info_publisher import compute_intrinsics_from_fov, validate_parameters


def test_compute_intrinsics_from_hfov_only() -> None:
    fx, fy, cx, cy = compute_intrinsics_from_fov(width=640, height=480, hfov_deg=90.0)

    assert math.isclose(fx, 320.0, rel_tol=1e-6)
    assert math.isclose(fy, 320.0, rel_tol=1e-6)
    assert math.isclose(cx, 320.0, rel_tol=1e-6)
    assert math.isclose(cy, 240.0, rel_tol=1e-6)


def test_validate_parameters_rejects_missing_intrinsics_and_fov() -> None:
    try:
        validate_parameters(
            {
                'image_topic': '/image_raw',
                'camera_info_topic': '/camera_info',
                'frame_id_override': '',
                'width_override': 0,
                'height_override': 0,
                'fx': 0.0,
                'fy': 0.0,
                'cx': 0.0,
                'cy': 0.0,
                'distortion_model': 'plumb_bob',
                'd': [0.0, 0.0, 0.0, 0.0, 0.0],
                'r': [],
                'p': [],
                'hfov_deg': 0.0,
                'vfov_deg': 0.0,
                'dfov_deg': 0.0,
            }
        )
    except ValueError as exc:
        assert 'Invalid configuration' in str(exc)
    else:
        raise AssertionError('validate_parameters should reject configurations without intrinsics or FOV.')
