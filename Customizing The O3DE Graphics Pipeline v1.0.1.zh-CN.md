# Understanding And Customizing The O3DE Render Pipeline v1.0.1 中文译文

来源 PDF：`C:\Users\Administrator\O3DE\Projects\siggraph2024\Customizing The O3DE Graphics Pipeline v1.0.1.pdf`

说明：本文按原幻灯片页码翻译。代码、API 名称、文件名、路径、命令行参数、链接保持英文。

## 第 1 页

理解并自定义 O3DE 渲染管线  
v1.0.1

## 第 2 页

关于我

Galib Arrieta

- 自 2018 年起积极参与 O3DE 贡献，当时项目还叫 Lumberyard。
- Discord: `galibzon`
- Github: `@galibzon` 或 `@lumbermixalot`

## 第 3 页

关于 O3DE

- https://github.com/o3de/o3de.git
- https://o3de.org/
- https://discord.com/invite/o3de

## 第 4 页

面向游戏与视频的实时、高保真 PBR 管线

虽然本演示的重点是帮助不熟悉 O3DE 的开发者开始使用图形 API，但有必要强调：O3DE 开箱即用地提供了一套实时、AAA 级质量的 PBR 材质系统。除了游戏之外，主渲染管线也已经可以支持机器人领域的计算机视觉仿真以及视频制作。当然，它全部以 MIT + Apache 2.0 许可开源。

使用 O3DE 渲染。

## 第 5 页

O3DE 支持的平台

https://www.docs.o3de.org/docs/welcome-guide/supported-platforms/

开发平台：

- Linux: Vulkan。
- Windows: Vulkan、DX12、OpenXR（通过 Vulkan）。
- MacOS 处于实验性支持状态，主要用于构建使用 O3DE 制作的 iOS 应用。

运行时平台：

- Linux: Vulkan。
- Windows: Vulkan、DX12、OpenXR（通过 Vulkan）。
- Android: Vulkan、OpenXR（通过 Vulkan，已在 Meta Quest 设备上测试）。
- MacOS (Metal)。
- iOS (Metal)。
- 关于主机平台支持，引擎具备允许任何人扩展主机平台支持的基础设施。

## 第 6 页

议程

## 第 7 页

议程

1. 目标受众。
2. 本演示配套的 Github 仓库。
3. O3DE 图形架构总览。
4. Hello World Triangle。（无 C++）
5. Hello World Mesh。（无 C++）
6. 添加自定义 Pass Templates。（C++）
7. 什么时候需要 C++？
8. 创建 Feature Processor。（C++）
9. 向主渲染管线注入 Pass。（C++）
10. 结论与下一步。

## 第 8 页

目标受众

欢迎来到 O3DE

## 第 9 页

目标受众

欢迎来到 O3DE。

本演示假设你至少具备 3D 图形学的基础知识，包括熟悉 Vulkan、DX12、Metal、OpenGL 等 API 中的至少一种。

你还需要有一些使用游戏引擎的经验，这类引擎通常围绕 Entities/Actors 和 Components 的概念来构建。

具体来说，本演示讲的是如何从零开始自定义 O3DE Render Pipeline。你将学到足够多的内容，甚至可以替换 PBR Material System。

## 第 10 页

本演示配套的 Github 仓库

https://github.com/galibzon/siggraph2024

## 第 11 页

配套 Github 仓库

https://github.com/galibzon/siggraph2024

包含两个文件夹：

- `Gem/`：包含代码和资产。作为典型的 O3DE Gem，它可以被其他项目引用和使用。
- `Project/`：包含示例关卡。

## 第 12 页

O3DE 图形架构总览

O3DE -> RPI -> RHI -> GPU。

## 第 13 页

最终目标

概念链路：

Game Logic + Data -> RPI Material System (PBR) -> Render Pipeline Interface (RPI) Pass System -> Render Hardware Interface (RHI) -> DX12 / Vulkan / Metal -> GPU

本演示讲的是如何驱动 Pass System。

最终目标，双关意义上的 end game，是学习如何驱动 PassSystem（docs）。

你将充分理解如何与 MaterialSystem（docs）共存，甚至完全替换它。

## 第 14 页

整体图景

https://www.docs.o3de.org/docs/atom-guide/dev-guide/frame-rendering/

O3DE：Entities 及其 Components 将数据推送到 FeatureProcessors。

RPI：Feature Processors 处理从 Components 接收到的数据，并将 DrawPackets 推送到 Views。通常只有当 View 包含 Feature Processor 关心的 Pass 所对应的 DrawListTag 时，Draw Packets 才会被推送到该 View。

当一个 RenderPipeline tick 时，它的每个 Pass 都会：

- 从 Render Pipeline 拥有的 View 中拉取 DrawListViews。View 只返回与特定 DrawListTag 匹配的 Draw Lists。这些 Draw Lists 会被缓存。
- 所有 Pass 都会把自身作为 Scope 插入 FrameGraphBuilder，也就是同一个 FrameScheduler。

RHI：Frame Scheduler 在每个 Scope（Pass）上调用 API，用于设置 attachment 依赖、绑定 Pass ShaderResourceGroup（如果需要），并根据已经缓存的 Draw Lists 构建 CommandLists。

## 第 15 页

Render Pipelines 可以从 Assets 或 C++ 实例化

Render Pipeline 可以在扩展名为 `.azasset` 的资产中定义。

启动时，O3DE 会假定当前激活的 Render Pipeline 由 `r_renderPipelinePath` CVAR 定义，默认值为 `"passes/MainRenderPipeline.azasset"`。

例如，要用 Mobile RenderPipeline 替换当前激活的 Render Pipeline，可以向 Editor 传递以下命令行参数：

```text
--r_renderPipelinePath=Passes/Mobile/MobileRenderPipeline.azasset
```

Render Pipeline 也可以在运行时完全用 C++ 定义和实例化，或者采用 C++ 与 assets 混合的方式。这么做没有性能优势。

## 第 16 页

Render Pipeline Descriptor Assets 概览

Render Pipeline Descriptor `.azasset`（JSON）文件定义了一个由 PassRequests 组成的有向无环图（DAG）。Pass 之间的依赖关系和执行顺序由 Attachments 定义。

图中 Pass 看起来像是并行运行，但实际它们是连接起来并按 DAG 排序的。

Render Pipeline asset 只能引用通过 `PassFactory::AddPassCreator(...)` 注册过的 C++ Pass 类。

示例可参考 `PassFactory::AddCorePasses()` 或 `CommonSystemComponent::Activate()`。

## 第 17 页

Hello World Triangle（无 C++）

一个只包含一个三角形的自定义 Render Pipeline。

## 第 18 页

Hello World Triangle

让我们创建一个单 Pass 的 Render Pipeline，用它显示一个三角形。最终，用户应该能够配置：

1. 背景颜色，也就是 Clear Color。
2. 三角形三个顶点各自的位置和颜色。
3. 我们会用 One Triangle Render Pipeline 替换 Editor 使用的 Main Render Pipeline。

## 第 19 页

创建 One Triangle Render Pipeline 的步骤概要

