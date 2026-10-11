#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
BMI088_final_lib_setup.py
BMI088(加速度计 + 陀螺仪)库自动配置工具

它会把你指定目录下的 Keil MDK-ARM 工程配置好：
  1. 复制库文件到 <你的MDK-ARM>/BMI088_final_lib/
  2. 在 .uvprojx 里新建同名分组，加入全部 .c/.h
  3. 把头文件搜索路径 .\BMI088_final_lib 加进 C 编译器设置
  4. 把 README(BMI088_final_lib.md) 复制到工程目录下
  5. 检查 .ioc 里 SPI1 的引脚配置（MOSI 配成 PB5 会直接报警，这是最容易踩的坑）
  6. 所有 .uvprojx 修改前都会自动备份，界面上一键可还原

用法：
  图形界面：python BMI088_final_lib_setup.py
  命令行  ：python BMI088_final_lib_setup.py --cli "D:\\proj\\MyBoard\\MDK-ARM"
  还原备份：python BMI088_final_lib_setup.py --cli "D:\\proj\\MyBoard\\MDK-ARM" --restore
"""

import argparse
import datetime
import glob
import os
import re
import shutil
import sys

LIB_DIR_NAME = "BMI088_final_lib"          # 库文件夹名(和 README 同名)
README_NAME = "BMI088_final_lib.md"        # README 文件名(和库文件夹同名)
GROUP_NAME = "BMI088_final_lib"            # 在 Keil 里显示的分组名
C_FILES = ["bmi088_common.c", "bmi088_accel.c", "bmi088_gyro.c"]
H_FILES = ["bmi088.h", "bmi088_config.h", "bmi088_reg.h", "bmi088_private.h"]
ALL_FILES = C_FILES + H_FILES + [README_NAME]

# 期望的 C 板接线(用于 .ioc 检查)
EXPECT_IO = {"SPI1_SCK": "PB3", "SPI1_MISO": "PB4", "SPI1_MOSI": "PA7"}


class SetupError(Exception):
    """配置过程中的可预期错误，直接展示给用户"""


# ---------------------------------------------------------------- 基础工具

def script_dir():
    return os.path.dirname(os.path.abspath(__file__))


def src_lib_dir():
    return os.path.join(script_dir(), LIB_DIR_NAME)


def read_text(path):
    """按字节读、UTF-8 解，无法解码的字节原样保留(surrogateescape)"""
    with open(path, "rb") as f:
        raw = f.read()
    return raw.decode("utf-8", "surrogateescape")


def write_text(path, text):
    with open(path, "wb") as f:
        f.write(text.encode("utf-8", "surrogateescape"))


def newline_of(text):
    return "\r\n" if "\r\n" in text else "\n"


# ---------------------------------------------------------------- 找工程

def find_uvprojx(target):
    """在用户给的路径(或其下一/二级目录，常见的是 MDK-ARM)里找 .uvprojx"""
    target = os.path.abspath(target)
    if not os.path.isdir(target):
        raise SetupError("路径不存在或不是文件夹：\n%s" % target)

    hits = sorted(glob.glob(os.path.join(target, "*.uvprojx")))
    if hits:
        return hits[0], target

    for sub in sorted(os.listdir(target)):
        d = os.path.join(target, sub)
        if os.path.isdir(d):
            hits = sorted(glob.glob(os.path.join(d, "*.uvprojx")))
            if hits:
                return hits[0], d

    for sub in sorted(os.listdir(target)):
        d = os.path.join(target, sub)
        if not os.path.isdir(d):
            continue
        for sub2 in sorted(os.listdir(d)):
            d2 = os.path.join(d, sub2)
            if os.path.isdir(d2):
                hits = sorted(glob.glob(os.path.join(d2, "*.uvprojx")))
                if hits:
                    return hits[0], d2

    raise SetupError("在这个路径下没找到 Keil 工程文件(.uvprojx)：\n%s\n"
                     "请填到你的 MDK-ARM 文件夹，或者工程根目录。" % target)


# ---------------------------------------------------------------- 复制文件

def copy_lib_files(uv_dir, log):
    src = src_lib_dir()
    if not os.path.isdir(src):
        raise SetupError("找不到库文件夹：\n%s\n(请把本脚本和 %s 文件夹放在同一目录)" % (src, LIB_DIR_NAME))

    missing = [f for f in ALL_FILES if not os.path.isfile(os.path.join(src, f))]
    if missing:
        raise SetupError("库文件不完整，缺少：%s" % ", ".join(missing))

    dst = os.path.join(uv_dir, LIB_DIR_NAME)
    os.makedirs(dst, exist_ok=True)
    for name in ALL_FILES:
        shutil.copy2(os.path.join(src, name), os.path.join(dst, name))
    log("[1/5] 已复制 %d 个文件到 %s" % (len(ALL_FILES), dst))
    return dst


# ---------------------------------------------------------------- 改 uvprojx

def build_group_xml(existing_text, lib_rel):
    """按工程里现有的缩进生成 <Group> 片段"""
    nl = newline_of(existing_text)
    m = re.search(r"([ \t]*)<Group>\r?\n([ \t]*)<GroupName>", existing_text)
    i1 = m.group(1) if m else "      "
    i2 = i1 + "  "
    i3 = i1 + "    "
    i4 = i1 + "      "

    out = [i1 + "<Group>",
           i2 + "<GroupName>" + GROUP_NAME + "</GroupName>",
           i2 + "<Files>"]
    for name in C_FILES + H_FILES:
        ftype = "1" if name.endswith(".c") else "5"
        out.append(i3 + "<File>")
        out.append(i4 + "<FileName>" + name + "</FileName>")
        out.append(i4 + "<FileType>" + ftype + "</FileType>")
        out.append(i4 + "<FilePath>.\\" + lib_rel + "\\" + name + "</FilePath>")
        out.append(i3 + "</File>")
    out.append(i2 + "</Files>")
    out.append(i1 + "</Group>")
    return nl.join(out) + nl


def patch_uvprojx(uvprojx, lib_rel, log):
    text = read_text(uvprojx)
    nl = newline_of(text)
    original = text

    # --- 1) 头文件搜索路径：只改 <Cads> 里的那个 IncludePath ---
    m = re.search(r"<Cads>.*?</Cads>", text, re.S)
    if not m:
        raise SetupError("这个 .uvprojx 里找不到 <Cads> 段，可能不是标准 Keil 工程：\n%s" % uvprojx)

    cads = m.group(0)
    if lib_rel in cads:
        log("[2/5] 头文件搜索路径里已经有 %s，跳过" % lib_rel)
    else:
        def add_path(mm):
            old = mm.group(1).strip()
            new = (old + ";" if old else "") + ".\\" + lib_rel
            return "<IncludePath>" + new + "</IncludePath>"

        new_cads = re.sub(r"<IncludePath>([^<]*)</IncludePath>", add_path, cads, count=1)
        text = text[:m.start()] + new_cads + text[m.end():]
        log("[2/5] 已把头文件搜索路径 .\\%s 加入 C 编译器设置" % lib_rel)

    # --- 2) 文件分组 ---
    if ("<GroupName>" + GROUP_NAME + "</GroupName>") in text:
        log("[3/5] 工程里已经有 %s 分组，跳过" % GROUP_NAME)
    else:
        idx = text.rfind("</Groups>")
        if idx < 0:
            raise SetupError("这个 .uvprojx 里找不到 </Groups>，可能不是标准 Keil 工程")
        text = text[:idx] + build_group_xml(text, lib_rel) + text[idx:]
        log("[3/5] 已新建分组 %s，加入 %d 个文件" % (GROUP_NAME, len(C_FILES) + len(H_FILES)))

    # --- 3) 备份 + 写回 ---
    if text != original:
        stamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        backup = uvprojx + ".bak_" + stamp
        shutil.copy2(uvprojx, backup)
        write_text(uvprojx, text)
        log("[4/5] 已修改 .uvprojx（备份：%s）" % os.path.basename(backup))
    else:
        log("[4/5] .uvprojx 无需修改")


def restore_latest_backup(uvprojx, log):
    backs = sorted(glob.glob(uvprojx + ".bak_*"))
    if not backs:
        raise SetupError("没有找到备份文件(%s.bak_*)，无法还原" % os.path.basename(uvprojx))
    latest = backs[-1]
    shutil.copy2(latest, uvprojx)
    log("已用备份还原 %s" % uvprojx)
    log("使用的备份：%s" % latest)


# ---------------------------------------------------------------- 检查 ioc

def check_ioc(uv_dir, log):
    cands = []
    for d in (uv_dir, os.path.dirname(uv_dir)):
        cands += sorted(glob.glob(os.path.join(d, "*.ioc")))
    if not cands:
        log("[检查] 没找到 .ioc，跳过引脚检查（只影响自动检查，不影响编译）")
        return

    ioc = cands[0]
    text = read_text(ioc)
    found = {}
    for signal in EXPECT_IO:
        m = re.search(r"^([A-Za-z0-9_\-]+)\.Signal=" + re.escape(signal) + r"[ \t\r]*$", text, re.M)
        found[signal] = m.group(1) if m else "?"

    log("[检查] %s" % ioc)
    log("[检查] SPI1_SCK=%s  SPI1_MISO=%s  SPI1_MOSI=%s"
        % (found["SPI1_SCK"], found["SPI1_MISO"], found["SPI1_MOSI"]))

    if found["SPI1_MOSI"] != EXPECT_IO["SPI1_MOSI"]:
        log("  警告：C 板的 SPI1_MOSI 必须是 %s，当前 .ioc 里是 %s ！"
            % (EXPECT_IO["SPI1_MOSI"], found["SPI1_MOSI"]))
        log("        配错的后果：IMU 收不到任何字节、永远不应答，读回来的是浮空线上的耦合波形，")
        log("        现象非常像\"芯片在应答但数据不对\"。请到 CubeMX 里把 MOSI 改到 %s 并重新生成代码。"
            % EXPECT_IO["SPI1_MOSI"])
    if found["SPI1_SCK"] != EXPECT_IO["SPI1_SCK"] or found["SPI1_MISO"] != EXPECT_IO["SPI1_MISO"]:
        log("  警告：SCK/MISO 期望 %s/%s，请确认接线"
            % (EXPECT_IO["SPI1_SCK"], EXPECT_IO["SPI1_MISO"]))

    for pin in ("PA4", "PB0"):
        m = re.search(r"^" + pin + r"\.GPIO_ModeDefaultOutputPP=(\S+)[ \t\r]*$", text, re.M)
        if m and m.group(1).strip().upper().endswith("_OD"):
            log("  警告：片选 %s 被配成开漏(%s)，应改成 GPIO_MODE_OUTPUT_PP"
                % (pin, m.group(1)))
    log("        提示：SPI1 需为 全双工主机 / 8bit / MSB first / 模式3(CPOL=High, CPHA=2Edge)")


def check_conflicts(uvprojx, log):
    """检查工程里是否已有别的 BMI088 驱动：它们和本库的全局符号重名，会报 L6200E"""
    text = read_text(uvprojx)
    uv_dir = os.path.dirname(uvprojx)
    symbols = ("bmi088_cs_probe", "bmi088_accel_dummy", "bmi088_accel_error",
               "bmi088_gyro_error", "BMI088_AccelInit", "BMI088_GyroInit", "BMI088_ReadData")

    hits = []
    for rel in re.findall(r"<FilePath>([^<]+)</FilePath>", text):
        if LIB_DIR_NAME in rel:
            continue                                  # 跳过本库自己的文件
        if not rel.lower().endswith(".c"):
            continue
        path = os.path.normpath(os.path.join(uv_dir, rel.replace("\\", os.sep)))
        if not os.path.isfile(path):
            continue
        try:
            src = read_text(path)
        except OSError:
            continue
        for s in symbols:
            # 只认"定义"形式(前面带类型)，避免把注释里提到这些名字的文件也算进来
            pat = (r"^[ \t]*(static[ \t]+)?"
                   r"(unsigned[ \t]+char|uint8_t|uint16_t|int16_t|int32_t|float|double|void)[ \t]+"
                   + re.escape(s) + r"\b")
            if re.search(pat, src, re.M):
                hits.append((rel, s))
                break

    if hits:
        log("  警告：工程里已有的文件里出现了会和本库重名的符号，"
            "直接编译会链接报错 L6200E multiply defined：")
        for rel, s in hits[:8]:
            log("        %s   (含 %s)" % (rel, s))
        log("        请把这些旧驱动文件从工程里移除（Keil 里右键 Remove File），或改掉它们的符号名。")
    else:
        log("        没有发现会和本库重名的旧驱动，可以直接编译。")


# ---------------------------------------------------------------- 主流程

def configure(target, readme_to_root=True, log=print):
    uvprojx, uv_dir = find_uvprojx(target)
    log("找到 Keil 工程：%s" % uvprojx)

    copy_lib_files(uv_dir, log)
    patch_uvprojx(uvprojx, LIB_DIR_NAME, log)

    # README：库目录里已有一份，这里再放到工程目录(以及可选的 CubeMX 工程根目录)
    src_readme = os.path.join(src_lib_dir(), README_NAME)
    dst_readme = os.path.join(uv_dir, README_NAME)
    if os.path.abspath(src_readme) != os.path.abspath(dst_readme):
        shutil.copy2(src_readme, dst_readme)
    log("[5/5] README 已复制到 %s" % dst_readme)

    if readme_to_root:
        root = os.path.dirname(uv_dir)
        if root and os.path.abspath(root) != os.path.abspath(uv_dir):
            try:
                shutil.copy2(src_readme, os.path.join(root, README_NAME))
                log("      README 也复制到 %s" % root)
            except OSError as e:
                log("      (复制到工程根目录失败：%s)" % e)

    log("")
    check_ioc(uv_dir, log)

    log("")
    log("[检查] 是否已存在别的 BMI088 驱动(会和本库重名)")
    check_conflicts(uvprojx, log)

    log("")
    log("============================================================")
    log("配置完成。还需要你在代码里手动加两处(脚本不擅自改你的 main.c)：")
    log("")
    log('  1) main.c 的 USER CODE BEGIN Includes 里：  #include "bmi088.h"')
    log("")
    log("  2) API1 初始化(放在 while(1) 之前；FreeRTOS 里放在第 1 个任务)：")
    log("        while (BMI088_Init() != 0) { HAL_Delay(100); }   /* 0=成功 */")
    log("")
    log("     API2 读取(float数组, 读什么)：")
    log("        float data[3];")
    log("        BMI088_ReadData(data, BMI088_DATA_ACCEL);   /* m/s^2 */")
    log("        BMI088_ReadData(data, BMI088_DATA_GYRO);    /* rad/s  */")
    log("")
    log("  然后用 Keil 打开工程直接重新编译即可（文件和头文件路径已加好）。")
    log("  详细说明见工程目录下的 %s" % README_NAME)
    log("============================================================")


# ---------------------------------------------------------------- 图形界面

def run_gui():
    import tkinter as tk
    from tkinter import filedialog, messagebox, scrolledtext, ttk

    root = tk.Tk()
    root.title("BMI088 库自动配置工具  -  BMI088_final_lib")
    root.geometry("820x600")
    root.minsize(700, 500)

    state = {"readme_root": tk.BooleanVar(value=True), "uvprojx": None}

    def log(msg):
        box.configure(state="normal")
        box.insert("end", str(msg) + "\n")
        box.see("end")
        box.configure(state="disabled")
        root.update_idletasks()

    def clear_log():
        box.configure(state="normal")
        box.delete("1.0", "end")
        box.configure(state="disabled")

    # 顶部说明
    head = ttk.Label(
        root, justify="left",
        text=("把你 MDK-ARM 工程的路径填在下面，点\"开始配置\"即可。\n"
              "脚本会复制库文件、在 .uvprojx 里建好分组和头文件路径、复制 README，并检查 .ioc 引脚。\n"
              "修改 .uvprojx 前会自动备份，随时可以一键还原。"))
    head.pack(anchor="w", padx=12, pady=(12, 6))

    # 路径行
    row = ttk.Frame(root)
    row.pack(fill="x", padx=12)
    ttk.Label(row, text="MDK-ARM 工程文件夹：").pack(side="left")
    path_var = tk.StringVar()
    entry = ttk.Entry(row, textvariable=path_var)
    entry.pack(side="left", fill="x", expand=True, padx=(0, 6))

    def browse():
        d = filedialog.askdirectory(title="选择你的 MDK-ARM 工程文件夹（或工程根目录）")
        if d:
            path_var.set(os.path.normpath(d))

    ttk.Button(row, text="浏览...", command=browse).pack(side="left")

    opt = ttk.Frame(root)
    opt.pack(fill="x", padx=12, pady=6)
    ttk.Checkbutton(opt, text="把 README 也复制到 CubeMX 工程根目录",
                    variable=state["readme_root"]).pack(side="left")

    # 按钮行
    btns = ttk.Frame(root)
    btns.pack(fill="x", padx=12, pady=4)

    def do_configure():
        path = path_var.get().strip().strip('"')
        if not path:
            messagebox.showwarning("提示", "请先选择 MDK-ARM 工程文件夹")
            return
        clear_log()
        log("目标：%s" % path)
        log("库源：%s" % src_lib_dir())
        log("-" * 60)
        try:
            configure(path, readme_to_root=state["readme_root"].get(), log=log)
        except SetupError as e:
            log("")
            log("失败：%s" % e)
            messagebox.showerror("配置失败", str(e))
            return
        except Exception as e:  # 兜底，避免界面直接崩
            log("")
            log("出现未预期的错误：%r" % e)
            messagebox.showerror("出错了", "%r" % e)
            return
        messagebox.showinfo("完成", "库已配置好，去 Keil 里重新编译吧。\n\n"
                                    "注意：还需要在 main.c 里 #include \"bmi088.h\" 并调用 BMI088_Init()。")

    def do_restore():
        path = path_var.get().strip().strip('"')
        if not path:
            messagebox.showwarning("提示", "请先选择 MDK-ARM 工程文件夹")
            return
        try:
            uvprojx, _ = find_uvprojx(path)
            clear_log()
            log("还原：%s" % uvprojx)
            restore_latest_backup(uvprojx, log)
            messagebox.showinfo("完成", "已还原 .uvprojx")
        except SetupError as e:
            messagebox.showerror("还原失败", str(e))
        except Exception as e:
            messagebox.showerror("出错了", "%r" % e)

    def do_open_lib():
        d = src_lib_dir()
        if os.path.isdir(d):
            os.startfile(d) if hasattr(os, "startfile") else None
        else:
            messagebox.showerror("找不到", "库文件夹不存在：\n%s" % d)

    ttk.Button(btns, text="开始配置", command=do_configure).pack(side="left")
    ttk.Button(btns, text="还原 .uvprojx 备份", command=do_restore).pack(side="left", padx=6)
    ttk.Button(btns, text="打开库文件夹", command=do_open_lib).pack(side="left")
    ttk.Button(btns, text="退出", command=root.destroy).pack(side="right")

    # 日志区
    box = scrolledtext.ScrolledText(root, height=22, wrap="word",
                                    font=("Consolas", 9), state="disabled")
    box.pack(fill="both", expand=True, padx=12, pady=(6, 12))

    log("等待操作……")
    log("库源目录：%s" % src_lib_dir())
    root.mainloop()


# ---------------------------------------------------------------- 入口

def main():
    parser = argparse.ArgumentParser(
        description="把 BMI088_final_lib (BMI088 加速度计+陀螺仪库) 配置进 Keil MDK-ARM 工程")
    parser.add_argument("target", nargs="?", help="MDK-ARM 工程文件夹路径")
    parser.add_argument("--cli", metavar="PATH", help="命令行模式：直接配置这个路径下的工程")
    parser.add_argument("--no-readme", action="store_true", help="不把 README 复制到工程根目录")
    parser.add_argument("--restore", action="store_true", help="还原最近一次的 .uvprojx 备份")
    args = parser.parse_args()

    target = args.cli or args.target
    if target:
        try:
            sys.stdout.reconfigure(encoding="utf-8", errors="replace")
        except Exception:
            pass
        try:
            uvprojx, _ = find_uvprojx(target)
            if args.restore:
                print("还原：%s" % uvprojx)
                restore_latest_backup(uvprojx, print)
            else:
                configure(target, readme_to_root=not args.no_readme, log=print)
        except SetupError as e:
            print("失败：%s" % e)
            return 1
        return 0

    run_gui()
    return 0


if __name__ == "__main__":
    sys.exit(main())
