# FloraisonRender

FloraisonRender 是基于 C++ 17 / Vulkan 构建的现代渲染器，提供高性能实时图形渲染，具有延迟渲染、网格着色、光线追踪等现代 GPU 特性。

****



## GPU-Driven

重写顶点着色阶段，使用 TaskShader + MeshShader 替换传统顶点+细分+几何着色器，对静态顶点合批，实现前期的精细图元剔除（视锥剔除 + 深度剔除），大幅降低 DrawCall 开销



## 光线追踪全局光照 (RTGI)

建立加速结构并以光线查询 (RayQuery / inline RayTracing) 在片元着色器中查询遮挡或交点辐照度，对每像素在世界空间中以随机采样的方式做辐照度的半球积分，并根据启发式方法做空间滤波和时域积累以实现可接受的降噪效果。

![](https://raw.githubusercontent.com/floraison-io/imgRepo/refs/heads/master/demo_img/RTGI.png)

## 光线追踪软阴影

对于每个像素以随机采样的方式查询自身与区域光源的的可见度，与 RTGI Pass 共同进行时空降噪，实现高质量软阴影



## 漫反射无限次反弹/辐照度缓存 (Radiance Cache)

使用分层体素缓存辐照度，相较于GIBS表面贴花方案，省略持久化储存surfel的启发策略，无需构建查询surfel的空间加速结构。当光线查询得到的交点命中辐照度缓存，便终止继续查询。反之则在查询后写入辐照度缓存。此项可极大提高光线追踪的查询效率，同时实现对于漫反射的无限次光线反弹。

![](https://raw.githubusercontent.com/floraison-io/imgRepo/refs/heads/master/demo_img/inf_bounce.png)



## 时域超分辨率 (TSR/TAAU)

为抵消光线查询所造成的开销，渲染器默认以半分辨率（四分之一像素量）进行渲染，每帧为投影矩阵添加一个微小的抖动 (jitter) 来更新每组中的下一个像素，在帧生成时间较低的情况下造成的延迟可以忽略不计。由于画面由原生半分辨率合成得到，硬件纹理过滤效果有限，但总体在可接受范围。

![](https://raw.githubusercontent.com/floraison-io/imgRepo/refs/heads/master/demo_img/TAAU.png)



## FSR1.0 (RCAS + EASU)

完整复现AMD的单帧自适应锐化方案FSR1.0。边缘适应空间上采样 + 对比度适应锐化。整体观感稍强于硬件双线性插值。

![](https://raw.githubusercontent.com/floraison-io/imgRepo/refs/heads/master/demo_img/EASU_RCAS.png)



## 定制化 UI

JSON 文件加载 UI 定义、渲染 UI 元素（包括纹理、交互式按钮和文本），处理交互以及维护 UI 状态持久性。

![](https://raw.githubusercontent.com/floraison-io/imgRepo/refs/heads/master/demo_img/UI.png)



## 场景序列化

以 JSON 序列存储场景，处理包括物理，光源，HDR天空盒，音频，模型信息等有关参数



## 事件/脚本

通过对文本命令提供解析，对渲染器进行运行时控制。控制台界面允许以命令更改渲染设置、加载资源、配置系统行为以及执行包含多个命令的脚本。



## 空间音频混响

集成 OpenAL-Soft 计算空间音频混响，实现 3D 音效和距离衰减



## 物理碰撞

集成 Bullet3 模拟物理运算，用于碰撞检测
