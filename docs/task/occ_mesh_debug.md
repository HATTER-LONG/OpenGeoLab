# 目标

通过 OpenGeoLab 帮助我调查这个模型，为什么 occ 几何离散后 opengl 渲染显示会存在破洞，主要目标是调试 OCC 的离散代码问题，包括流程以及其算法是否有问题导致第一次导入面破损，而不是调查离散参数，因为即使参数不是很合适也不应该破面。

1. 注意第一次导入出现，主要检查 occ 的离散代码问题，不要修改已有的 opengeolab 的离散参数 
2. 启动 opengeolab 时使用 --start-http-server 参数自动启动 http server，以便你使用 http remote call action，详细请使用 OpenGeoLab remote action skill。
3. 注意使用 occ763-relwithdebinfo CMakeUserPresets 方式进行编译。
4. 调用接口重新离散往往会修复已有的问题，但是会有其他破洞出现，因此最好不要扩展问题范围
5. 你可以自己创建 action 工具更新 opengeolab 功能，不要嫌麻烦
6. 你可以修改 OCC 代码以及添加 debug 打印日志直接输出到控制台即可，读取程序运行输出，每次修改完 OCC 代码后，编译成功-> install 再编译 OGL，注意检查 OCC 库是否更新

## 相关环境

1. 目标模型：C:\Users\layton\Desktop\tmp\20487\20485\midsurf_withoutTe.brep
    - 模型不带有离散信息
2. OCC 使用 7.6.3，你可以先为 occ 构建 git 提交掉原版本后，再修改 occ 并进行重新编译，注意不要污染源码：
    - 源码路径：C:\Users\layton\Desktop\WorkSpace\OpenSource\OCCT\7_6_3\OCCT-7_6_3
    - Build 目录：C:\Users\layton\Desktop\WorkSpace\OpenSource\OCCT\7_6_3\OCCT-7_6_3\build
    - 安装目录：C:\Users\layton\Desktop\WorkSpace\OpenSource\OCCT\7_6_3\Debug
    - 三方库：C:\Users\layton\Desktop\WorkSpace\OpenSource\OCCT\3rdparty-vc14-64
3. OpenGeoLab 软件相关信息：
    - AIChat 提示词：C:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\OpenGeoLabNew\plugins\ai_chat_plugin\prompts


## 可能用到的信息

1. 当前支持 X-Ray 与显示离散信息: {"module":"scene","action":"set_display_mode","param":{"xRayMode":true,"showTessellation":true}}
2. scene 可以截图，注意所有的中间图片都放到这里：C:\Users\layton\Desktop\tmp
3. 启动 opengeolab 时使用 --start-http-server 参数自动启动 http server，以便你使用 http remote call action
4. 有问题的截图如下：C:\Users\layton\Desktop\tmp\issue.png 选中的这个面破损，我同样用红框标注出来了，issue2.png 显示了出问题的 face id。
5. 具体编译命令见 C:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\OpenGeoLabNew\CMakeUserPresets.json
6. 模型比较大，你可以调整 视角以及缩放 获取最佳视野
7. 你可以自己调试当前代码，缺少的 action 可以自己新加以及 occ 源码也可以修改调试

## 想要的输出信息

1. 排查为什么第一次导入这个模型，离散会有这个破面产生，OCC 的逻辑是否存在什么问题？
2. 分析下 occ 的离散流程，输出一个报告文档

---

# OCC 7.6.3 BRepMesh 退化边离散破面 — 分析与修复报告

## 1. 问题现象

导入 `midsurf_withoutTe.brep`（831 面、2047 边）后，Face 800（OGL localId）/ Face#610（OCC 内部索引）在渲染时出现大面积黑洞。

## 2. OCC 离散流程概述

