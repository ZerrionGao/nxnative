# 無双OROCHI２ Ultimate（`0100153006300000`）

中文名：无双大蛇 2 终极版。本笔记针对 **日文 Ultimate** 卡带 Title ID `0100153006300000` 显示版本 1.0.0。NACP 仅声明日文。

## 状态（笔记作者环境）

| 步骤 | 结果 |
|---|---|
| 拼卡、解包、临时 profile | 完成 |
| 着色器 | 791 个库，220 组配对 |
| Mac / 手机 | 曾进到出帧；开场影片解码后可达数百～上千帧 |
| 声音 | **无**（SDK 1.6 无进程内音频渲染器；补丁 02 仅为避免 Break） |
| 语言 | 启动窗可中文；游戏内仍为日文 |
| 未做 | 未验收菜单 / 战斗 / 听感 / 长时运行 |

## 关键特征

| 项 | 值 |
|---|---|
| SDK | 1.6.0（NVN、movie、libcurl、NEX 4.1.2） |
| 模块 | `main`、`subsdk0`（GLSLC）、`subsdk1`（libcurl）、`subsdk3`（movie）、`subsdk4`（NVN）、`subsdk5`（sf）、`sdk`（不加载 `subsdk2` libc） |
| RomFS 标记 | `LINKDATA.BIN` + `LINKDATA.IDX`（约 36,196 条记录） |
| 语言 | 仅 `ja` |
| 内存（临时） | 与无双 7 同组猜测值（见 `profile/game.tsv`） |

## 打包语言

`--language ja`（脚本拒绝卡带未声明的语言）。装好后可将 `Info.plist` 的 `DMLanguage` 改为 `zh-Hans`：启动窗中文，宿主报告简体；**游戏文本不会变中文**。官方 Switch 版无中文升级档 / 语言 DLC。

## 本卡特有问题（均需宿主补丁）

1. **跳转表** `main+0x1bb31c` → 补丁 `01`  
2. **`audren:u` Break** → 补丁 `02`（无声）  
3. **影片路径空指针 / 非 const Buffer** → 补丁 `03`  
4. LINKDATA 文件名配对已由无双 7 那次覆盖 → 补丁 `04`

## Profile

见同目录 [`profile/`](profile/)。复制到：

`Games/wo3/input/0100153006300000_build/profile/0100153006300000/`
