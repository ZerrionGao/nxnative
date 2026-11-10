// One launcher setting as a row (a menu, a slider with its percentage, a toggle), shared by the iOS launcher (Launcher.swift) and the Mac app's settings window (dm/mac/Launcher.swift).
// A row whose setting is fixed by an env file or by the launch environment is shown but disabled.
// A game packed in Chinese (Info.plist DMLanguage zh-Hans or zh-Hant) shows this screen in Chinese. The keys stay the English strings in settings.tsv and nacp.py.
import SwiftUI

enum LauncherL10n {
    static var chinese: Bool { ((Bundle.main.object(forInfoDictionaryKey: "DMLanguage") as? String) ?? "").hasPrefix("zh") }
    static func text(_ english: String) -> String { chinese ? (zh[english] ?? english) : english }
    static func version(_ number: String) -> String { chinese ? "版本 \(number)" : "Version \(number)" }
    static func quit(_ name: String) -> String { chinese ? "退出 \(name)" : "Quit \(name)" }

    private static let zh = [
        "Game": "游戏", "Controls": "操控", "Performance": "性能",
        "Play": "开始游戏", "Quit": "退出", "Close Window": "关闭窗口",
        "Advanced settings": "高级设置", "Show this window at launch": "启动时显示此窗口",
        "Reset to tested defaults": "恢复测试默认值",
        "Brings back all settings this game was tested with.": "恢复这款游戏测试时使用的全部设置。",
        "Cloud saves": "云存档", "Off": "关", "Turn off": "关闭",
        "Choose a folder…": "选择文件夹…", "Choose a different folder…": "换一个文件夹…",
        "Choose a folder in iCloud Drive…": "在 iCloud 云盘中选择文件夹…",
        "Turn off cloud saves": "关闭云存档",
        "Saved. The game uses this folder when you press Play.": "已保存。按下「开始游戏」后会使用这个文件夹。",
        "That folder could not be remembered — try a different one.": "无法记住这个文件夹，请换一个。",
        "The folder you chose before is no longer readable (the app was signed differently). Please choose it again.": "之前选的文件夹现在读不了（应用签名变了）。请重新选择。",
        "Choose a folder in iCloud Drive, and the same one in the Mac app's Cloud saves. The save comes in when the game starts and goes out after each save; a save that is replaced is kept in save-backups.": "在 iCloud 云盘里选一个文件夹，并在 Mac 应用的云存档里选同一个。游戏启动时读入存档，每次存档后写回；被替换的存档留在 save-backups。",
        "Choose the iCloud Drive folder the iPhone app uses for this game. The save comes in when the game starts and goes out after each save; a save that is replaced is kept in the save's -backups folder.": "选择这款游戏在 iPhone 上使用的 iCloud 云盘文件夹。游戏启动时读入存档，每次存档后写回；被替换的存档留在存档目录的 -backups 文件夹。",
        "Use for Cloud Saves": "用作云存档",
        "The folder the iPhone app chose in its launcher's Cloud saves": "iPhone 应用在启动画面的云存档里选的文件夹",
        "Memory available": "可用内存",
        "Set at launch": "启动时已指定", "Set by an env file": "由环境文件指定",
        "Frame rate": "帧率", "30 fps": "30 帧", "60 fps": "60 帧",
        "Dynamic resolution": "动态分辨率",
        "Adjusts resolution between 70% and 100% under load. Turn off to keep full resolution for the selected console mode.": "负载高时把分辨率调到 70% 到 100%。关掉则保持所选主机模式的完整分辨率。",
        "Render scale": "渲染比例", "Console mode": "主机模式", "Docked": "底座模式", "Handheld": "掌机模式",
        "Upscaling": "放大", "Uses more GPU time and can increase heat.": "会多用 GPU，机身可能更热。",
        "Standard": "标准", "Game language": "游戏语言", "Display mode": "显示模式", "Window": "窗口", "Fullscreen": "全屏",
        "Frame-rate counter": "帧率显示", "Touch feedback": "触控反馈", "Game rumble": "震动",
        "Rumble strength": "震动强度", "Rumble cutoff": "震动下限",
        "Frame buffering": "帧缓冲", "Up to one frame more display latency.": "画面最多多一帧延迟。",
        "Metal 4 pipelines": "Metal 4 管线", "Keeps a pipeline archive; uses more memory.": "会保留管线缓存，占用更多内存。",
        "Pipeline prebuild": "预编译管线", "The first start waits a few minutes for it, once.": "第一次启动会为此多等几分钟，只此一次。",
        "Encode thread": "编码线程", "Hold frames when late": "掉帧时稳住帧率", "Performance HUD": "性能监视",
        "American English": "美式英语", "British English": "英式英语", "Japanese": "日语", "French": "法语", "German": "德语",
        "Latin American Spanish": "拉丁美洲西班牙语", "Spanish": "西班牙语", "Italian": "意大利语", "Dutch": "荷兰语",
        "Canadian French": "加拿大法语", "Portuguese": "葡萄牙语", "Russian": "俄语", "Korean": "韩语",
        "Traditional Chinese": "繁体中文", "Simplified Chinese": "简体中文", "Brazilian Portuguese": "巴西葡萄牙语",
    ]
}

extension LauncherModel {
    func binding(_ s: Setting) -> Binding<String> { Binding(get: { self.values[s.env] ?? s.def }, set: { self.values[s.env] = $0 }) }
}

struct SettingRow: View {
    let s: Setting
    @ObservedObject var m: LauncherModel

    private var locked: Bool { m.fixed.contains(s.env) }
    private var note: String {
        guard locked else { return LauncherL10n.text(s.note) }
        let lock = LauncherL10n.text(m.fromLaunch.contains(s.env) ? "Set at launch" : "Set by an env file")
        return (s.note.isEmpty ? "" : LauncherL10n.text(s.note) + " · ") + lock
    }
    private var rangeValue: Binding<Double> {
        Binding(get: { Double(m.values[s.env] ?? s.def) ?? s.max ?? 1 },
                set: { m.values[s.env] = String(format: "%.2f", $0) })
    }
    private var percentage: String { m.rangeLabel(s, rangeValue.wrappedValue) }

    private var settingLabel: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text(LauncherL10n.text(s.label))
            if !note.isEmpty { Text(note).font(.footnote).foregroundStyle(.secondary) }
        }
    }

    var body: some View {
        Group {
            switch s.kind {
            case "choice":
                Picker(selection: m.binding(s)) {
                    ForEach(s.options ?? [], id: \.[0]) { Text(LauncherL10n.text($0[1])).tag($0[0]) }
                } label: {
                    settingLabel
                }
                .pickerStyle(.menu)
            case "range":
                VStack(alignment: .leading, spacing: 8) {
                    HStack {
                        settingLabel
                        Spacer()
                        Text(percentage).monospacedDigit().foregroundStyle(.secondary)
                    }
                    Slider(value: rangeValue, in: (s.min ?? 0)...(s.max ?? 1), step: s.step ?? 0.05)
                        .accessibilityLabel(LauncherL10n.text(s.label))
                        .accessibilityValue(percentage)
                }
            default:
                Toggle(isOn: Binding(get: { (m.values[s.env] ?? s.def) == s.onValue }, set: { m.values[s.env] = $0 ? s.onValue : s.offValue })) {
                    settingLabel
                }
            }
        }
        .disabled(locked)
    }
}
