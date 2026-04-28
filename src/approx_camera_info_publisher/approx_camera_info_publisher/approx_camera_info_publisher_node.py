from __future__ import annotations

from array import array
from dataclasses import dataclass
import math
from typing import Any, Mapping, Sequence

import rclpy
from rclpy.logging import get_logger
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from sensor_msgs.msg import CameraInfo, Image

NODE_NAME = 'approx_camera_info_publisher'
DEFAULT_DISTORTION_MODEL = 'plumb_bob'
DEFAULT_D = (0.0, 0.0, 0.0, 0.0, 0.0)
DEFAULT_R = (
    1.0,
    0.0,
    0.0,
    0.0,
    1.0,
    0.0,
    0.0,
    0.0,
    1.0,
)


@dataclass(frozen=True)
class ValidatedParameters:
    image_topic: str
    camera_info_topic: str
    frame_id_override: str
    width_override: int
    height_override: int
    mode: str
    fx: float
    fy: float
    cx: float
    cy: float
    distortion_model: str
    d: tuple[float, ...]
    r: tuple[float, ...]
    p: tuple[float, ...]
    hfov_deg: float
    vfov_deg: float
    dfov_deg: float
    fov_source: str | None


class ApproxCameraInfoPublisher(Node):
    def __init__(self) -> None:
        super().__init__(NODE_NAME)

        self.declare_parameters(
            namespace='',
            parameters=[
                ('image_topic', ''),
                ('camera_info_topic', ''),
                ('frame_id_override', ''),
                ('width_override', 0),
                ('height_override', 0),
                ('fx', 0.0),
                ('fy', 0.0),
                ('cx', 0.0),
                ('cy', 0.0),
                ('distortion_model', DEFAULT_DISTORTION_MODEL),
                ('d', list(DEFAULT_D)),
                ('r', []),
                ('p', []),
                ('hfov_deg', 0.0),
                ('vfov_deg', 0.0),
                ('dfov_deg', 0.0),
            ],
        )

        raw_parameters = {
            'image_topic': self.get_parameter('image_topic').value,
            'camera_info_topic': self.get_parameter('camera_info_topic').value,
            'frame_id_override': self.get_parameter('frame_id_override').value,
            'width_override': self.get_parameter('width_override').value,
            'height_override': self.get_parameter('height_override').value,
            'fx': self.get_parameter('fx').value,
            'fy': self.get_parameter('fy').value,
            'cx': self.get_parameter('cx').value,
            'cy': self.get_parameter('cy').value,
            'distortion_model': self.get_parameter('distortion_model').value,
            'd': self.get_parameter('d').value,
            'r': self.get_parameter('r').value,
            'p': self.get_parameter('p').value,
            'hfov_deg': self.get_parameter('hfov_deg').value,
            'vfov_deg': self.get_parameter('vfov_deg').value,
            'dfov_deg': self.get_parameter('dfov_deg').value,
        }

        self._config = validate_parameters(raw_parameters)
        self._runtime_error_reported = False

        self._camera_info_publisher = self.create_publisher(
            CameraInfo,
            self._config.camera_info_topic,
            qos_profile_sensor_data,
        )
        self._image_subscription = self.create_subscription(
            Image,
            self._config.image_topic,
            self._image_callback,
            qos_profile_sensor_data,
        )

        self._log_startup_summary()

    def _log_startup_summary(self) -> None:
        frame_id_description = (
            f'override={self._config.frame_id_override}'
            if self._config.frame_id_override
            else 'from incoming Image header'
        )

        if self._config.width_override > 0 and self._config.height_override > 0:
            dimension_description = (
                f'override={self._config.width_override}x{self._config.height_override}'
            )
        elif self._config.width_override > 0:
            dimension_description = (
                f'width_override={self._config.width_override}, height=from incoming Image'
            )
        elif self._config.height_override > 0:
            dimension_description = (
                f'width=from incoming Image, height_override={self._config.height_override}'
            )
        else:
            dimension_description = 'from incoming Image'

        if self._config.mode == 'explicit':
            mode_description = (
                'mode=explicit '
                f'(fx={self._config.fx:.3f}, fy={self._config.fy:.3f}, '
                f'cx={self._config.cx:.3f}, cy={self._config.cy:.3f})'
            )
        else:
            mode_description = (
                f'mode=fov_approximation ({self._config.fov_source}, '
                f'hfov_deg={self._config.hfov_deg:.3f}, '
                f'vfov_deg={self._config.vfov_deg:.3f}, '
                f'dfov_deg={self._config.dfov_deg:.3f})'
            )

        self.get_logger().info(
            'Starting approx_camera_info_publisher with '
            f'image_topic={self._config.image_topic}, '
            f'camera_info_topic={self._config.camera_info_topic}, '
            f'frame_id={frame_id_description}, '
            f'dimensions={dimension_description}, '
            f'{mode_description}, '
            f'distortion_model={self._config.distortion_model}'
        )

    def _image_callback(self, image_msg: Image) -> None:
        try:
            camera_info_msg = build_camera_info_msg(image_msg, self._config)
        except ValueError as exc:
            if not self._runtime_error_reported:
                self._runtime_error_reported = True
                self.get_logger().error(
                    f'Cannot publish CameraInfo due to invalid runtime input: {exc}. '
                    'Shutting down the node.'
                )
                self.destroy_subscription(self._image_subscription)
                rclpy.shutdown()
            return

        self._camera_info_publisher.publish(camera_info_msg)


