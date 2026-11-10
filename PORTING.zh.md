# 移植与打包指南（中文）

本仓库是 [nxnative](https://github.com/seedds/nxnative) 的完整可用副本，并已集成下列修复，**无需再手动打 patch**：

- 跳转表在索引重载后仍识别范围检查（无双大蛇 2）
- 无进程内渲染器的 SDK 本地应答 `audren`（无声但可启动）
- 将游戏图形分配器装入 `NvOs`；接受 SDK 1.6 非 const 的 `movie::Buffer`
- `LINKDATA.BIN` 与 `LINKDATA.IDX` 配对（真三国无双 7）
- 启动窗在 `DMLanguage` 以 `zh` 开头时显示中文

另附两款 KTGL 游戏的临时 profile 与过程说明：

| 目录 | 游戏 | Title ID |
|---|---|---|
| [Games/dw8](Games/dw8/) | 真三国无双 7 猛将传 DX | `01009D600CA88000` |
| [Games/wo3](Games/wo3/) | 无双大蛇 2 终极版 | `0100153006300000` |

仓库**不含**游戏本体、`prod.keys`、签名证书或成品 `.app` / IPA。请合法持有游戏，密钥只放本机。

## 1. 环境

需要：Apple 芯片 Mac、macOS 26.6+、Xcode 27.x、Homebrew、Python 3.12+（arm64）。

```sh
sudo xcode-select -s /Applications/Xcode.app
xcodebuild -downloadComponent MetalToolchain
git clone https://github.com/ZerrionGao/nxnative.git
cd nxnative
brew install xcodegen zstd glslang
brew install --cask dotnet-sdk
python3 -m pip install lz4 "capstone>=5,<6" fonttools nsz zbic "backports.zstd; python_version < '3.14'"
```

也可用 `uv run --with …` 按下方命令临时装 Python 依赖。

## 2. 放置游戏与密钥

以真三国无双 7 为例（大蛇 2 把 `dw8` 换成 `wo3`，Title ID 换成 `0100153006300000`）：

```text
Games/dw8/input/
  keys/
    prod.keys          # 自备
    title.keys         # 如有
  ShinSangokuMusou7XL-[01009D600CA88000][v0].xci
```

分段卡带请先按 `00`–`03` **显式拼接**成一份 XCI，再核对总长度与 `HEAD`。拼接后可删除分段原件。

临时 profile 已在：

```text
Games/dw8/profile/01009D600CA88000/
Games/wo3/profile/0100153006300000/
```

打包时让工具优先读它们：

```sh
export DM_GAMES_DIR="$PWD/Games/dw8/profile"
export DM_GAMES_FIRST=1
export GAME=01009D600CA88000
```

（大蛇 2：`DM_GAMES_DIR=$PWD/Games/wo3/profile`，`GAME=0100153006300000`。）

## 3. 打 Mac 包

在仓库根目录：

```sh
export DM_GAMES_DIR="$PWD/Games/dw8/profile"
export DM_GAMES_FIRST=1

uv run --no-project --python /opt/homebrew/bin/python3.14 \
  --with lz4 --with 'capstone>=5,<6' --with fonttools \
  python scripts/build_ipa_from_nsp.py Games/dw8/input \
  -k Games/dw8/input/keys/prod.keys \
  --platform mac-app \
  --allow-other-metal --allow-damaged --gpu none \
  --language zh-Hans \
  -o Games/dw8/artifacts/apps/ShinSangokuMusou7.app \
  --progress lines
```

无双大蛇 2：

```sh
export DM_GAMES_DIR="$PWD/Games/wo3/profile"
export DM_GAMES_FIRST=1

uv run --no-project --python /opt/homebrew/bin/python3.14 \
  --with lz4 --with 'capstone>=5,<6' --with fonttools \
  python scripts/build_ipa_from_nsp.py Games/wo3/input \
  -k Games/wo3/input/keys/prod.keys \
  --platform mac-app \
  --allow-other-metal --allow-damaged --gpu none \
  --language ja \
  -o Games/wo3/artifacts/apps/WarriorsOrochi3U.app \
  --progress lines
```

| 开关 | 作用 |
|---|---|
| `--allow-other-metal` | 本机 Metal 版本与仓库锁定不一致时仍继续 |
| `--gpu none` | 不打入不匹配的离线 PSO；首次启动本机编译 |
| `--allow-damaged` | 个别文件哈希损坏时仍写入读到的字节 |
| `--language` | 必须是卡带 NACP 已声明的语言 |

## 4. 打 IPA（未签名）

把上面命令的 `--platform mac-app` 换成 `--platform ios`，`-o` 改成例如 `Games/dw8/artifacts/ipas/ShinSangokuMusou7.ipa`。

未签名 IPA **不能**直接装机。请用你自己的 Apple Developer 账号签名，Bundle ID 自定，描述文件须含 `com.apple.developer.kernel.increased-memory-limit`。不要把 Xcode DerivedData 里没有 `romfs.img` 的空壳装到手机。

## 5. 常见问题

见各游戏目录的说明，以及曾单独整理的 FAQ 要点：

- **没有 profile**：检查 `DM_GAMES_DIR` / `DM_GAMES_FIRST` / `GAME`。
- **着色器特别少**：确认已是本仓库（含 LINKDATA.IDX 配对），不要用未集成修复的旧上游裸仓。
- **点 Play 立刻 Break / audren:u**：本仓库已处理；大蛇 2 仍无声音。
- **开场影片闪退**：本仓库已处理 NvOs 分配器与 Buffer 符号。
- **大蛇 2 游戏内仍是日文**：卡带无中文数据，属预期。

更细的机制说明在 `docs/reference/`（英文）。

## 6. 与上游的关系

上游项目：<https://github.com/seedds/nxnative>（MIT）。本仓库在其基础上集成移植所需修复与两款游戏的临时 profile，方便一键克隆使用。若你要跟进上游最新提交，请自行 `git remote add upstream` 后合并，并解决可能冲突。