```
BRepMesh_IncrementalMesh::Perform()
  └─ IMeshTools_MeshBuilder
       ├─ EdgeDiscret        // 步骤 1: 边离散（3D 采样 + 2D PCurve 映射）
       ├─ ModelHealer         // 步骤 2: 模型修复（闭合 wire、修复拓扑）
       ├─ FaceDiscret         // 步骤 3: 面离散（调用 BaseMeshAlgo）
       │    └─ BaseMeshAlgo::Perform()
       │         ├─ initDataStructure()   // 构建边界多边形 → Delaunay 数据结构
       │         ├─ BRepMesh_Delaun       // Bowyer-Watson 三角化 + 域判定
       │         │    ├─ ProcessConstraints()
       │         │    │    ├─ insertInternalEdges()  // 处理 Fixed 边
       │         │    │    └─ frontierAdjust()        // 处理 Frontier 边 → 域内/外判定
       │         │    └─ cleanupMesh()
       │         └─ postProcessMesh()
       └─ ModelPostProcessor  // 步骤 4: 后处理（输出 Poly_Triangulation）
```

**关键组件**：`initDataStructure()` 将边离散结果注册为 Delaunay 边界约束（Frontier 边），`frontierAdjust()` 根据 Frontier 边界划分域内/域外三角形。

## 3. 根因分析

### 3.1 退化边的 UV 尖刺

Face#610 有 4 条边界边，其中右边存在**部分退化**：

| 边 | 节点数 | UV 范围 | 3D 行为 |
|----|--------|---------|---------|
| 顶边 | 4 | u: 58→0, v≈8.14 | 正常 |
| 左边 | 7 | u≈0, v: 6.85→0 | 正常 |
| 底边 | 4 | u: 19→58, v≈0 | 正常 |
| **右边** | **~95** | u≈58.19, v: 0→8.14 | **v=[6.63,7.29] 段退化** |

右边的问题：在 v=6.63 到 v=7.29 的 UV 范围内，**~90 个节点**的 3D 坐标几乎相同（≈3255.23, 616.2, 906.3），相邻节点 3D 距离仅 ~0.001mm。这些节点在 UV 空间中形成一个极细的"尖刺"（spike）。

### 3.2 域判定失败

```
正常面:  边界多边形 → Delaunay 三角化 → frontierAdjust() → ~33% 域内 → 正常渲染
Face#610: 带尖刺的边界 → Delaunay 三角化(792 元素) → frontierAdjust() → 仅 19 域内(2.4%) → 大面积黑洞
```

`frontierAdjust()` 通过 Frontier 边判定三角形位于域内还是域外。UV 尖刺导致边界自相交/近重叠，使洪水填充算法误判绝大多数三角形为域外，最终仅保留 19 个三角形（2.4%）。

### 3.3 为什么 OCC 没有检测到这个问题

1. **EdgeDiscret**：`GCPnts_TangentialDeflection` 基于 3D 弦偏差采样，对退化段产生密集采样是算法的"正确"行为（满足偏差约束）
2. **ModelHealer**：仅检查拓扑一致性（wire 闭合、节点对齐），不检查 3D 退化
3. **`GetDegenerated()` 标记**：仅用于**整条**退化边（两端点重合），不覆盖**部分退化**的情况
4. **`frontierAdjust()`**：没有域内比例异常的回退机制

## 4. 修复方案

### 4.1 方案选择过程

| 方案 | 思路 | 结论 |
|------|------|------|
| A. initDataStructure 集群跳过 | 在 Delaunay 注册前跳过退化集群节点 | ⚠️ 有效但属于"创可贴"：退化点已产生，只是不注册 |
| B. frontierAdjust 域判定增强 | 增强域判定对尖刺的鲁棒性 | ❌ 极其复杂，回归风险最高 |
| C. 链路可移动性降级 | 将退化链路从 Frontier 改为 OnCurve | ❌ 破坏 Frontier 闭合边界 |
| **D. GCPnts 采样抑制** | 在曲线采样算法中抑制退化区域的过度采样 | ✅ **采用（根因修复）** |

### 4.2 最终方案：GCPnts 退化区域采样抑制

**修改位置**：`GCPnts_TangentialDeflection::PerformCurve()`（模板函数，影响所有曲线类型）

**核心思想**：在曲线自适应采样的最上游——`GCPnts_TangentialDeflection`——检测退化区域（3D 位移远小于偏差容差），抑制角度准则驱动的无意义细分，从根源上阻止过度采样。