def validate_parameters(raw_parameters: Mapping[str, Any]) -> ValidatedParameters:
    image_topic = _coerce_non_empty_string('image_topic', raw_parameters.get('image_topic', ''))
    camera_info_topic = _coerce_non_empty_string(
        'camera_info_topic', raw_parameters.get('camera_info_topic', '')
    )
    frame_id_override = _coerce_string(
        'frame_id_override', raw_parameters.get('frame_id_override', '')
    ).strip()
    width_override = _coerce_int('width_override', raw_parameters.get('width_override', 0))
    height_override = _coerce_int('height_override', raw_parameters.get('height_override', 0))

    if width_override < 0:
        raise ValueError('width_override must be greater than or equal to 0.')
    if height_override < 0:
        raise ValueError('height_override must be greater than or equal to 0.')

    fx = _coerce_float('fx', raw_parameters.get('fx', 0.0))
    fy = _coerce_float('fy', raw_parameters.get('fy', 0.0))
    cx = _coerce_float('cx', raw_parameters.get('cx', 0.0))
    cy = _coerce_float('cy', raw_parameters.get('cy', 0.0))
    hfov_deg = _coerce_float('hfov_deg', raw_parameters.get('hfov_deg', 0.0))
    vfov_deg = _coerce_float('vfov_deg', raw_parameters.get('vfov_deg', 0.0))
    dfov_deg = _coerce_float('dfov_deg', raw_parameters.get('dfov_deg', 0.0))

    distortion_model = _coerce_non_empty_string(
        'distortion_model', raw_parameters.get('distortion_model', DEFAULT_DISTORTION_MODEL)
    )
    d = _coerce_float_sequence('d', raw_parameters.get('d', list(DEFAULT_D)))
    r = _coerce_float_sequence('r', raw_parameters.get('r', []))
    p = _coerce_float_sequence('p', raw_parameters.get('p', []))

    if len(r) not in (0, 9):
        raise ValueError('r must contain exactly 9 values when provided.')
    if len(p) not in (0, 12):
        raise ValueError('p must contain exactly 12 values when provided.')

    effective_d = DEFAULT_D if len(d) == 0 else d

    explicit_values = (fx, fy, cx, cy)
    has_any_explicit = any(value != 0.0 for value in explicit_values)
    has_full_explicit = fx > 0.0 and fy > 0.0 and not (cx == 0.0 and cy == 0.0)

    if has_any_explicit and not has_full_explicit:
        raise ValueError(
            'Explicit intrinsics are only partially configured. Provide fx, fy, cx, and cy '
            'together, or leave them at defaults and use hfov_deg, vfov_deg, or dfov_deg.'
        )

    fov_values = {
        'hfov_deg': hfov_deg,
        'vfov_deg': vfov_deg,
        'dfov_deg': dfov_deg,
    }
    provided_fov_names = []
    for name, value in fov_values.items():
        if value != 0.0:
            _validate_fov_degrees(name, value)
        if value > 0.0:
            provided_fov_names.append(name)

    if not has_full_explicit and not provided_fov_names:
        raise ValueError(
            'Invalid configuration: provide either explicit intrinsics (fx, fy, cx, cy) '
            'or at least one of hfov_deg, vfov_deg, or dfov_deg.'
        )

    if has_full_explicit:
        if fx <= 0.0 or fy <= 0.0:
            raise ValueError('fx and fy must be greater than 0.0.')
        mode = 'explicit'
        fov_source = None
    else:
        mode = 'fov'
        fov_source = _select_fov_source(hfov_deg, vfov_deg, dfov_deg)

    return ValidatedParameters(
        image_topic=image_topic,
        camera_info_topic=camera_info_topic,
        frame_id_override=frame_id_override,
        width_override=width_override,
        height_override=height_override,
        mode=mode,
        fx=fx,
        fy=fy,
        cx=cx,
        cy=cy,
        distortion_model=distortion_model,
        d=effective_d,
        r=r,
        p=p,
        hfov_deg=hfov_deg,
        vfov_deg=vfov_deg,
        dfov_deg=dfov_deg,
        fov_source=fov_source,
    )


