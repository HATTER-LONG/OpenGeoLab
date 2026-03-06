# OpenGeoLab 架构说明

## 目标

OpenGeoLab 当前聚焦于几何建模原型、网格生成原型和可视化原型三条主线，并通过 app 模块把 QML、后台服务和渲染控制连接起来。

## 模块职责

- app：QML 与 C++ 之间的入口层，负责请求分发、后台线程、视口对象与用户交互桥接。
- geometry：几何文档与实体层级的权威数据源，负责 OCC shape 到实体关系树和几何渲染数据的构建。
- mesh：网格节点、单元及其关系索引的权威数据源，负责网格渲染数据构建。
- render：渲染快照、场景控制、GPU pass、拾取和高亮。
- io：模型文件导入。

## 当前边界约定

- 跨模块依赖应优先通过 include/ 中的接口完成。
- geometry 和 mesh 负责生产渲染快照，render 负责消费快照并组织 GPU 生命周期。
- app 不应直接修改 render 内部状态，而应通过 service 和 controller 协调。

## 本轮已完成的设计修正

- GeometryDocumentImpl 与 MeshDocumentImpl 增加文档级读写锁。
  目的：避免后台服务线程、渲染数据构建线程与查询线程并发访问时产生数据竞争。
- RenderSceneController 改为共享快照交换。
  目的：避免在收到几何或网格变更时直接原地改写当前渲染快照，降低 GUI 线程与渲染线程之间的悬垂引用风险。
- BackendService 修正错误信号参数和取消语义。
  目的：避免 operationFailed 的 action/error 字段错位，并避免通过 quit/wait 伪装“取消成功”。
- 测试工程补齐 mesh 文档测试并升级到 C++20。

## 本轮审查后仍建议继续处理的点

- 消除 GeoDocumentInstance、MeshDocumentInstance 等全局单例宏对测试和多文档场景的限制。
- 将 QML 到后端的 JSON 字符串接口逐步升级为更强类型的数据边界，减少重复序列化与运行时错误。
- 拆分 RenderSceneController 的职责，把相机状态、渲染快照构建、可见性配置进一步解耦。
- 为 geometry 和 render 模块补充更细粒度的单元测试，尤其是拾取解析、关系索引和渲染批构建。

## 建议的后续演进顺序

1. 为 geometry/render 引入显式文档上下文，逐步弱化单例。
2. 为后台 service 请求定义结构化参数对象和统一错误码。
3. 为渲染快照增加更细的脏区或版本域，减少全量重建频率。
4. 为 mesh 与 geometry 的关系增加更清晰的来源追踪，服务后续局部重建与编辑操作。