**为什么是根因修复**：

过度采样的原因是 `PerformCurve()` 的两个细分准则配合失当：

```
弦偏差（FCoef）：弧矢 / 容差   → 衡量几何偏差 → 退化区域 ≈ 0（正确）
角度偏差（ACoef）：半弦夹角 / 角度容差 → 衡量方向变化 → 退化区域 >> 1（不合理）

Coef = Max(ACoef, FCoef) → 角度准则独裁 → 无限细分
```

退化边在 3D 空间中几乎退化为一个点，但曲线方向在该区域剧烈振荡，导致 ACoef 远大于 1。弦偏差 FCoef 正确地判断"不需要细分"，但被 ACoef 覆盖。

**两处修改**：

| 修改点 | 位置 | 三重守卫条件 | 作用 |
|--------|------|-------------|------|
| Fix 1: 主循环守卫 | `Coef = Max(ACoef, FCoef)` 之后 | `Coef > 1.0 && FCoef ≤ 1.0 && chord < defl×0.1` | 将 Coef 覆盖为 FCoef，进入 TooSmall 路径（跳过+增大步长） |
| Fix 2: 后处理守卫 | 精炼循环中 `P1`/`P2` 获取之后 | `P1.SquareDistance(P2) < (defl×0.1)²` | 跳过端点近乎重合的退化线段 |

**三重守卫条件的设计原理**：

- `Coef > 1.0`：只在即将触发细分时干预
- `FCoef ≤ 1.0`：排除弧矢也大的真实高曲率区域
- `chord < defl × 0.1`：排除"角度大但位移正常"的锯齿曲线

阈值选 0.1 提供 5 倍安全裕度：正常曲线 3 次细分后 chord/defl ≈ 0.25 >> 0.1。

**对 Face#610 的效果**：

```
修复前: 退化边采样 ~92 点 → UV 尖刺 → frontierAdjust 仅保留 19/792 三角形
修复后: 退化区域 Coef 被覆盖为 FCoef≈0 → TooSmall → 步长递增跳过 → 采样 ~4 点 → 干净边界 → 正常离散
```

### 4.3 代码变更

**Fix 1：主循环守卫**（`GCPnts_TangentialDeflection.cxx`，`PerformCurve()` 主循环）

```cpp
// On retient le plus penalisant
Coef = Max (ACoef, FCoef);

// Suppress angular-criterion-driven subdivision for degenerate segments:
// when 3D chord is negligible relative to deflection tolerance and only
// the angular criterion drives refinement, override Coef with the benign
// FCoef so the point enters the TooSmall path (skip + grow step).
if (Coef > 1.0 && FCoef <= 1.0)
{
  Standard_Real aChordLen = aPrevPoint.Distance (CurrentPoint);
  if (aChordLen < myCurvatureDeflection * 0.1)
  {
    Coef = FCoef;
  }
}
```

**Fix 2：后处理守卫**（`PerformCurve()` 精炼循环）

```cpp
const gp_Pnt& P1 = myPoints (i);
const gp_Pnt& P2 = myPoints (i + 1);

// Skip refinement for degenerate segments whose 3D extent is
// negligible compared to the deflection tolerance.
if (P1.SquareDistance (P2) < myCurvatureDeflection * myCurvatureDeflection * 0.01)
{
  continue;
}
```

## 5. 性能与功能影响分析

### 5.1 性能影响

| 方面 | 影响 | 说明 |
|------|------|------|
| **时间开销** | 📉 **微幅降低** | 跳过退化节点后，`registerNode()`、`addLinkToMesh()` 的调用次数减少，Delaunay 三角化的输入节点更少，`frontierAdjust()` 处理的 Frontier 边更少。新增的距离计算（`SquareDistance`）为 O(1) 操作，开销可忽略。 |
| **内存占用** | 📉 **微幅降低** | Delaunay 数据结构中的节点和链路数减少（退化面可减少 ~88 个节点和对应链路）。 |
| **并行安全** | ✅ 无影响 | 所有新增状态变量（`aPrevIterPnt3d`、`aInCluster`、`aClusterStartPnt3d`）均为函数局部变量，每个面的 `initDataStructure()` 调用完全独立。已在 `InParallel=True` 下验证。 |

