# Current path visualization

该 ROS1 Noetic 节点持续查询 `map -> base_link` 的最新 TF，并将当前启动周期内的
机器人位姿累积为 `nav_msgs/Path` 发布到 `/current_path`。

节点只使用实时 TF，不读取 RTAB-Map database，也不订阅 `/rtabmap/mapPath`；节点
重启后轨迹从空列表重新开始，退出时不保存文件。

## 启动

```bash
roslaunch current_path_visualization current_path.launch
```

也可以直接启动 Python 文件，所有默认参数已满足当前配置：

```bash
./scripts/current_path_publisher.py
```

## RViz

1. 将 `Fixed Frame` 设置为 `map`。
2. 点击 `Add`，选择 `Path`。
3. 将 Path 的 `Topic` 设置为 `/current_path`。

## 参数

- `~parent_frame`：默认 `map`。
- `~child_frame`：默认 `base_link`。
- `~path_topic`：默认 `/current_path`。
- `~lookup_rate`：TF 查询频率，默认 `30.0 Hz`。
- `~min_distance`：相邻记录点的最小三维位移，默认 `0.05 m`；必须严格超过该值。
- `~tf_timeout`：单次 TF 查询超时，默认 `0.1 s`。

查看输出：

```bash
rostopic info /current_path
rostopic echo -n 1 /current_path/header
```