1. 创建 One Triangle Shader assets。
2. `OneTriangle.azsl`：实际 shader 代码。
3. `OneTriangle.shader`：定义 shader 入口点、blend states、编译选项等。
4. 创建 Pass asset：`OneTriangle.pass`。
5. 创建 Pipeline，也就是 Parent Pass asset：`OneTrianglePipeline.pass`。
6. 创建 RenderPipelineDescriptor asset：`OneTriangleRenderPipeline.azasset`。
7. 创建 `AutoLoadPassTemplates.azasset` asset。
8. 将 `r_renderPipelinePath` CVAR 配置为命令行参数，让 Editor 显示 One Triangle Render Pipeline。

```text
--r_renderPipelinePath=Passes/Siggraph2024Gem/OneTriangleRenderPipeline.azasset
```

## 第 20 页

`OneTriangle.azsl` - Shader Code

```hlsl
#include <Atom/Features/ColorManagement/TransformColor.azsli>
#include <scenesrg_all.srgi>
ShaderResourceGroup PassSrg : SRG_PerPass {
    float4x4 m_clipSpacePositions;
    float4x4 m_vertexColors;
}
struct VSInput { 
    uint m_vertexID : SV_VertexID;
};
struct VSOutput {
    float4 m_position : SV_Position;
    float4 m_color : COLOR;
};
VSOutput MainVS(VSInput input) {
    VSOutput OUT;
    OUT.m_position = PassSrg::m_clipSpacePositions[input.m_vertexID];
    OUT.m_color = PassSrg::m_vertexColors[input.m_vertexID];
    return OUT;
}
struct PSOutput { 
    float4 m_color : SV_Target0;
};
PSOutput MainPS(VSOutput input) {
    PSOutput OUT;
    OUT.m_color.xyz = TransformColor(input.m_color.xyz, ColorSpaceId::LinearSRGB, ColorSpaceId::SRGB);
    return OUT;
}
```

在 O3DE 中，shader 代码使用 AZSL 编写。AZSL 是 HLSL 的超集。稍后我们会进一步介绍这门语言的细节。

就顶点数据而言，我们拿到的只有 `SV_VertexID` 语义。这是由 `FullscreenTrianglePass` 发起的三顶点 Draw Call。

作为本练习的一部分，每个顶点的位置和颜色来自 `float4x4` shader constants。这些常量会反射到 Pass System 中，并在 Pass asset 里定义，从而获得更高的灵活性。

Vertex shader 和 Pixel shader 函数，也就是入口点，都非常简单。

## 第 21 页

`OneTriangle.shader`

```json
{
    "Source" : "OneTriangle.azsl",
    "DepthStencilState" : { 
        "Depth" : { "Enable" : false, "CompareFunc" : "GreaterEqual" }
    }, 
    "DrawList" : "forward",
    "ProgramSettings":
    { 
      "EntryPoints":
      [ 
        { 
          "name": " MainVS",
          "type": "Vertex" 
        }, 
        { 
          "name": " MainPS",
          "type": "Fragment" 
        } 
      ] 
    } 
}
```

在 O3DE 中，`ShaderSourceData`（`*.shader`）asset 定义了编译 shader 所需的全部内容：AZSL 源码、Depth/Stencil state、Raster state、Blend State、入口点函数（VS、PS 等）以及编译选项。

最低限度上，这个 asset 表示禁用 Depth testing，Vertex Shader 入口点名为 `MainVS`，Pixel Shader 入口点名为 `MainPS`。

Shader 文件应放在：

```text
/GIT/siggraph2024/Gem/Assets/Shaders/Siggraph2024Gem/OneTriangle.azsl
/GIT/siggraph2024/Gem/Assets/Shaders/Siggraph2024Gem/OneTriangle.shader
```

## 第 22 页

`OneTriangle.pass`

```json
{
    "Type": "JsonSerialization",
    "Version": 1,
    "ClassName": "PassAsset",
    "ClassData": {
        "PassTemplate": {
            "Name": "OneTrianglePassTemplate",
            "PassClass": "FullScreenTriangle",
            "Slots": [ 
                { 
                    "Name": "ColorOutput", "SlotType": "Output",
                    "ScopeAttachmentUsage": "RenderTarget",
                    "LoadStoreAction": { 
                        "ClearValue": { "Value": [ 0.75, 0.75, 0.75, 0.0 ] },
                        "LoadAction": "Clear" 
                    } 
                } 
            ], 
            "PassData": { 
                "$type": "FullscreenTrianglePassData",
                "ShaderAsset": { 
                    "FilePath": "Shaders/Siggraph2024Gem/OneTriangle.shader"
                }, 
                "BindViewSrg": true,
                " ShaderDataMappings": {
                    "Matrix4x4Mappings": [ 
                        { 
                            "Name": "m_clipSpacePositions",
                            "Value": [ 0.0,  0.5, 0.0, 1.0,
                                      -0.5, -0.5, 0.0, 1.0,
                                       0.5, -0.5, 0.0, 1.0,
                                       0.0,  0.0, 0.0, 0.0 ]
                        }, 
                        { 
                            "Name": "m_vertexColors",
                            "Value": [ 0.0, 1.0, 0.0, 1.0,
                                       1.0, 0.0, 0.0, 1.0,
                                       0.0, 0.0, 1.0, 1.0,
                                       0.0, 0.0, 0.0, 0.0 ]
                        } 
                    ] 
                } 
            } 
        } 
    } 
}
```

`PassAsset`（`*.pass`）（doc）文件描述一个 Pass Template。Pass Template 指定某个具体 Pass 应实例化的 C++ Pass 类（这里是 `FullscreenTriangle`），以及在这个 Pass 下运行的任何 Shader 所需的全部 attachments。

在这个简单例子中，我们只需要一个 Render Target。当 Pass 在 GPU 上开始执行时，这个 Render Target 会被清除为浅灰色。

`PassData` 属性定义该 Pass 应执行的具体 Shader，以及 Shader Constants 的取值。

此文件应放在：

```text
/GIT/siggraph2024/Gem/Assets/Passes/Siggraph2024Gem/OneTriangle.pass
```

## 第 23 页

`OneTrianglePipeline.pass`

```json
{
    "Type": "JsonSerialization",
    "Version": 1,
    "ClassName": "PassAsset",
    "ClassData": {
        "PassTemplate": {
            "Name": "OneTrianglePipelineTemplate",
            "PassClass": "ParentPass",
            "Slots": [ 
                { 
                    "Name": "PipelineOutput",
                    "SlotType": "InputOutput" 
                } 
            ], 
            "PassRequests": [     
                { 
                    "Name": "OneTrianglePass", 
                    "TemplateName": "OneTrianglePassTemplate",
                    "Enabled": true, 
                    "Connections": [ 
                        { 
                            "LocalSlot": " ColorOutput",
                            "AttachmentRef": { 
                                "Pass": "Parent", 
                                "Attachment": " PipelineOutput"
                            } 
                        } 
                    ] 
                } 
            ] 
        } 
    } 
}
```

Pipeline 本身只是另一个 `PassAsset`，其中 Pass class 是 `ParentPass`。

Parent Pass Template 会列出它需要的所有 Pass（作为 `PassRequests`），以及这些 Pass 如何通过 attachment dependencies（`Connections`）相互连接。

在这个例子中，只请求了一个 child Pass。Child Pass Template 是 `OneTrianglePassTemplate`。`OneTrianglePass` 的 `ColorOutput` Slot Attachment 连接到这个 Parent Pass 的 `PipelineOutput`。借助 PassSystem，这个 `PipelineOutput` 又会直接连接到 Swapchain。

此文件应放在：

```text
/GIT/siggraph2024/Gem/Assets/Passes/Siggraph2024Gem/OneTrianglePipeline.pass
```

