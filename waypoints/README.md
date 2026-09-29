# Waypoint visualization

这个 ROS1 Noetic 包把 YAML 中的航点发布为 RViz `MarkerArray`，并与
`map_server` 发布的 `OccupancyGrid` 叠加显示。

## 使用方法

把 `waypoints` 目录放到 catkin 工作区的 `src` 下，然后构建并 source：

```bash
catkin_make
source devel/setup.bash
```

编辑 `config/waypoints.yaml` 后，一条命令加载地图、航点和 RViz：

```bash
roslaunch waypoint_visualization map_waypoints_rviz.launch \
  map_yaml:=/absolute/path/to/map.yaml
```

这里的地图参数必须是与 PGM 配套的 `map.yaml`。RViz 不直接解析裸 PGM；
`map_server` 根据地图 YAML 中的 `image`、`resolution` 和 `origin` 发布 `/map`。

如果 `/map` 已由 SLAM 或其他节点发布，只启动航点节点：

```bash
roslaunch waypoint_visualization waypoints.launch \
  config:=/absolute/path/to/waypoints.yaml
```

RViz 的 Fixed Frame 设为 `map`，Map topic 设为 `/map`，MarkerArray topic
设为 `/waypoints/markers`。

## 坐标模式

- `coordinate_mode: map`：每个航点填写 `x`、`y`，单位为米。这是推荐方式。
- `coordinate_mode: pixel`：每个航点填写 PGM 中的 `u`、`v`，原点位于图片左上角。
  节点会使用 `/map` 消息中的尺寸、分辨率和原点将像素中心换算到 `map` 坐标系。

无论哪种模式，`yaw_deg` 都是相对 `map` 坐标系正 X 轴逆时针旋转的角度。