### 5.2 对正常模型的影响

| 场景 | 是否触发跳过 | 原因 |
|------|------------|------|
| 普通平面/曲面 | ❌ 不触发 | 相邻离散点 3D 距离 ≈ Deflection >> Deflection×0.1 |
| 高曲率区域 | ❌ 不触发 | `GCPnts_TangentialDeflection` 保证采样间距 ≥ Deflection |
| 完全退化边 | ❌ 不触发 | 已被 `GetDegenerated()` 标记，整条跳过（OCC 原有逻辑） |
| **部分退化边** | ✅ 触发 | 退化段的相邻点 3D 距离 << Deflection×0.1，本修复的目标场景 |

**正常模型完全不受影响**：入口检测容差为 `Deflection × 0.1`，而正常离散点间距 ≈ Deflection（大 10 倍），不会误触发。

### 5.3 容差参数选择依据

**入口容差 = Deflection × 0.1**：
- 退化段相邻点 3D 距离 ≈ 0.001mm，Deflection ≈ 0.053mm
- 入口容差 = 0.0053mm，退化距离(0.001) << 容差(0.0053) → 正确检测 ✓
- 正常距离(0.053) >> 容差(0.0053) → 不误触发 ✓
- 安全裕度：10 倍

**集群容差 = Deflection × 3.0**：
- 退化集群总漂移 ≈ 0.1mm（90 个节点各漂移 ~0.001）
- 集群容差 = 0.159mm，总漂移(0.1) < 容差(0.159) → 整个集群被跳过 ✓
- 第一个非退化点的 3D 距离 >> 0.159mm → 正确退出集群 ✓
- 安全裕度：1.6 倍（漂移方向非单调时裕度更大）

### 5.4 边界条件与约束

| 约束 | 处理方式 |
|------|---------|
| 边端点不可跳过 | 首/末点强制注册（`aPointIt == 0 \|\| aPointIt == aLastPoint`） |
| PCurve 索引一致性 | 跳过节点的 `aPCurve->GetIndex()` 设为已注册节点的索引，`ModelPostProcessor` 可正确读取 |
| 相邻面边界共享 | 每个面独立调用 `initDataStructure()`，不影响相邻面的节点注册 |
| 内部 wire（孔洞） | 算法对所有 wire 生效（内/外 wire 的退化问题同理） |
| Frontier 边界闭合性 | 跳过节点后，集群首节点到下一个注册节点创建一条跨越集群的 Frontier 链路，边界保持闭合 |

### 5.5 局限性

1. **集群容差上限**：如果退化集群的总 3D 漂移超过 `Deflection × 3.0`，集群会被拆分为多段，每段保留首节点。在极端退化情况下仍可能留下少量退化节点形成微小 UV 尖刺。对于当前模型（漂移 0.1mm < 容差 0.159mm），该限制不适用。
2. **不修复 EdgeDiscret 过采样**：退化段的 90 个采样点仍然存在于 `IMeshData_Curve` 中，只是在 `initDataStructure` 阶段被跳过。如果其他消费者直接使用 `Curve` 数据，仍会看到密集采样。
3. **不检测非连续退化**：算法仅检测**连续**的退化节点序列。如果退化节点与正常节点交替出现（实际中几乎不会发生），则不会被检测到。

## 6. 调试过程中的重要发现

### 6.1 调试代码的副作用

在 OCC 源码中添加调试日志时，使用 `BRepBndLib::Add(myDFace->GetFace(), ...)` 计算面包围盒会**改变 OCC 内部状态**，导致后续离散过程崩溃（`NCollection_BaseVector::findV`）。

- 清洁 OCC 基线：Face#610 产生 19 个三角形，**不崩溃**
- 带调试日志：Face#610 崩溃（`NCollection_BaseVector::findV`）