## 第 24 页

`OneTriangleRenderPipeline.azasset`

```json
{
    "Type": "JsonSerialization",
    "Version": 1,
    "ClassName": "RenderPipelineDescriptor",
    "ClassData": {
        "Name": "OneTrianglePipeline",
        "RootPassTemplate": "OneTrianglePipelineTemplate"
    } 
}
```

Render Pipeline Descriptor（`*.azasset`）asset 文件描述为了构建 Render Pipeline 应加载哪个 Pass Template。

`RootPassTemplate` 属性定义带有 `"PassClass": "ParentPass"` 的 Pass Template 名称。这个 Parent Pass 会成为 Pipeline 的 Root Pass。

此文件应放在：

```text
/GIT/siggraph2024/Gem/Assets/Passes/Siggraph2024Gem/OneTriangleRenderPipeline.azasset
```

## 第 25 页

`AutoLoadPassTemplates.azasset`

```json
{
    "Type": "JsonSerialization",
    "Version": 1,
    "ClassName": "AssetAliasesSourceData",
    "ClassData": {
        "AssetPaths": [ 
            { 
                "Name": " OneTrianglePassTemplate",
                "Path": "Passes/Siggraph2024Gem/OneTriangle.pass"
            }, 
            { 
                "Name": " OneTrianglePipelineTemplate",
                "Path": "Passes/Siggraph2024Gem/OneTrianglePipeline.pass"
            } 
        ] 
    } 
}
```

任何被其他 Pass Templates 或 Render Pipeline Descriptor 引用的 Pass Template，都必须注册到 Pass System。

O3DE Gems 可以自动加载位于以下路径的自定义 PassTemplate Mappings：

```text
<GemRoot>/Assets/Passes/<GemName>/AutoLoadPassTemplates.azasset
```

此文件应放在：

```text
/GIT/siggraph2024/Gem/Assets/Passes/Siggraph2024Gem/AutoLoadPassTemplates.azasset
```

也有 C++ API 可以向 Pass System 添加 Pass Templates。但在这个 Hello World 示例中，我们会尽一切可能避免使用 C++。

除了 Gems，Game Projects 也可以在以下位置注册自己的 PassTemplate Mappings asset：

```text
<PROJECT ROOT PATH>/Assets/Passes/<ProjectName>/AutoLoadPassTemplates.azasset
```

或者：

```text
<PROJECT ROOT PATH>/Passes/<ProjectName>/AutoLoadPassTemplates.azasset
```

## 第 26 页

运行 Editor 并渲染三角形

使用以下命令行参数执行 Editor：

```text
--r_renderPipelinePath=Passes/Siggraph2024Gem/OneTriangleRenderPipeline.azasset
```

出现提示时，选择任意关卡。

我们已经用一个只渲染一个三角形的单 Pass pipeline 替换了 Main Render Pipeline。

无论你选择、添加或删除哪些 Entities，viewport 都不会变化。没有 FPS counter，没有 gizmos，也没有 meshes，因为这些都是由 Main Render Pipeline 渲染的。

任务完成。你已经拥有了一个数据驱动的 render pipeline，具备自定义背景色以及可配置的三角形顶点和颜色。

可以随意修改 `OneTriangle.pass` 中的 `ClearValue` 或 `ShaderDataMappings`。Pass asset 重新加载后，你应该能立即在屏幕上看到变化，不需要重启 Editor。

## 第 27 页

添加更多三角形

如果再加一个三角形呢？只需再添加另一个 `OneTrianglePassTemplate`，但第二个 pass 不应清除 Render Target。

如果是三个三角形呢？下面是 `TriforceRenderPipeline`：

```text
--r_renderPipelinePath=Passes/Siggraph2024Gem/TriforceRenderPipeline.azasset
```

## 第 28 页

从 One Triangle 学到的内容

1. O3DE 足够灵活，可以以 assets 的形式接收 Render Pipelines 和 Render Passes。这也可以通过 C++ 完成。
2. 只需要一个命令行参数，就能用自己的 Render Pipeline 替换 Main Render Pipeline。
3. Render Pipeline 的 Root/Parent Pass 可以通过硬编码名称 `PipelineOutput`，把 Swapchain 引用为一个 `InputOutput` attachment。
4. 任何基于 `FullscreenTrianglePass` 的具体 Pass 都可以接收三个顶点，这些顶点可在 Vertex Shader 中通过 `SV_VertexID` 语义识别。

## 第 29 页

DrawListTag 深入讲解

把瓶中信发送到多个公共地址。

## 第 30 页

DrawListTag 深入讲解

为了渲染比一个三角形更复杂的内容，理解引擎更高层如何向任意一组 Passes 提交 Draw Packets 非常重要。

从核心上说，`DrawListTag` 是一个地址。它作为一层间接寻址机制，将 Feature Processors 与 RasterPasses 解耦。对开发者而言，`DrawListTag` 可以通过字符串识别，例如 `"forward"`、`"depth"`、`"auxgeom"`、`"motion"`、`"shadow"` 等。运行时，`DrawListTag` 是与这些字符串之一相关联的索引。最多可以有 64 个唯一的 `DrawListTag`，也就是 `AZ::RHI::Pipeline::DrawListTagCountMax`。

Feature Processors 将 DrawPackets 提交给 Views。Draw Packet 内的每个 Draw Item 都包含一个 mask，用于组合所有感兴趣的 DrawListTags。反过来，Passes 预期只读取与自己关心的 DrawListTag 匹配的 Draw Items。当然，如果它们愿意，也可以打破这个规则。

## 第 31 页

DrawListTag 深入讲解

运行时，RPI 会为每个 DrawListTag 字符串分配一个索引。

示例索引：

- `"auxgeom"`: `[0]`
- `"shadow"`: `[1]`
- `"motion"`: `[2]`
- `"forward"`: `[8]`
- `"depth"`: `[12]`

Feature Processor 提交 Draw Items，每个 item 拥有一个 32 位 mask：`DeviceDrawItemProperties::m_drawFilterMask`。其中每个激活的 bit 代表一个 DrawListTag。

这是一个 Draw Item 被寻址到 `"depth"`、`"forward"` 和 `"shadow"` 的示例。

Render Pipeline 中的每个 Pass 都可以识别寻址到自己的 Draw Items。这些 items 会变成由各 Pass 派发的 Draw Calls。

## 第 32 页

Hello World Mesh Rendering（无 C++）

一个自定义 Render Pipeline，它借用了 Mesh Feature Processor 和 Material System。

## 第 33 页

Hello World Mesh Rendering（无 C++）

让我们弄清楚如何组装一个自定义 Render Pipeline，用它渲染来自 Mesh Components 的 mesh assets。Mesh Normals 会被用作 Fragment Colors。

使用命令行参数：

<code style="color:red">
--r_renderPipelinePath=Passes/Siggraph2024Gem/SiggraphRenderPipeline.azasset
</code>

不传命令行参数时，会使用 `MainRenderPipeline.azasset`。

## 第 34 页

不写 C++ 能渲染 Meshes 吗？

答案是可以。让我们弄清楚如何组装一个自定义 Render Pipeline，用它渲染来自 Mesh Components 的 mesh assets。

在讨论 Feature Processors 提交 Draw Items 时，有一个点我们略过了：Draw Item 是带着 Pipeline State Object 创建的，也就是 PSO。这意味着 Feature Processor 在构建 Draw Item 时必须知道要使用哪个 Shader Asset。

