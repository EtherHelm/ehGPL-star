# ehGPL：STAR-CCM+ 黏势耦合插件

> 基于 **Ghost Potential Layer（GPL）** 方法，实现黏性流与势流双向耦合的 **STAR-CCM+** 插件。

---

## 📋 关于本项目

本插件为 **STAR-CCM+** 提供了 GPL 黏势耦合能力，通过将黏性流求解器（VOF）与势流求解器相结合，在保证精度的前提下显著提高波浪模拟的计算效率。

> **注意**：当前仓库仅包含 STAR-CCM+ 插件代码，用于演示插件与后端服务的交互方式，**不包含后端势流计算代码**。后端代码正在整理开源中，敬请期待。

---

## 🚀 快速开始

### 1. 文件准备

从 [Release 页面](https://github.com/ehw-fit/ehGPL-star/releases) 下载所需文件，并将以下文件放置在与 STAR-CCM+ 计算文件**相同的目录**下：

| 文件 | 说明 |
|------|------|
| `libSCL.so` | 插件动态库 |
| `libducc.so` / `libducc.dll` | FFT 计算依赖库 |
| `config.ini` | 配置文件 |
| `server` | GPL 后端服务程序 |

### 2. 配置 `config.ini`

编辑 `config.ini`，配置以下参数：

| 参数 | 说明 |
|------|------|
| `iniFile` | 初始波浪信息文件路径 |
| `ipcChannel` | 进程间通信管道 |
| `watchDir` | 双向耦合数据交换目录 |

### 3. 加载插件

在 STAR-CCM+ 中通过 **工具 → 加载动态库** 加载插件：

```
libSCL.so
```

---

## ⚙️ 详细配置步骤

### 3.1 设置用户参数

进入 **工具 → 参数（Tools → Parameters）**，新建以下两个参数：

| 参数 | 类型 | 维度 | 含义 |
|------|------|------|------|
| `dx` | 标量 | **长度（Length）** | 势流背景网格的**网格间距** |
| `nx` | 标量 | **无量纲（Dimensionless）** | 势流背景网格的**节点数量** |

### 3.2 运行 `setup.java`

执行 `setup.java` 后，插件将自动创建以下对象：

#### 区域与连续体

- **名称**：`SCL`
- **网格规模**：$n_x \times n_x$
- **网格间距**：`dx`

#### 自动创建的对象清单

| 类别 | 对象名称 |
|------|----------|
| **报告** | `pushData` |
| **监视器** | `History of User eta`、`History of User phi` |
| **衍生零部件** | `wave_potential` |
| **内部表** | `SCL_data`（XYZ Internal Table） |

> ⚠️ **重要检查**：请确认以下对象已正确关联至部件 **`SCL`**：
>
> - `pushData`
> - `History of User eta`
> - `History of User phi`

### 3.3 设置 VOF 波模型

进入 **物理 → 模型 → VOF 波 → 波**，新建一个**用户自定义波**：

```
scl_wave
```

参数配置如下：

| 参数 | 设置值 |
|------|--------|
| **自由表面高度（Free Surface Elevation）** | 场函数：`User etaVOF` |
| **波速（Wave Velocity）** | 场函数：`User velVOF` |

### 3.4 设置黏流域

按常规方式设置黏流域参数。

> 💡 **建议**：在计算域边界附近启用 **VOF 力消波（VOF Wave Damping）**，以抑制黏流域边界处的涡量反射。

---

## 🔄 配置耦合模式

在黏流域中使用波浪 **`scl_wave`** 时，**单向耦合默认自动启用**。

### 启用双向耦合

如需启用**双向耦合**，请按以下步骤操作：

#### Step 1：创建自由液面等值面

新建**等值面（Derived Part → Iso Surface）**：

- **标量场**：`Volume fraction of water`
- **关联对象**：黏流域

该等值面即为黏流域的**自由液面**。

#### Step 2：关联数据表

在 **`SCL_data`（XYZ Internal Table）** 中，将部件关联至上一步创建的**自由液面等值面**。

#### Step 3：切换耦合方式

| 操作 | 效果 |
|------|------|
| ✅ **启用** `SCL_data` 表 | 切换至 **双向耦合** |
| ❌ **禁用** `SCL_data` 表 | 自动回退至 **单向耦合** |

---

## 📝 备注

- 初始波浪信息可通过在线工具生成：[Wave Initialization Tool](https://etherhelm.com/waveIni/)