def resolve_dimensions_from_image_or_override(
    image_msg: Image,
    width_override: int,
    height_override: int,
) -> tuple[int, int]:
    width = width_override if width_override > 0 else int(image_msg.width)
    height = height_override if height_override > 0 else int(image_msg.height)

    if width <= 0:
        raise ValueError('Resolved width must be greater than 0.')
    if height <= 0:
        raise ValueError('Resolved height must be greater than 0.')

    return width, height


def compute_intrinsics_from_fov(
    width: int,
    height: int,
    hfov_deg: float = 0.0,
    vfov_deg: float = 0.0,
    dfov_deg: float = 0.0,
) -> tuple[float, float, float, float]:
    if width <= 0 or height <= 0:
        raise ValueError('width and height must both be greater than 0.')

    for name, value in (
        ('hfov_deg', hfov_deg),
        ('vfov_deg', vfov_deg),
        ('dfov_deg', dfov_deg),
    ):
        if value != 0.0:
            _validate_fov_degrees(name, value)

    if hfov_deg > 0.0 and vfov_deg > 0.0:
        fx = width / (2.0 * math.tan(math.radians(hfov_deg) / 2.0))
        fy = height / (2.0 * math.tan(math.radians(vfov_deg) / 2.0))
    elif hfov_deg > 0.0:
        fx = width / (2.0 * math.tan(math.radians(hfov_deg) / 2.0))
        fy = fx
    elif vfov_deg > 0.0:
        fy = height / (2.0 * math.tan(math.radians(vfov_deg) / 2.0))
        fx = fy
    elif dfov_deg > 0.0:
        focal_length = math.hypot(width, height) / (2.0 * math.tan(math.radians(dfov_deg) / 2.0))
        fx = focal_length
        fy = focal_length
    else:
        raise ValueError(
            'At least one of hfov_deg, vfov_deg, or dfov_deg must be greater than 0.0.'
        )

    if fx <= 0.0 or fy <= 0.0:
        raise ValueError('Computed fx and fy must both be greater than 0.0.')

    cx = width / 2.0
    cy = height / 2.0
    return fx, fy, cx, cy