具体来说，Mesh Feature Processor 会与 Material System 协同工作。Mesh Component（doc）与 Material Component（doc）配合，将 Model Asset 与 Material Asset 关联起来。Material Asset 进而描述渲染 Mesh 时应使用的一组 Shaders。

我们将创建一个最基础的 Material，使用自定义 Shader。自定义 Render Pipeline 将包含一个 Raster Pass，它会匹配该自定义 Shader 的 DrawListTag。

Draw Item 里包含一个 cylinder 的 Draw Item。它包含 VS 与 PS 的二进制 shader 函数，这些函数来自已编译的 Shader Assets。

## 第 35 页

创建自定义 Material

这不是关于 O3DE PBR Material System 的演示。相反，我们会尽可能少地借用 Material System，以便能够渲染带有 Mesh + Material Components 的 entities。

概要：

1. 创建 Shader，并设置 `"DrawList": "siggraph"`。
2. 创建 Material Type。
3. 创建 Material。
4. 创建 Pass asset。它会使用带有 `"DrawListTag": "siggraph"` 的 RasterPass。
5. 创建单 Pass pipeline 的 Pass asset。
6. 创建 Render Pipeline Descriptor asset。
7. 在 Editor 中，为 ShaderBall 和 Ground entities 设置新的 Material asset。

## 第 36 页

`NormalVectorDisplay.azsl`

```hlsl
#include <scenesrg_all.srgi>
#include <viewsrg_all.srgi>
#include <Atom/Features/PBR/DefaultObjectSrg.azsli>
struct VSInput {
    float3 m_position : POSITION;
    float3 m_normal : NORMAL;
};
struct VSOutput {
    float4 m_position : SV_Position;
    float3 m_normal: NORMAL;
};
VSOutput MainVS(VSInput IN) {
    VSOutput OUT;
    float3 worldPosition = mul(ObjectSrg::GetWorldMatrix(), float4(IN.m_position, 1.0)).xyz;
    OUT.m_position = mul(ViewSrg::m_viewProjectionMatrix, float4(worldPosition, 1.0));
    OUT.m_normal = IN.m_normal;
    return OUT;
}
struct PSOutput {
    float4 m_color : SV_Target0;
};
PSOutput MainPS(VSOutput IN) {
    PSOutput OUT;
    OUT.m_color = float4(normalize(IN.m_normal), 1.0);
    return OUT;
}
```

这是一个极简 shader，用顶点 Normals 作为 fragment Colors 来渲染 mesh。

Material System 会自动设置与 Scene、View 和 Object 相关的 shader constants。

Object World Matrix 对应的 shader constant 可在 `ObjectSrg` 中使用。View-Projection Matrix 对应的 shader constant 位于 `ViewSrg::m_viewProjectionMatrix`。

位置：

```text
/GIT/siggraph2024/Gem/Assets/Shaders/Siggraph2024Gem/Materials/NormalVectorDisplay.azsl
```

## 第 37 页

`NormalVectorDisplay.shader`

```json
{
    "Source" : "NormalVectorDisplay.azsl",
    "DepthStencilState" : { 
        "Depth" : { "Enable" : true, "CompareFunc" : "GreaterEqual" }
    }, 
    "DrawList" : "siggraph",
    "ProgramSettings": {
      "EntryPoints":
      [ 
        { 
          "name": "MainVS", 
          "type": "Vertex" 
        }, 
        { 
          "name": "MainPS", 
          "type": "Fragment" 
        } 
      ] 
    } 
}
```

这一次我们启用 depth testing，以便能按正确的深度顺序渲染所有 Entities。

最重要的是，我们设置了 `"DrawList" : "siggraph"`，它应与自定义 RasterPass template（`NormalVectorDisplay.pass`）的 `DrawListTag` 匹配。

位置：

```text
/GIT/siggraph2024/Gem/Assets/Shaders/Siggraph2024Gem/Materials/NormalVectorDisplay.shader
```

## 第 38 页

`NormalVectorDisplay.materialtype`

```json
{
    "description": "For Siggraph2024. This material renders vertex normals as fragment colors",
    "version": 1,
    "shaders": [
        { 
            "file": "Shaders/Siggraph2024Gem/Materials/NormalVectorDisplay.shader"
        } 
    ] 
}
```

通常，Material Type 会列出所有 Material Properties。与 Material Properties 相关的 shader constants 和 resources 包含在 Material ShaderResourceGroup 中，也就是 shader 代码里的 `MaterialSrg`。

在我们的案例中，没有 Material Properties。稍后我们会处理这部分。

Material Type 拥有一组 Shaders。在这个例子中，我们只使用一个 Shader。对于场景中所有使用我们自定义 Material 的 Mesh Entities，这个 Shader 会成为 Draw Items Pipeline State Object 的一部分。

位置：

```text
/GIT/siggraph2024/Gem/Assets/Materials/Siggraph2024Gem/NormalVectorDisplay.materialtype
```

## 第 39 页

`NormalVectorDisplay.material`

```json
{
    "materialType": "NormalVectorDisplay.materialtype",
    "materialTypeVersion": 1
}
```

Material asset 表示 Material Type 的一个实例。每个 Material Property 的具体值会列在这里。

在我们的案例中，没有 Material Properties。

这个极简 Material asset 只是说明它是 `"NormalVectorDisplay.materialtype"` 的一个实例。

稍后给带有 Mesh Component 的 Entities 添加 Material Component 时，我们会选择这个 asset。

位置：

```text
/GIT/siggraph2024/Gem/Assets/Materials/Siggraph2024Gem/NormalVectorDisplay.material
```

## 第 40 页

编写新的 Render Pipeline 前的一些思考

此时我们有了一个名为 `NormalVectorDisplay.material` 的新 Material asset。我们可以把它用在带有 Mesh Components 的 Entities 上。

但是，在使用 Main Render Pipeline 时，关联到 `NormalVectorDisplay.material` 的 meshes 会变得不可见，因为 Main Render Pipeline 中没有任何 RasterPass 会查找带有 DrawListTag `"siggraph"` 的 Draw Items。

一旦把 Material Component 设置为 `NormalVectorDisplay.material`，`Shader Ball` Entity 的 mesh 就会消失。

## 第 41 页

`NormalVectorDisplay.pass`

```json
{
    "Type": "JsonSerialization", "Version": 1, "ClassName": "PassAsset",
    "ClassData": {
        "PassTemplate": {
            "Name": "NormalVectorDisplayPassTemplate", "PassClass": "RasterPass",
            "Slots": [ 
                { "Name": "DepthOutput",
                  "SlotType": "Output", 
                  "ScopeAttachmentUsage": "DepthStencil",
                  "LoadStoreAction": { 
                        "ClearValue": { "Type": "DepthStencil" }, 
                        "LoadAction": "Clear", 
                        "LoadActionStencil": "Clear" 
                    } 
                }, 
                { "Name": "LightingOutput", 
                  "SlotType": "Output", 
                  "ScopeAttachmentUsage": "RenderTarget",
                  "LoadStoreAction": { 
                        "ClearValue": { "Value": [ 0.7, 0.0, 0.0, 0.0 ] }, 
                        "LoadAction": "Clear" 
                    } 
                } 
            ], " ImageAttachments": [
                { 
                    "Name": "DepthAttachment",
                    "SizeSource": { "Source": { "Pass": "Parent", "Attachment": "PipelineOutput" } },
                    "ImageDescriptor": { "Format": "D32_FLOAT_S8X24_UINT", "SharedQueueMask": "Graphics" }
                } 
            ], "Connections": [ 
                { 
                    "LocalSlot": "DepthOutput",
                    "AttachmentRef": { "Pass": "This", "Attachment": "DepthAttachment" }
                } 
            ], "PassData": { 
                "$type": "RasterPassData", 
                "DrawListTag": "siggraph",
                "BindViewSrg": true 
            } 
        } 
    } 
}
```

