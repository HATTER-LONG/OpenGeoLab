# 软件简介 
软件主要功能是通过读取模型文件 brep stp 或者手动创建几何后可以通过 opengl 渲染显示，几何使用 OCC 进行管理，还要支持用户通过鼠标编辑几何，例如 trim 等，支持选择几何点、边、面以及 part，并且后续还会对几何面进行网格剖分，最终还要引入 AI 来实现网格质量诊断与协助修复差网格信息。

# 软件架构
- include 头文件包括对外接口，不同 src 下的目录为不同模块，只能通过 include 暴露的接口进行调用

# 计划任务

## Plan: 重构 Geometry 实体关系索引

TL;DR — 目标是在重构/加载时一次性构建跨实体的反向/直接索引（例如 Node→Part、Node→Face、Edge→Face、Face→Part、Part→成员分组），以便运行时 O(1) 查找而非向上向下爬树。实现路径是在新 `RelationshipIndex`中维护这些映射，重建时基于现有实体关系一次填充，变更时保持增量更新，保持序列化不变（仅内存索引），并采用读多写少的读写锁保护。关键实现点与受影响文件已在发现阶段列出并在步骤中引用。

**Steps**
1. 设计新索引结构新增 `RelationshipIndex` ，基于 EntityID + EntityUid + EntityType 管理关系映射  
   - 新增类 `RelationshipIndex`，负责维护以下映射：
     - node -> parts/Solids/Faces/Wires/Edges  
     - edge -> parts/Solids/Faces/Wires/Nodes
     - wire -> parts/Solids/Faces/Edges/Nodes
     - face -> parts/Solids/Wires/Edges/Nodes
     - solid -> parts
     - part -> members 分组（nodes, edges, faces, solids, wires） 
   - 选择数据结构：以 `IndexHandle`（现有槽句柄）存储以避免 shared_ptr 的生命周期问题，并保持内存高效。

2. 根据构建形状时填充索引：
   - 去除 Geometry Entity 中旧的 parent child 关系，以及修改 document add child detachAllRelations 等维护关系的代码，改为处理新的 RelationshipIndex。以及关系都放到 RelationshipIndex 中维护，不需要继续维护 parent child 这种关系，只需要以类型作为依据。
   - 在文档加载或 rebuild 时，根据 build shape 构建中更新 RelationshipIndex。  

3. 增量维护：在增删/关系变更点更新映射  
   - 在 `EntityIndex::addEntity` / `EntityIndex::removeEntity` 中加入索引更新逻辑（见 entity_index.cpp 和 geometry_documentImpl.cpp）。  
   - 保证在移除实体前先从所有反向映射中清理。

4. API：提供高效查找接口并向后兼容  
   - 在 `GeometryDocumentImpl` 实中增加新方法：
        - `std::vector<EntityId> findRelateTargetIDByNode(EntityId node_id, EntityType target_type);`  
         - `std::vector<EntityId> findRelateTargetIDByEdge(EntityId edge_id, EntityType target_type);`
         - `std::vector<EntityId> findRelateTargetIDByWire(EntityId wire_id, EntityType target_type);`
         - `std::vector<EntityId> findRelateTargetIDByFace(EntityId face_id, EntityType target_type);`
        - `PartMembers getMembersOfPart(EntityId part_id);`
    - 以及 uid 版本：
        - `std::vector<EntityUId> findRelateTargetUidByNode(EntityId node_id, EntityType target_type);`  
        - 依此类推。
   - 在 `GeometryDocument` 对外接口中增加更高级别抽象接口。

5. 并发安全（读多写少）  
   - 在 `RelationshipIndex` 层使用 `std::shared_mutex m_indexMutex`：查找使用 `shared_lock`，写入/重建使用 `unique_lock`，确保尽量缩小加锁范围以减少阻塞。
   - 文档层（`GeometryDocumentImpl`）继续负责更高层的写锁粒度（若已有文档级锁，保持一致）。

6. 性能与内存权衡  
   - 通过 `IndexHandle` 保持引用轻量；只在必要时解析为 `GeometryEntityPtr`。  
   - 提供 `rebuildRelationshipIndex()` 可选参数：只重建某类型或受影响子图以减少完整重建成本。  
   - 避免使用 `entitiesByType` 的 O(max_uid) 遍历，改为直接遍历 `m_slots` 或维护按类型的实体列表（`m_handlesByType`）。

7. 检查工程中所有的 qml cpp hpp 代码，完善或补充注释信息，当前注释不符合要求的也要进行修改。所有注释信息参考  doxygen_comment_style.md 文件中的要求进行编写。
8. 注意更新 README.md 以及 docs/json_protocols.md 文件中的内容，确保与代码实现保持一致，以中文版本为准更新英文版本。
9. 保证最终代码可以编译通过，并正确执行。
10. 为可以的代码单元编写单元测试代码，确保代码质量。
