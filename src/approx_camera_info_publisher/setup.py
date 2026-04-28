from glob import glob
import os

from setuptools import setup

package_name = 'approx_camera_info_publisher'

setup(
    name=package_name,
    version='0.1.0',
    packages=[package_name],
    data_files=[
        (
            'share/ament_index/resource_index/packages',
            [os.path.join('resource', package_name)],
        ),
        (os.path.join('share', package_name), ['package.xml', 'README.md']),
        (os.path.join('share', package_name, 'launch'), glob('launch/*.py')),
        (os.path.join('share', package_name, 'config'), glob('config/*.yaml')),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='Codex',
    maintainer_email='maintainer@example.com',
    description='Publish approximate CameraInfo messages to match an Image stream.',
    license='Apache License 2.0',
    entry_points={
        'console_scripts': [
            'approx_camera_info_publisher = approx_camera_info_publisher.approx_camera_info_publisher_node:main',
        ],
    },
)