Pass Class 是 `RasterPass`，并带有 `"DrawListTag": "siggraph"`。所有带有这个 tag 的 Draw Items 都会在此 Pass 下被渲染。

这个 Pass 会要求 Pass System 创建一个名为 `DepthAttachment` 的 transient GPU Image Attachment，用作 Depth Testing 的 DepthStencil attachment。

当 Pass 开始执行时，它会用深红色 `(0.7, 0.0, 0.0, 0.0)` 清除 Render Target。

对于 Color attachment，这个 Pass Template 需要 Parent Pass 将 `LightingOutput` Slot 连接到正确的 Color attachment。稍后 Parent Pass template `NormalVectorDisplayPipeline.pass` 会把它直接绑定到 Swapchain。

位置：

```text
/GIT/siggraph2024/Gem/Assets/Passes/Siggraph2024Gem/NormalVectorDisplay.pass
```

## 第 42 页

`NormalVectorDisplayPipeline.pass`

```json
{
    "Type": "JsonSerialization",
    "Version": 1,
    "ClassName": "PassAsset",
    "ClassData": {
        "PassTemplate": {
            "Name": "NormalVectorDisplayPipelineTemplate",
            "PassClass": "ParentPass",
            "Slots": [ 
                { 
                    "Name": "PipelineOutput", 
                    "SlotType": "InputOutput" 
                } 
            ], 
            "PassRequests": [     
                { 
                    "Name": "NormalVectorDisplayPass", 
                    "TemplateName": "NormalVectorDisplayPassTemplate",
                    "Enabled": true, 
                    "Connections": [
                        { 
                            "LocalSlot": " LightingOutput",
                            "AttachmentRef": { 
                                "Pass": "Parent", 
                                "Attachment": " PipelineOutput"
                            } 
                        } 
                    ] 
                } 
            ] 
        } 
    } 
}
```

这是我们的单 Pass Pipeline Pass template。

它明确表示 `NormalVectorDisplayPass` 中的 `LightingOutput` Slot 应直接绑定到 Swapchain。

位置：

```text
/GIT/siggraph2024/Gem/Assets/Passes/Siggraph2024Gem/NormalVectorDisplayPipeline.pass
```

## 第 43 页

`NormalVectorDisplayRenderPipeline.azasset`

```json
{
    "Type": "JsonSerialization",
    "Version": 1,
    "ClassName": "RenderPipelineDescriptor",
    "ClassData": {
        "Name": "NormalVectorDisplayPipeline",
        "MainViewTag": "MainCamera",
        "RootPassTemplate": "NormalVectorDisplayPipelineTemplate",
        "AllowModification": false,
        "RenderSettings": {
            "MultisampleState": { 
                "samples": 1 
            } 
        } 
    } 
}
```

这是一个常规 Render Pipeline Descriptor asset，它引用 `"NormalVectorDisplayPipelineTemplate"` 作为 Root Pass Template。

位置：

```text
/GIT/siggraph2024/Gem/Assets/Passes/Siggraph2024Gem/NormalVectorDisplayRenderPipeline.azasset
```

## 第 44 页

扩展 `AutoLoadPassTemplates.azasset`

```json
{
    "Type": "JsonSerialization",
    "Version": 1,
    "ClassName": "AssetAliasesSourceData",
    "ClassData": {
        "AssetPaths": [ 
            { 
                "Name": "OneTrianglePassTemplate",
                "Path": "Passes/Siggraph2024Gem/OneTriangle.pass"
            }, 
            { 
                "Name": "OneTrianglePipelineTemplate",
                "Path": "Passes/Siggraph2024Gem/OneTrianglePipeline.pass"
            }, 
            { 
                "Name": "OneTriangleNoClearPassTemplate",
                "Path": "Passes/Siggraph2024Gem/OneTriangleNoClear.pass"
            }, 
            { 
                "Name": "TriforcePipelineTemplate",
                "Path": "Passes/Siggraph2024Gem/TriforcePipeline.pass"
            }, 
            { 
                "Name": " NormalVectorDisplayPassTemplate",
                "Path": "Passes/Siggraph2024Gem/NormalVectorDisplay.pass"
            }, 
            { 
                "Name": " NormalVectorDisplayPipelineTemplate",
                "Path": "Passes/Siggraph2024Gem/NormalVectorDisplayPipeline.pass"
            } 
        ] 
    } 
}
```

我们又向 Pass System 添加了两个 Pass Templates：

- `NormalVectorDisplayPassTemplate`
- `NormalVectorDisplayPipelineTemplate`

位置：

```text
/GIT/siggraph2024/Gem/Assets/Passes/Siggraph2024Gem/AutoLoadPassTemplates.azasset
```

## 第 45 页

Showtime（第 1 部分）

运行 Editor，不传命令行参数。

打开名为 `DefaultLevel` 的关卡。

将关卡另存为 `NormalsAsColor`。给 `Shader Ball` 和 `Ground` 两个 Entities 添加 Material Component，并为它们选择 `NormalVectorDisplay.material` asset。

如预期一样，两个 Meshes 都会消失，因为没有 RasterPass 查找 `DrawListTag == "siggraph"` 的 Draw Items。

然后用以下参数重启 Editor：

```text
--r_renderPipelinePath=Passes/Siggraph2024Gem/NormalVectorDisplayRenderPipeline.azasset
```

加载名为 `NormalsAsColor` 的新关卡。

## 第 46 页

什么时候需要 C++？

什么时候需要自定义 Components、Feature Processors 或 Passes？

## 第 47 页

什么时候需要 C++？

有些时候编写 C++ 是不可避免的。让我们回顾其中一些情况。

概念上，O3DE 和 RPI 假设通过以下总体数据流向 RHI 提交工作：

```text
Data -> Component -> Feature Processor -> DrawPackets -> Lists of DrawItems -> RasterPass -> RHI
```

对于某些路径，则是：

```text
Data -> Component -> Feature Processor -> Subclass of RenderPass -> RHI
```

第二种路径会引用特定的 RenderPass 实例。

如果一个 Render Pipeline 无法通过 O3DE 提供的 Core/Generic Passes 的某种组合完成任务，就必须添加自定义 C++ Pass class，也可能需要新的 Feature Processors 以及对应的面向用户的 Components。可参考 `void PassFactory::AddCorePasses()`。

## 第 48 页

利用 Mesh Feature Processor

Mesh 和 Material Systems 足够灵活，有时你不需要编写 C++，前面已经演示过。有时你也可以借用这些系统的一部分来完成任务。

常规路径：

```text
Mesh Asset -> Mesh Component -> Mesh Feature Processor -> DrawPackets -> RasterPass -> RHI
Material Asset -> Material Component
```

如果三角形数据并不来自 Asset 呢？例如，数据来自 TCP Stream。