def build_camera_info_msg(image_msg: Image, config: ValidatedParameters) -> CameraInfo:
    width, height = resolve_dimensions_from_image_or_override(
        image_msg,
        config.width_override,
        config.height_override,
    )

    if config.mode == 'explicit':
        fx = config.fx
        fy = config.fy
        cx = config.cx
        cy = config.cy
    else:
        fx, fy, cx, cy = compute_intrinsics_from_fov(
            width=width,
            height=height,
            hfov_deg=config.hfov_deg,
            vfov_deg=config.vfov_deg,
            dfov_deg=config.dfov_deg,
        )

    if fx <= 0.0 or fy <= 0.0:
        raise ValueError('fx and fy must both be greater than 0.0.')

    frame_id = config.frame_id_override if config.frame_id_override else image_msg.header.frame_id

    camera_info_msg = CameraInfo()
    camera_info_msg.header.stamp = image_msg.header.stamp
    camera_info_msg.header.frame_id = frame_id
    camera_info_msg.width = width
    camera_info_msg.height = height
    camera_info_msg.distortion_model = config.distortion_model
    camera_info_msg.d = list(config.d)
    camera_info_msg.k = [
        fx,
        0.0,
        cx,
        0.0,
        fy,
        cy,
        0.0,
        0.0,
        1.0,
    ]
    camera_info_msg.r = list(config.r) if config.r else list(DEFAULT_R)
    camera_info_msg.p = list(config.p) if config.p else [
        fx,
        0.0,
        cx,
        0.0,
        0.0,
        fy,
        cy,
        0.0,
        0.0,
        0.0,
        1.0,
        0.0,
    ]
    return camera_info_msg


def _coerce_non_empty_string(name: str, value: Any) -> str:
    text = _coerce_string(name, value).strip()
    if not text:
        raise ValueError(f'{name} must be a non-empty string.')
    return text


def _coerce_string(name: str, value: Any) -> str:
    if not isinstance(value, str):
        raise ValueError(f'{name} must be a string.')
    return value


def _coerce_int(name: str, value: Any) -> int:
    if isinstance(value, bool):
        raise ValueError(f'{name} must be an integer.')
    if not isinstance(value, int):
        raise ValueError(f'{name} must be an integer.')
    return value


def _coerce_float(name: str, value: Any) -> float:
    if isinstance(value, bool):
        raise ValueError(f'{name} must be a floating-point value.')
    try:
        return float(value)
    except (TypeError, ValueError) as exc:
        raise ValueError(f'{name} must be a floating-point value.') from exc


def _coerce_float_sequence(name: str, value: Any) -> tuple[float, ...]:
    if value is None:
        raise ValueError(f'{name} must be a list of floating-point values.')
    if isinstance(value, (str, bytes)):
        raise ValueError(f'{name} must be a list of floating-point values.')

    if isinstance(value, array):
        items = list(value)
    elif isinstance(value, Sequence):
        items = list(value)
    else:
        raise ValueError(f'{name} must be a list of floating-point values.')

    result = []
    for item in items:
        if isinstance(item, bool):
            raise ValueError(f'{name} must contain floating-point values only.')
        try:
            result.append(float(item))
        except (TypeError, ValueError) as exc:
            raise ValueError(f'{name} must contain floating-point values only.') from exc
    return tuple(result)


def _select_fov_source(hfov_deg: float, vfov_deg: float, dfov_deg: float) -> str:
    if hfov_deg > 0.0 and vfov_deg > 0.0:
        return 'hfov+vfov'
    if hfov_deg > 0.0:
        return 'hfov'
    if vfov_deg > 0.0:
        return 'vfov'
    if dfov_deg > 0.0:
        return 'dfov'
    raise ValueError('No valid FOV source is available.')


def _validate_fov_degrees(name: str, value: float) -> None:
    if value < 0.0:
        raise ValueError(f'{name} must be greater than or equal to 0.0 degrees.')
    if value >= 180.0:
        raise ValueError(f'{name} must be less than 180.0 degrees.')


def main(args: list[str] | None = None) -> None:
    rclpy.init(args=args)
    node: ApproxCameraInfoPublisher | None = None

    try:
        node = ApproxCameraInfoPublisher()
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    except Exception as exc:
        get_logger(NODE_NAME).error(str(exc))
        raise SystemExit(1) from exc
    finally:
        if node is not None:
            node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