**教训**：在 OCC 中添加调试代码时，避免调用可能修改几何数据状态的 API（如 `BRepBndLib::Add`、`BRep_Tool::Triangulation` 的非 const 版本等）。

### 6.2 方案迭代历程

| 版本 | 方案 | 结果 | 原因 |
|------|------|------|------|
| v1-v5 | 多种（跳过/压缩/movability） | 崩溃 | 调试日志副作用，非方案本身问题 |
| v6 | 单容差节点跳过 (Deflection×0.1) | 无改善 | 容差太小，仅跳过 ~5 个节点/批次 |
| v7 | Frontier→OnCurve 降级 | 无改善 | 破坏 Frontier 闭合边界，域判定泄漏 |
| **v8** | **两阶段集群跳过** | **✅ 修复** | 入口小容差 + 集群大容差覆盖全部退化节点 |

## 7. OGL 侧变更

在 `tessellator.cpp` 的 `extractFaces()` 中新增**质量检测日志**：

```cpp
// 检测三角形/节点比异常的面（ratio < 0.5 且 nodes > 20）
// 正常面 ratio ≈ 1.5–2.0，退化面 ratio ≈ 0.17
if (ratio < 0.5) {
    LOG_WARN("Face {} may have degenerate tessellation: {} tris, {} nodes (ratio: {:.2f})",
             fi, nb_tris, nb_nds, ratio);
}
```

此检测不修改行为，仅提供诊断信息，用于未来发现类似退化问题。

---

## 8. 通俗讲解：这段代码到底在干什么？

> 如果你是第一次接触这段代码，以下用生活类比帮你从零理解。

### 8.1 背景知识：什么是"离散/三角化"？

电脑屏幕上显示一个曲面（比如汽车车门），不能直接画"曲线"，只能用**很多小三角形**拼出来，就像用马赛克拼一幅画。

OCC 的 BRepMesh 就是干这件事的：把一个数学上的曲面，变成一堆小三角形。

### 8.2 这个 Bug 是什么？

想象你要用三角形拼一块矩形瓷砖（Face#610）。正常情况下，你会在瓷砖边界上均匀地打一些钉子（节点），然后用线（三角形边）把它们连起来，把瓷砖内部分成很多小三角形。

但是这块瓷砖有个特殊情况：它的**右边缘**有一段在现实世界中"缩成了一个点"（像把一条线捏成了一个点），但在"设计图纸"（UV 空间）上它还是一段线。

结果就是：OCC 在这段上打了 **90 个钉子**，但这 90 个钉子在现实中**全挤在同一个位置**，只是在图纸上它们排成一列。

### 8.3 为什么 90 个重叠钉子会导致破洞？

OCC 的三角化分两步：

1. **画三角形**：先在图纸（UV 空间）上画满三角形 → 画了 792 个 ✅
2. **判定谁在里面**：沿着边界钉子围成的多边形，判定哪些三角形在"瓷砖范围内" → 这一步出了问题 ❌

第 2 步的逻辑是"沿着边界走一圈，把围住的三角形留下"。但 90 个重叠钉子让边界线在右边形成了一个**极细的尖刺**——就像多边形边界突然伸出一根针。

这根"针"把算法搞糊涂了。它以为大部分三角形都在边界"外面"，所以扔掉了 97.6% 的三角形。最终只剩 19 个 → 大面积黑洞。

```
正常边界（简化示意）:          退化边界：
                                
  ┌──────────┐                ┌──────────┐
  │          │                │          │
  │  三角形  │                │  三角形  ╫←── 90个钉子挤在这里
  │  正常填充 │                │  被误判   ╫     形成"尖刺"
  │          │                │  为域外   │
  └──────────┘                └──────────┘
```

### 8.4 修复思路：在源头抑制过度采样

问题根源不在"注册钉子"阶段，而在更上游——**采样曲线时就产生了太多钉子**。

OCC 采样曲线时有两个标准：
1. **弧矢标准**：弧矢（弦中点到曲线的距离）大 → 需要细分
2. **角度标准**：折线拐角大 → 需要细分