```text
TCP Stream -> Custom C++ Component -> Mesh Feature Processor -> DrawPackets -> RasterPass -> RHI
```

你可以借用 Mesh Feature Processor，但需要一个自定义 C++ Component。

类似地，只要源数据本质上是动态的，例如 WhiteBox Component（doc），并且数据由三角形等标准 primitives 组成，就需要自定义 C++ Component，同时它可以借用 Mesh Feature Processor。

## 第 49 页

其他常见的需要 C++ 的情况

- 某些 Shader Constants 的值可能每帧变化，它们由 Pass System 管理，例如 `SceneSrg::m_time`。如果 Pipeline 需要其他类型的动态 Shader Constants 数据，就需要 C++。
- 不使用 Material System 时，如果需要将 Texture resources 绑定到 Shaders，就需要自定义 C++ Passes，也可能需要自定义 Feature Processors。
- 非标准 primitive 数据，例如 Volumetrics，总是需要自定义 Feature Processors、自定义 Passes，或者两者组合。

## 第 50 页

创建 Feature Processor（C++）

每个人都喜欢贴纸，对吧？

## 第 51 页

一个新的 Feature Processor（C++）

每个人都喜欢贴纸，对吧？

图片由 https://meta.ai 生成。Prompt：

```text
"Picture of a laptop with stickers"
```

让我们创建一个 Feature Processor，在 Viewport 上渲染 stickers。

## 第 52 页

一个渲染 Stickers 的 Feature Processor

在这个关卡中，有三个 entities，它们各自带有自己的 `Sticker` Component。

## 第 53 页

Sticker Feature Processor

这个 Feature Processor 的目标是在 viewport 上叠加渲染任意数量的 Images，也就是 Stickers。

这些 images 可以带有 Alpha color。

用户可以使用归一化坐标（0.0 到 1.0 之间）指定 sticker 的位置和尺寸，也可以在需要时保持 image 的原始比例。

## 第 54 页

Sticker Feature Processor 的数据流

数据流概念：

```text
Sticker Component 1
Sticker Component 2
Sticker Component N
        -> Sticker Feature Processor
        -> Draw Packet 1 / Draw Packet 2 / Draw Packet N
        -> Main Camera View
        -> Sticker Pass (RasterPass)
        -> RHI
```

所有 Draw Packets 都带有 DrawListTag `"sticker"`。

Sticker Pass 获取所有带有 DrawListTag `"sticker"` 的 Draw Packets。

备注：我们会借用 `RasterPass` 类。不需要用 C++ 编写自定义 Pass。

我们会编写两个新的类：

- Sticker Component：面向用户的 Component，可在其中配置 Sticker 的尺寸和 image asset。
- Sticker Feature Processor：从每个 Sticker Component 获取数据，并向 Render Pipeline 的 main View 提交带有 `"sticker"` DrawListTag 的 Draw Packets。

还会添加其他 helper classes。

## 第 55 页

`Sticker.pass` 只是一个普通的 Raster Pass

```json
{
    "Type": "JsonSerialization",
    "Version": 1,
    "ClassName": "PassAsset",
    "ClassData": {
        "PassTemplate": {
            "Name": " StickerPassTemplate",
            "PassClass": " RasterPass",
            "Slots": [ 
                { 
                    "Name": "ColorOutput", 
                    "SlotType": "Output", 
                    "ScopeAttachmentUsage": "RenderTarget",
                    "LoadStoreAction": { 
                        "LoadAction": "Load"
                    } 
                } 
            ], 
            "PassData": { 
                "$type": "RasterPassData", 
                "DrawListTag": " sticker",
                "BindViewSrg": true 
            } 
        } 
    } 
}
```

`StickerPassTemplate` 只是一个普通的 `RasterPass`，它会渲染带有 `"sticker"` DrawListTag 的 Draw Items。

这个 Pass 开始时不会清除 Render Target 的内容。Draw Items，也就是 Stickers，会绘制在已有像素内容之上。

位置：

```text
/GIT/siggraph2024/Gem/Assets/Passes/Siggraph2024Gem/Sticker.pass
```

## 第 56 页

`StickerPipeline.pass` Render Pipeline

```json
{
    . . .
        "PassTemplate": {
            "Name": " StickerPipelineTemplate",
            "PassClass": "ParentPass",
             . . . 
            "PassRequests": [     
                { 
                    "Name": " NormalVectorDisplayPass",
                    "TemplateName": "NormalVectorDisplayPassTemplate",
                    . . .
                }, 
                { 
                    "Name": " StickerPass",
                    "TemplateName": "StickerPassTemplate",
                    "Enabled": true, 
                    "Connections": [ 
                        { 
                            "LocalSlot": " ColorOutput",
                            "AttachmentRef": { 
                                "Pass": "Parent", 
                                "Attachment": "PipelineOutput" 
                            } 
                        } 
                    ] 
                } 
            ] 
        } 
    } 
}
```

这个 asset 基于 `NormalVectorDisplayPipeline.pass`。它与原始版本的区别是添加了第二个 Pass，名为 `StickerPass`。

`StickerPass` 基于上一页展示的 Template `StickerPassTemplate`。

现在 Pipeline 包含两个 passes。

`StickerPass` 会把 stickers 直接渲染到 Swapchain 中。

位置：

```text
/GIT/siggraph2024/Gem/Assets/Passes/Siggraph2024Gem/StickerPipeline.pass
```

## 第 57 页

`StickerRenderPipeline.azasset` Render Pipeline Descriptor

```json
{
    "Type": "JsonSerialization",
    "Version": 1,
    "ClassName": "RenderPipelineDescriptor",
    "ClassData": {
        "Name": "StickerPipeline",
        "MainViewTag": "MainCamera",
        "RootPassTemplate": "StickerPipelineTemplate",
        "AllowModification": false
    } 
}
```

这只是一个标准 Render Pipeline Descriptor asset，它通过 template `StickerPipelineTemplate` 引用我们的新 Pipeline。

位置：

```text
/GIT/siggraph2024/Gem/Assets/Passes/Siggraph2024Gem/StickerRenderPipeline.azasset
```

## 第 58 页

注册新的 Pass Templates

```json
{
    "Type": "JsonSerialization",
    "Version": 1,
    "ClassName": "AssetAliasesSourceData",
    "ClassData": {
        "AssetPaths": [ 
            . . . 
            { 
                "Name": "NormalVectorDisplayPipelineTemplate",
                "Path": "Passes/Siggraph2024Gem/NormalVectorDisplayPipeline.pass"
            }, 
            { 
                "Name": " StickerPassTemplate",
                "Path": "Passes/Siggraph2024Gem/Sticker.pass"
            }, 
            { 
                "Name": " StickerPipelineTemplate",
                "Path": "Passes/Siggraph2024Gem/StickerPipeline.pass"
            } 
        ] 
    } 
}
```

新的 pass templates 必须注册，否则当 `StickerRenderPipeline.azasset` 被设为激活或默认 Render Pipeline 时会崩溃。

位置：

```text
/GIT/siggraph2024/Gem/Assets/Passes/Siggraph2024Gem/AutoLoadPassTemplates.azasset
```

备注：如果使用以下命令行参数启动 Editor：

```text
--r_renderPipelinePath=Passes/Siggraph2024Gem/StickerRenderPipeline.azasset
```

一切应该可以正常工作。

Pipeline 末尾会有第二个什么都不做的 Pass，名为 `StickerPass`。

