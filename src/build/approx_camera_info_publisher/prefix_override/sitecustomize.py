import sys
if sys.prefix == '/usr':
    sys.real_prefix = sys.prefix
    sys.prefix = sys.exec_prefix = '/home/futuredrive/yhs_control2_ws/src/install/approx_camera_info_publisher'
