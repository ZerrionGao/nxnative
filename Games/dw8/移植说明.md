# 真・三國無双７ with 猛将伝 DX（`01009D600CA88000`）

中文名：真三国无双 7 猛将传。日文「无双 7」对应西方 Dynasty Warriors 8。本笔记针对 **Switch 亚洲 DX 卡带** Title ID `01009D600CA88000` 显示版本 1.0.0。西方完全版 `0100E9A00CB30000` 是另一份卡，**不能**混用本目录 profile。

## 状态（笔记作者环境）

| 步骤 | 结果 |
|---|---|
| 拼卡、解包、临时 profile | 完成；两份 `dlc/*.bin.gz` 哈希损坏，按读到的字节写入 |
| 着色器 | 890 个库（像素 503、顶点 350、计算 35、几何 2），350 组配对 |
| Mac 包 | 曾跑到约第 300 帧，日志无缺着色器 |
| 手机 | 需自备签名；流程见主 README |
| 未做 | 未验收菜单 / 战斗 / 听感 / 长时运行 |

## 关键特征

| 项 | 值 |
|---|---|
| SDK | 5.6.0（NVN、movie、NEX 4.5.0） |
| 模块 | `main`、`subsdk0`（movie）、`subsdk1`（GLSLC）、`sdk` |
| RomFS 标记 | `LINKDATA.BIN`（旁有 `LINKDATA.IDX`，约 17,403 条 32 字节记录） |
| 语言（NACP） | `ja`、`zh-Hant`、`zh-Hans` |
| 内存（临时） | `mem_reserve 0x100000000`，`mem_alias`/`mem_stack 0x10000000`，`app_pool 0xCD500000`，`sys_resource 0` |

## 打包语言

使用 `--language zh-Hans`，启动窗与游戏语言默认走简体（卡带支持）。

## 本卡特有问题

1. **损坏的 dlc**  
   `--allow-damaged`；已知损坏：`dlc/13169.bin.gz`、`dlc/13170.bin.gz`。

2. **LINKDATA 文件名**  
   必须应用补丁 `04-linkdata-idx-pairing`，否则着色器只扫到散落的一小部分。

3. **不要写入 `dm/games` 直到 Mac 玩通**  
   本仓库 profile 为临时猜测，玩通并人工核对前勿当作正式表项。

## Profile

见同目录 [`profile/`](profile/)。复制到：

`Games/dw8/input/01009D600CA88000_build/profile/01009D600CA88000/`
