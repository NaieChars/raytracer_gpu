# GPU Path Tracer — Compute Shader Real-Time Renderer

一个基于 OpenGL 4.3 Compute Shader 构建的实时 GPU 路径追踪器。项目起点是《Ray Tracing in One Weekend》系列的 CPU 实现，随后完整移植到 GPU 计算管线，并在此基础上扩展了迭代式 BVH 遍历、PDF 混合重要性采样、色散、体积次表面散射、多分辨率 Bloom、ACES 色调映射等特性。

![OpenGL](https://img.shields.io/badge/OpenGL-4.3-blue)
![C++](https://img.shields.io/badge/C++-17-orange)
![GLSL](https://img.shields.io/badge/GLSL-430-green)

---

## 特性概览

### 核心渲染管线
- **纯 Compute Shader 光线追踪**：有别于光栅化管线，相机生成光线、求交、着色、降噪全部在 compute shader 中完成
- **迭代式 BVH 遍历**：GPU 不支持递归，因此 BVH 遍历采用显式数组模拟栈，通过数组在 CPU 与 GPU 传递数据，构建阶段支持 **SAH（Surface Area Heuristic）** 最优分割
- **多图元类型支持**：球体（Sphere）与轴对齐立方体（Box），统一通过 BVH 叶子节点的 `primType` 标记区分，解析法求交，无需矩形拼接
- **渐进式累积渲染（Progressive Accumulation）**：多帧样本累加求平均，实现路径追踪的实时收敛（TTA抗锯齿还没做）
- **PCG 哈希随机数生成器**：GPU 线程专用，替代 CPU 端的 `std::mt19937`

### 材质系统
| 材质类型 | 说明 |
|---|---|
| Lambertian | 余弦加权重要性采样(Cosine-Weighted Importance Sampling) |
| Metal | 镜面反射 + 可调模糊度(Fuzz) |
| Dielectric | 玻璃材质，Schlick 近似菲涅尔反射/折射，支持 **随机通道色散(Stochastic Spectral Dispersion)** 模拟三棱镜效果 |
| Diffuse Light | 自发光材质，作为场景光源 |
| Isotropic（体积介质） | **体积随机游走(Volumetric Random Walk)** 实现次表面散射，支持 Henyey-Greenstein 相函数各向异性散射 |

### 光照与采样
- **PDF 混合重要性采样**：Lambertian 表面按 50/50 概率在余弦分布采样与光源方向采样（Next Event Estimation）之间混合，显著降低小光源场景的噪点，但是由于一些不确定性影响（？），玻璃部分噪点严重。
- **球形光源锥体采样**：基于立体角的光源采样，避免采样落在光源背面造成的浪费
- **俄罗斯轮盘赌（Russian Roulette）**：路径深度超过阈值后按贡献度概率终止，无偏且降低计算开销

### 相机系统
- **自由飞行相机（Free-fly Camera）**：LearnOpenGL 基础，通过欧拉角控制，WASD 移动 + 鼠标环视
- **景深（Depth of Field）**：薄透镜模型，支持光圈大小/对焦距离实时调节
- **自动跟焦（Auto Focus Target）**：绑定世界坐标目标点，相机移动时焦平面自动跟随
- 
### 后处理管线
- **多分辨率分层 Bloom**：13-tap 降采样 + 3×3 Tent 升采样构建 mip 链，模拟真实相机光晕的层次感（参考 COD Advanced Warfare, SIGGRAPH 2014）
- **ACES Filmic 色调映射**：HDR 压缩至 LDR，避免高光死白，保留明暗层次
- **色彩分级**：曝光、对比度、饱和度、色彩平衡（冷暖调），可调

---

## 渲染管线架构


场景数据在 CPU 端构建完成后（球体/立方体列表 → BVH 构建 → 拍平为 flat array），通过 SSBO（Shader Storage Buffer Object）一次性上传至显存，GPU 端只负责纯数学计算，不参与场景决策。

raytrace.comp 进行光追与累计样本计算，将纹理传递到 resolve.comp 进行平均化处理（平均帧）与提取高亮，进而通过 downsample.comp  和 upsample.comp 进行分级 Bloom，最后 composite.comp进行叠加处理。

| Binding | 数据 |
|---|---|
| image2D 0 | 累积样本 / 中间纹理 |
| SSBO 1 | BVH 节点（std430，栈式遍历） |
| SSBO 2 | 球体数组 |
| SSBO 3 | 材质数组 |
| SSBO 4 | 光源索引数组 |
| SSBO 5 | 立方体数组 |

---

## 技术栈

- **语言**：C++17 / GLSL 4.30
- **图形 API**：OpenGL 4.3 Core Profile（Compute Shader 最低版本要求）
- **依赖库**（均以源码形式内嵌于 `third_party/`，无需包管理器）：
  - [GLFW](https://www.glfw.org/) — 窗口与输入
  - [GLAD](https://glad.dav1d.de/) — OpenGL 函数加载
  - [GLM](https://github.com/g-truc/glm) — 数学库（矩阵/向量）
  - [stb](https://github.com/nothings/stb) — 图像相关工具（预留）
- **构建系统**：CMake ≥ 3.15

---

## 构建方式

```bash
git clone <your-repo-url>
cd raytracer_gpu
cmake -B build
cmake --build build
```

Windows / MSVC 下产物位于 `build/Debug/raytracer_gpu.exe`。

> 运行前请确认显卡驱动支持 OpenGL 4.3 及以上（Compute Shader 硬性要求）。

---

## 操作说明

| 按键 | 功能 |
|---|---|
| `W` / `A` / `S` / `D` | 相机前后左右移动 |
| `Q` / `E` | 相机上升 / 下降 |
| `Shift` | 加速移动 |
| 鼠标移动 | 视角环视 |
| `F` | 切换景深开关 |
| `[` / `]` | 减小 / 增大光圈（景深虚化强度） |
| `Esc` | 退出鼠标锁定 |

相机移动或参数变化时，累积样本会自动重置，画面从噪点重新开始收敛——这是路径追踪渐进渲染的正常行为，静止等待数秒后画面会重新变得干净。

---

## 项目结构

```
raytracer_gpu/
├── CMakeLists.txt
├── third_party/
│   ├── glfw/
│   ├── glad/
│   ├── glm/
│   └── stb/
└── src/
    ├── main.cpp              # 主循环、SSBO 管理、场景加载
    ├── camera.h               # 自由相机类
    ├── Shader.h                # Shader 编译/链接封装
    ├── bvh.h / sphere.h / ...  # CPU 端场景构建（沿用 RTIOW 体系）
    └── shaders/
        ├── raytrace.comp       # 核心光追:相机生成、BVH遍历、材质散射、体积介质
        ├── resolve.comp        # 样本求平均 + 高亮提取
        ├── downsample.comp     # Bloom mip链降采样
        ├── upsample.comp       # Bloom mip链升采样叠加
        ├── composite.comp      # 合成 + 色调映射 + 色彩分级
        ├── quad.vert / quad.frag  # 全屏三角形显示
```

---

## 已知限制

- 立方体光源目前不参与 Next Event Estimation（光源采样），仅能通过 BSDF 随机采样被间接照亮，收敛速度慢于球形光源
- 体积介质（Isotropic）材质的边界折射简化为直接透射，未做真实的边界折射弯曲
- 暂无 HDRI 环境光照，背景为纯黑（依赖场景内自发光物体提供照明）

---

## Roadmap

- [ ] HDRI 环境光照与重要性采样
- [ ] 立方体光源的 Next Event Estimation 支持
- [ ] 相机路径动画（用于渲染展示视频）
- [ ] 硬件光追（DXR）管线迁移
