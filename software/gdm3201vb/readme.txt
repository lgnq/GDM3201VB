-- cn --
按需要生成工程文件:
1. 修改 rtconfig.py :
- CROSS_TOOL='keil'           #使用的工具链
- STM32_TYPE = 'STM32F10X_MD' #STM32对应的型号.
2. 修改 rtconfig.h 配置需要的功能.
3. 自动拷贝rt-thread目录
- set RTT_ROOT=xxx
- scons --copy
4. 执行 scons --target=mdk4 -s  生成MDK工程文件.

note: 需要安装 python,scons.

-- en --
