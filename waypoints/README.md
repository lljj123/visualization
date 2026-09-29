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

- `coordinate_mode: map`：每个航点只需填写 `x`、`y`，单位为米。这是推荐方式。
- `coordinate_mode: pixel`：每个航点只需填写 PGM 中的 `u`、`v`，原点位于图片左上角。
  节点会使用 `/map` 消息中的尺寸、分辨率和原点将像素中心换算到 `map` 坐标系。

航点不包含朝向，也不显示朝向箭头。

## 显示样式

- 第一个航点：蓝色圆柱。
- 最后一个航点：红色圆柱。
- 中间航点：绿色方块。
- 相邻航点：绿色虚线连接；虚线长度和间隔由 `dash_length`、`dash_gap` 调整。
- `id` 只作为 Marker 的内部唯一编号；不填写时会根据列表顺序自动生成。

可以继续在 `waypoints` 列表中添加航点。列表顺序决定起点、中间点和终点，
只填写坐标时不会显示文字。如果需要文字标签，可以额外填写可选的 `name`；
如果手动填写 `id`，每个航点的 `id` 应保持唯一。
