# RTX 2080 Ti 22GB 显存实测报告（2026-10-12）

显卡：NVIDIA RTX 2080 Ti (TU102, 10de:1e07)，CardExpert (10b0)，改装 22GB GDDR6，VBIOS 90.02.17.00.BE
平台：X79 主板，Ubuntu 24.04 LiveCD，内核 7.0.0-31，系统内存 128GB

## 结论

显卡硬件故障，定位到显存子系统：22 个显存通道（11 颗 GDDR6 × 2）中恰好 1 个完全失效，
其余 21 个正常。不是 PCIe 插槽或接触问题，不是驱动或系统问题。

## 测试方法

不依赖任何驱动或计算 API。直接 mmap 显卡 BAR0（/sys/bus/pci/devices/0000:03:00.0/resource0），
用 PRAMIN 窗口（寄存器 0x1700 设基址，0x700000 起 1MB 窗口）访问显存物理地址。
数据只经过显卡自己的显存控制器，所以不会把系统内存或 llvmpipe 软件渲染误认成显卡。

## 结果

| 测试 | 结果 | 日志 |
|---|---|---|
| 写 64KB 立即读回（命中显卡 L2 缓存） | 22GB 全部 OK | [logs/10](./logs/10_quick_test_cache_hit_all_ok.txt) |
| 先写满再读（数据真正进出显存颗粒） | 每 22 个 32 字节扇区坏 1 个 | logs/[14](./logs/14_sweep_first_64MB.txt), [15_bad](./logs/15_bad_addrs_sample.txt), [15_map](./logs/15_map_first_8MB.txt) |
| 全 22GB 每隔 1GB 抽样 | 坏 32768/720896 = 4.545% = 正好 1/22.00，各处均匀 | [logs/13](./logs/13_full_range_sample_1GB_step.txt) |
| 三种图案（随机/全0/全1） | 坏扇区位置 100% 相同（23840 个） | [logs/11](./logs/11_pattern_stability.txt) |
| 坏扇区重复读 5 次 | 0 个变化；0 个部分损坏（都是整扇区坏） | [logs/12](./logs/12_reread_stability.txt) |
| 坏扇区内容 | 约 98% 读回同一组固定垃圾值 264d609e d3fbffe6 ...，写入没有落地 | logs/[11](./logs/11_pattern_stability.txt), [13](./logs/13_full_range_sample_1GB_step.txt) |
| PCIe AER 错误计数（显卡与上游端口） | 全 0；RxErr/BadTLP/BadDLLP 均无 | [logs/01](./logs/01_pcie_link_aer.txt) |

## 为什么排除 PCIe 接触不良

1. 同一条 PCIe 通路，数据命中显卡缓存时完全正确（logs/10），只有经过显存颗粒的数据才错。
2. PCIe 包有 CRC 和自动重传，传输错误会记入 AER 计数，实测全 0。
3. 接触不良是随机、时有时无的错误；实测错误位置固定、可重复，并精确对应 1/22 的显存通道布局。
4. 开机花屏出现在任何驱动加载之前；其他显卡在这块主板上工作正常。
5. 链路显示 2.5GT/s "downgraded" 是无驱动时的正常节能降速，宽度 x16 完整。

## 与其他现象吻合

- NVIDIA 580 驱动：RmInitAdapter failed (0x24:0x72:1603)，0x72 = NV_ERR_MEMORY_ERROR
- nouveau GSP 模式：sec2(gsp) booter-load boot failed -5（logs/02）
- nouveau 非 GSP 模式：ACR 初始化读显存时内核 oops，伴随 IOMMU DMA 错误（logs/03）
- Windows Code 43、开机花屏

## 可能的具体原因（需维修店确认）

按可能性大致排序：
1. 改装时那一颗显存的 BGA 焊点虚焊或冷焊
2. 那颗颗粒本身损坏（一半通道失效）
3. PCB 上该通道走线或过孔损伤
4. GPU 核心对应的显存控制器通道损坏（少见，若如此基本无法修）
5. VBIOS 显存参数导致该通道训练失败（可能性低，因为其余 21 通道正常）

[MATS日志文件](./mats.log)

## 文件说明

- src/pramin.c：快速测试（每块写后立即读，会被缓存掩盖，作对照）
- src/sweep.c：区间整体写入后统一校验，统计逐位翻转
- src/map.c：前 8MB 坏地址分布，输出 bad_addrs
- src/stab.c：多图案坏扇区一致性
- src/reread.c：坏扇区重复读稳定性
- src/sample.c：全 22GB 抽样，区分固定垃圾与其他错误
- src/peek.py：只读查看 BAR0 寄存器
- 编译：gcc -O2 -o sample sample.c；需 root 运行；显卡不能被驱动正常占用时也能跑
- 注意：这些程序会改写显存内容，并临时改动 0x1700 寄存器（结束时恢复）

日志 logs/ 下按编号排序：00 系统与寄存器，01 PCIe/AER，02 开机 dmesg，03 nouveau 崩溃，
04 当前 dmesg，10-15 显存测试输出。

## License

MIT