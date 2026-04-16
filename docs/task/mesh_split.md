# 目标

参考我给出的代码示例，按如下要求实现功能：

1. 具体 mesh split 分割方案如下，具体拓扑图详见 C:\Users\layton\Desktop\tmp\mesh_split.html：
    - 选中 edge 模式时有两种情况：
        - 四边网格：1、2、4 条时选中的边中间都会进行 split，情况仅有一种，但是选中 3 条边时情况附加会有多种情况，需要用户进行选择具体 split 方式
        - 三角网格：当选中三条边时有两种分割情况
    - 选中 node 模式时仅有一种情况，三角网格 三个 node 都选中时的分割
2. ui 界面设计包括一个 edge node 模式选择，可以并选，然后一个拾取器，当拾取到需要区分的情况时，弹出具体选择按钮，详见参考 ui 选择按钮 C:\Users\layton\Desktop\tmp\ui.png ，每个红框是一组 radio button 其他可以混合设置。
3. 具体算法实现细节参考 C:\Users\layton\Desktop\tmp\immeshspliter*  这几张图片，overview 展示框架没有 lambda 细节 其他两张是补充 lambda ，每种情况的剖分效果见C:\Users\layton\Desktop\tmp\preview 下的 png
4. 这些参考代码是其他软件的，可能细节和 OGL 有出入，尽可能理解并补充相关枚举信息
5. 使用 occ792-relwithdebinfo Ninja build，以支持 gmsh 网格剖分

## 可能用到的信息

1. 注意使用  action-remote-call skill 用来更好的操作 OGL
2. scene 可以截图，注意所有的中间图片都放到这里：C:\Users\layton\Desktop\tmp
3. 启动 opengeolab 时使用 --start-http-server 参数自动启动 http server，以便你使用 http remote call action
4. 具体编译命令见 C:\Users\layton\Desktop\WorkSpace\OGLWorkSpace\OpenGeoLabNew\CMakeUserPresets.json
5. 模型比较大，你可以调整 视角以及缩放 获取最佳视野
6. 你可以自己调试当前代码，缺少的 action 可以自己新加以及 occ 源码也可以修改调试

## 输出信息

1. 为我输出参考代码的讲解更新当前文档追加到下边即可
2. 使用头脑风暴根据参考代码以及我们现有框架，完成目标的设计与开发