之所以不会发生任何新变化，是因为目前还没有 Feature Processor 提交带有 `"sticker"` DrawListTag 的 Draw Packets。

## 第 59 页

`StickerComponentController`

这个类是 `StickerComponent` 与 `StickerFeatureProcessor` 之间的联络者。

当这个类 Activated 时，它会调用 `StickerFeatureProcessor::AddSticker()`，用于实例化一个 DrawPacket。这个 DrawPacket 包含在屏幕上渲染 Sticker 所需的全部数据。

位置：

```text
/GIT/siggraph2024/Gem/Code/Source/Components/StickerComponentController.h
/GIT/siggraph2024/Gem/Code/Source/Components/StickerComponentController.cpp
```

每当用户修改此 Component 的属性，例如 Position、Size 或 Texture 变化时，这个类会调用：

```cpp
StickerFeatureProcessor::UpdateStickerGeometry()
```

或者：

```cpp
StickerFeatureProcessor::UpdateStickerTexture()
```

当用户删除此 Component 时，这个类会 `Deactivate()`，并调用 `StickerFeatureProcessor::RemoveSticker()`。

## 第 60 页

`StickerFeatureProcessor`（`struct StickerRenderData`）

这是负责提交 Draw Packets 的类。每个 Sticker 会创建一个 Draw Packet。

```cpp
struct StickerFeatureProcessor::StickerRenderData
{
    StickerProperties m_properties;
    AZStd::array<StickerVertex, VertexCountPerSticker> m_cpuVertices;
    AZ::Data::Instance<AZ::RPI::Buffer> m_gpuVertexBuffer;
    AZStd::array<AZ::RHI::StreamBufferView, 1> m_gpuVertexBufferView;
    AZ::Data::Instance<AZ::RPI::ShaderResourceGroup> m_drawSrg;
    AZ::RHI::ConstPtr<AZ::RHI::DrawPacket> m_drawPacket;
    bool m_needsSrgUpdate = true;
};
AZStd::unordered_map<AZ::EntityId, StickerRenderData> m_stickers;
```

`m_properties` 是来自 Sticker Component 的原始数据副本，也包含 Texture Image。

`m_cpuVertices`、`m_gpuVertexBuffer`、`m_gpuVertexBufferView` 等是 CPU 和 GPU 渲染相关的数据与 Buffers。

`m_drawPacket` 引用 GPU Buffer Views 和 `DrawSrg`。`DrawSrg` 是 Shader Constants 与 SRVs 的容器。

所有 Stickers 都被方便地组织在一个 Map 中，可通过 `EntityId` 寻址。

## 第 61 页

`StickerFeatureProcessor`（把 Sticker 作为一种 Primitive）

从绘制角度看，一个 Sticker 是一个包含 4 个顶点的 Triangle Strip，表示由两个三角形组成的 Quad。按照右手法则，这些三角形应朝向屏幕外：

```text
(0, 1, 2), (1, 3, 2)
```

位置：

```text
/GIT/siggraph2024/Gem/Code/Source/Render/StickerFeatureProcessor.h
/GIT/siggraph2024/Gem/Code/Source/Render/StickerFeatureProcessor.cpp
```

primitive 的布局和内容在此函数中定义：

```cpp
void InitCpuVertices(AZStd::array<StickerVertex, 4>& cpuVertices, const StickerProperties& properties);
```

Shader `Sticker.shader`（`Sticker.azsl`）不会变换顶点。因此，顶点预期已经处于 Normalized Device Coordinates，W 分量为 `1.0`，Z 为 `0.0`。

```cpp
struct StickerVertex
{
    AZStd::array<float, 3> m_position;
    AZStd::array<float, 2> m_uv;
};
```

## 第 62 页

`Sticker.shader`

位置：

```text
/GIT/siggraph2024/Gem/Assets/Shaders/Siggraph2024Gem/Sticker.shader
```

```json
{
    "Source" : "Sticker.azsl",
    "DepthStencilState" : { 
        "Depth" : { "Enable" : false, "CompareFunc" : "GreaterEqual" }
    }, 
    "GlobalTargetBlendState" : {
      "Enable" : true,
      "BlendSource" : "One",
      "BlendAlphaSource" : "One",
      "BlendDest" : "AlphaSourceInverse",
      "BlendAlphaDest" : "AlphaSourceInverse",
      "BlendAlphaOp" : "Add"
    }, 
    "DrawList" : "sticker",
    "ProgramSettings":
    { 
      "EntryPoints":
      [ 
        { 
          "name": "MainVS", 
          "type": "Vertex" 
        }, 
        { 
          "name": "MainPS", 
          "type": "Fragment" 
        } 
      ] 
    } 
}
```

如预期，这个 shader 没有启用 Depth Testing。

Blend State 被设置为支持 `"Add"` Alpha Blending Operation。

`StickerFeatureProcessor` 会使用这个 Shader asset 中的 DrawListTag `"sticker"`，作为 Draw Packets 的 DrawListTag。

## 第 63 页

`Sticker.azsl`

```hlsl
#include <Atom/Features/SrgSemantics.azsli>
#include <Atom/Features/ColorManagement/TransformColor.azsli>
ShaderResourceGroup DrawSrg : SRG_PerDraw {
    Texture2D<float4> m_texture;
    Sampler LinearSampler;
}
struct VSInput {
    float3 m_position : POSITION; float2 m_uv : UV0;
};
struct VSOutput {
    float4 m_position : SV_Position; float2 m_uv : UV0;
};
VSOutput MainVS(VSInput input) {
    VSOutput OUT;
    OUT.m_position = float4(input.m_position, 1);
    OUT.m_uv = input.m_uv;
    return OUT;
}
struct PSOutput {
    float4 m_color : SV_Target0;
};
PSOutput MainPS(VSOutput input) {
    PSOutput OUT;
    float4 color = DrawSrg::m_texture.Sample(DrawSrg::LinearSampler, input.m_uv);
    const float3 srgbColor = TransformColor(color.xyz, ColorSpaceId::LinearSRGB, ColorSpaceId::SRGB);
    OUT.m_color = float4(srgbColor, color.a);
    return OUT;
}
```

这是一个简单的 Vertex Shader 与 Pixel Shader 函数组合。

Vertex Shader 只是对数据进行透传，让数据被插值后发送给 Pixel Shader。

Sticker image，也就是 `DrawSrg::m_texture`，会被线性采样为输出颜色。这个输出颜色会通过 Alpha Blended（Add Op）与 Render Target 混合。

位置：

```text
/GIT/siggraph2024/Gem/Assets/Shaders/Siggraph2024Gem/Sticker.azsl
```

## 第 64 页

Showtime

使用以下参数运行 Editor：

```text
--r_renderPipelinePath=Passes/Siggraph2024Gem/StickerRenderPipeline.azasset
```

加载名为 `NormalsWithStickers` 的关卡。

## 第 65 页

向主管线注入 Passes（C++）

如何在运行时修改另一个 Render Pipeline。

## 第 66 页

如何向 Main Pipeline 注入自定义 Pass

我们已经学会了如何使用自己的 Render Pipelines。

但是，是否可以把自己的 Passes 注入到 Main Pipeline 中？不手动复制粘贴原始 `MainRenderPipeline.azasset` 可以吗？我们能不能把 Sticker Pass 添加到 Main Pipeline？

答案是可以。要归功于 FeatureProcessor 函数：

