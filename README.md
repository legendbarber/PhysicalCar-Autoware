### prepare.sh
: autoware모듈 실행 전 간단한 환경세팅

### run2.sh
: 참고 실행 스크립트

###
/home/futuredrive/astudio/furive-kit-backend/client-deploy/.caches/[furive-planning]AW-Planning_v1.0.0_ws/install/autoware_launch/share/autoware_launch/config/planning/scenario_planning/common/motion_velocity_smoother/motion_velocity_smoother.param.yaml 
/home/futuredrive/yhs_control2_ws/client-deploy/.caches/[furive-control]AW-Control_v1.0.0_ws/install/vehicle_cmd_gate/share/vehicle_cmd_gate/config/vehicle_cmd_gate.param.yaml
/home/futuredrive/yhs_control2_ws/client-deploy/.caches/[furive-control]AW-Control_v1.0.0_ws/src/universe/autoware.universe/control/vehicle_cmd_gate/config/vehicle_cmd_gate.param.yaml
/home/futuredrive/yhs_control2_ws/client-deploy/.caches/[furive-planning]AW-Planning_v1.0.0_ws/install/autoware_launch/share/autoware_launch/config/planning/scenario_planning/common/motion_velocity_smoother/motion_velocity_smoother.param.yaml
/home/futuredrive/yhs_control2_ws/client-deploy/.caches/[furive-planning]AW-Planning_v1.0.0_ws/src/launcher/autoware_launch/autoware_launch/config/planning/scenario_planning/common/motion_velocity_smoother/motion_velocity_smoother.param.yaml
-> 여기서 max_velocity 상한을 걸어놧음(빌드까지 다함)


### 기어값은 여기
/home/futuredrive/yhs_control2_ws/src/yhs_autoware_vehicle_status_bridge/params/yhs_autoware_vehicle_status_bridge.param.yaml  
/home/futuredrive/yhs_control2_ws/src/yhs_can_control/params/tod_autoware_bridge.yaml

### 기어값 문서
TOD raw	의미	Autoware 값
0	disable	    ONE = 0
1	P	        PARK = 22
2	R	        REVERSE = 20
3	N	        NEUTRAL = 1
4	D	        DRIVE = 2

### 자율주행에서 후진 되게할려면 
    force_drive_gear: false로(/home/futuredrive/yhs_control2_ws/src/yhs_can_control/params/tod_autoware_bridge.yaml)


### 조향 부호가 좀 이상하면
/home/futuredrive/yhs_control2_ws/src/yhs_can_control/src/tod_autoware_bridge_node.cpp
tod_msg.data[2] = static_cast<double>(msg->lateral.steering_tire_angle) * kRadToDeg;
tod_msg.data[2] = -static_cast<double>(msg->lateral.steering_tire_angle) * kRadToDeg;
/
home/futuredrive/yhs_control2_ws/src/yhs_autoware_vehicle_status_bridge/src/yhs_autoware_vehicle_status_bridge_node.cpp
out.steering_tire_angle = static_cast<float>(msg.ctrl_fb.ctrl_fb_steering * kDegToRad);
out.steering_tire_angle = static_cast<float>(-msg.ctrl_fb.ctrl_fb_steering * kDegToRad);

/home/futuredrive/yhs_control2_ws/src/yhs_can_control/src/yhs_can_control_node.cpp
publish_odom(msg.ctrl_fb_velocity, msg.ctrl_fb_steering/180*3.1415);
publish_odom(msg.ctrl_fb_velocity, msg.ctrl_fb_steering/180*3.1415);