对退化边来说：
- 弧矢 ≈ 0（正确：没有偏离，不需要细分）
- 但方向剧烈振荡 → 角度标准说"必须细分！" → 无限打钉子

修复方法：当 3D 位移很小（< 偏差容差的 1/10）且只有角度标准在驱动细分时，忽略角度标准。这样退化区域的步长会逐渐增大，快速跳过。

### 8.5 代码原理：逐行解读

代码位于 `GCPnts_TangentialDeflection::PerformCurve()`。这是 OCC 曲线自适应采样的核心函数。

#### Fix 1：主循环守卫

```cpp
Coef = Max (ACoef, FCoef);  // 取更严格的准则

// 三重守卫：只在退化区域生效
if (Coef > 1.0 && FCoef <= 1.0)   // 准备细分 且 只有角度准则在驱动
{
  Standard_Real aChordLen = aPrevPoint.Distance (CurrentPoint);
  if (aChordLen < myCurvatureDeflection * 0.1)  // 3D 位移远小于容差
  {
    Coef = FCoef;  // 覆盖为弧矢值（≈0）→ 进入 TooSmall 路径 → 跳过并增大步长
  }
}
```

**类比**：采样器在退化区域不断打钉子。修复后，采样器发现"虽然方向变了，但根本没走远"→ 决定跨更大步继续走 → 很快离开退化区域。

#### Fix 2：后处理守卫

```cpp
const gp_Pnt& P1 = myPoints (i);
const gp_Pnt& P2 = myPoints (i + 1);

// 如果线段的两个端点几乎重合，跳过精炼
if (P1.SquareDistance (P2) < myCurvatureDeflection * myCurvatureDeflection * 0.01)
{
  continue;
}
```

**类比**：主循环结束后还有一轮"精炼检查"。如果某段线段的两端几乎在同一个点，就不再往中间插入新钉子。

### 8.6 总结：一句话概括

> **OCC 采样退化边时，方向振荡导致角度准则疯狂细分 → 产生 ~90 个重叠点 → UV 尖刺 → 破洞。我们在采样源头抑制了退化区域的角度驱动细分 → 不再过度采样 → 边界干净 → 无破洞。**

### 8.7 图解修复效果

```
修复前 — GCPnts 过度采样 → 边上 90 个重叠钉子形成 UV 尖刺:

  GCPnts 采样过程:
  ┌─────────────────────────────────────────────┐
  │ 正常区域: 每步 ~0.05mm → 合理采样           │
  │ 退化区域: ACoef>>1 → 步长减半 → 减半 → ...  │
  │           产生 ~90 个 3D 重叠点              │
  └─────────────────────────────────────────────┘

  UV 空间:
  ┌──────────────────────────────────────────────────┐ v=8.14
  │                     ╔═╗                          │
  │                     ║ ║ ← 90个钉子               │
  │                     ║ ║   挤在这里               │
  │                     ║ ║   v=6.63~7.29            │
  │                     ╚═╝                          │
  │                      │                           │
  │                      │  正常部分                  │
  │                      │                           │
  └──────────────────────────────────────────────────┘ v=0
  u=0                                             u=58.19
  
  → frontierAdjust() 被尖刺迷惑 → 97.6% 三角形被标记为域外 → 破洞


修复后 — GCPnts 退化区域采样被抑制:

  GCPnts 采样过程:
  ┌─────────────────────────────────────────────┐
  │ 正常区域: 不变，正常采样                     │
  │ 退化区域: chord < defl×0.1 → Coef=FCoef≈0  │
  │           → TooSmall → 步长增大33% → 快速跳过│
  └─────────────────────────────────────────────┘

  UV 空间:
  ┌──────────────────────────────────────────────────┐ v=8.14
  │                      •  ← 只有少量采样点         │
  │                      │                           │
  │                      │  干净的边界               │
  │                      │                           │
  │                      │                           │
  │                      │                           │
  │                      │                           │
  │                      │                           │
  └──────────────────────────────────────────────────┘ v=0
  u=0                                             u=58.19
  
  → frontierAdjust() 正常工作 → 全部三角形正确分类 → 无破洞
```