```cpp
virtual void AddRenderPasses(RenderPipeline* pipeline) {}
```

这个函数作为 Callback 工作。只有当 `pipeline` 允许修改时，它才会被调用。

当 Feature Processor 重写 `AddRenderPasses` 时，它可以使用 `AZ::RPI::PassFilters` 搜索其他 Passes，并根据结果决定是否向 `pipeline` 添加 Passes。

## 第 67 页

`StickerFeatureProcessor::AddRenderPasses(AZ::RPI::RenderPipeline* pipeline)`

```cpp
const auto StickerPassName = AZ::Name("StickerPass");
const auto UiPassName = AZ::Name("UIPass");
// Early exit if there's no "UIPass".
{
    AZ::RPI::PassFilter passFilter = AZ::RPI::PassFilter::CreateWithPassName(UiPassName, pipeline);
    AZ::RPI::Pass* existingPass = AZ::RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);
    if (existingPass == nullptr)
    { 
        // Can’t find “UIPass”
        return; 
    } 
}
// Early exit if the Sticker Pass already exists
{
    ... 
}
// We can safely add the StickerPass to the pipeline
static constexpr bool AddBefore = false;
AddPassRequestToRenderPipeline(pipeline,
   "Passes/Siggraph2024Gem/StickerPassRequest.azasset",
   UiPassName.GetCStr(), AddBefore);
// Optionally, validate we succeeded with another search.
{
   ... 
}
```

只有当 `pipeline` 允许修改时，这个函数才会被 PassSystem 调用。

`StickerFeatureProcessor` 会搜索名为 `UIPass` 的 Pass。如果该 Pass 不存在，就无法把 `StickerPass` 注入到 Pipeline。

只有当 `StickerPass` 是最后一个写入 Render Target 的 Pass 时，它才能很好地工作。它应被添加到 `UIPass` 之后。

备注：下一页会看到，必须在 `UIPass` 与 `StickerPass` 之间建立正确的 attachment connection，才能强制正确的执行顺序：`StickerPass` 必须在 `UIPass` 之后开始。

## 第 68 页

`StickerPassRequest.azasset`

```json
{
    "Type": "JsonSerialization",
    "Version": 1,
    "ClassName": "PassRequest",
    "ClassData": {
        "Name": " StickerPass",
        "TemplateName": "StickerPassTemplate",
        "Enabled": true,
        "Connections": [
            { 
                "LocalSlot": " ColorOutput",
                "AttachmentRef": { 
                    "Pass": " UIPass",
                    "Attachment": " InputOutput"
                } 
            } 
        ] 
    } 
}
```

PassRequest asset 只有在被 `AddPassRequestToRenderPipeline(...)` 调用引用时才有意义。

这个 asset 看起来像 Pipeline asset 的一个切片。

具体来说，这个 asset 假设 `UIPass` 存在，并且它的 Render Target slot 名为 `InputOutput`。这就是为什么 `StickerFeatureProcessor` 的 C++ runtime 会先搜索是否存在名为 `UIPass` 的 pass。

这个 request 表示 `StickerPass` 的 `ColorOutput` Slot 必须连接到 `UIPass` 的 `InputOutput` Slot。这会保证正确的执行顺序。

## 第 69 页

带 Stickers 的 Main Pipeline

现在我们可以在不传命令行参数的情况下运行 Editor。它会默认使用 Main Render Pipeline。但这一次，`StickerFeatureProcessor` 足够聪明，会注入 Sticker Pass。

加载名为 `DefaultLevelWithStickers` 的关卡。

## 第 70 页

结论与下一步

结束只是新的开始。

## 第 71 页

结论

对于 O3DE RPI 和 RHI 能做的事情，我们只是触及了表面。

O3DE Graphics APIs 是数据驱动的，但在需要时也可以用 C++ 完全自定义。

O3DE Material System 并不局限于只支持 Main Render Pipeline（基于 PBR）。相反，它本身就是一个引擎，已经准备好用于创建独特或专有的 Materials、Render Pipelines 和 Shaders。

Main Render Pipeline 是一个 Forward+ PBR pipeline，但现在应该很清楚：构建一个 Deferred PBR Pipeline 是直接可行的。它可以与 Main Pipeline 共存，并允许开发者在两者之间来回切换。

## 第 72 页

下一步

- Fork O3DE 并贡献代码：https://github.com/o3de/o3de
- 使用 OpenXR Gem 开发 VR 应用：https://github.com/o3de/o3de-extras
- 文档地址：https://www.docs.o3de.org/docs/
- 加入 O3DE Discord Server：https://discord.gg/nV5m4mJr
- 加入 `sig-graphics-audio`，讨论 O3DE 中与 3D Graphics 相关的一切。

使用 O3DE 渲染。

## 第 73 页

可选内容，为了完整性：

使用 C++ 添加 Custom Pass Templates。

这是向 Pass System 添加更多 Pass Templates 的另一种方式。

## 第 74 页

添加 Custom Pass Templates

除了创建名为 `AutoLoadPassTemplates.azasset` 的文件之外，还有一种替代方式可以注册自定义 Pass Templates。这种方式需要少量 C++。它完全是可选的，并且没有任何性能收益。

任何 Gem（或 Project）都可以实例化一个类型为以下类型的 EventHandler：

```cpp
RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler
```

它应使用以下 API 注册到 Pass System：

```cpp
RPI::PassSystemInterface::ConnectEvent(handler)
```

这是一次性注册，通常在 System Component 的 `Activate()` 中完成。

示例：

1. `Gems/Atom/Feature/Common/Code/Source/CommonSystemComponent.cpp`
2. `Gems/Terrain/Code/Source/Components/TerrainSystemComponent.cpp`

当时机合适时，Pass System 会调用所有连接到 `RPI::PassSystemInterface::OnReadyLoadTemplatesEvent` 事件的 handlers 的 callbacks。在这个 callback 中，自定义 System Component 可以调用 API：

```cpp
AZ::RPI::PassSystemInterface::LoadPassTemplateMappings(String& assetTemplatePath)
```

## 第 75 页

`Siggraph2024Gem` 中有被注释掉的代码

```text
/GIT/siggraph2024/Gem/Code/Source/Clients/Siggraph2024GemSystemComponent.h
```

```cpp
// Load pass template mappings for this Gem.
// void LoadPassTemplateMappings();
// We use this event handler to add our Pass Templates to the RPI::PassSystem.
// The callback will invoke this->LoadPassTemplateMappings()
// AZ::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler m_loadTemplatesHandler;
```

```cpp
void Siggraph2024GemSystemComponent::Activate() {
...
    // // Register the event handler that helps us register our custom pass templates.
    // m_loadTemplatesHandler = AZ::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler(
    //     [&]() { 
    //         LoadPassTemplateMappings();
    //     } 
    // ); 
    // AZ::RPI::PassSystemInterface::Get()->ConnectEvent(m_loadTemplatesHandler);
} 
// void Siggraph2024GemSystemComponent::LoadPassTemplateMappings() {
//     constexpr char passTemplatesFile[] = "Passes/Siggraph2024Gem/PassTemplates.azasset";
//     AZ::RPI::PassSystemInterface::Get()->LoadPassTemplateMappings(passTemplatesFile);
// } 
```

```text
/GIT/siggraph2024/Gem/Code/Source/Clients/Siggraph2024GemSystemComponent.cpp
```

## 第 76 页

谢谢！

问题？
